#include <orbis/libkernel.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>

struct OrbisBgftInitParams { void* heap; size_t heapSize; };
using OrbisBgftTaskId = int32_t;
enum { ORBIS_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM = 0x10000 };
struct OrbisBgftDownloadParam {
    int32_t userId;
    int32_t entitlementType;
    const char* id;
    const char* contentUrl;
    const char* contentExUrl;
    const char* contentName;
    const char* iconPath;
    const char* skuId;
    int32_t option;
    const char* playgoScenarioId;
    const char* releaseDate;
    const char* packageType;
    const char* packageSubType;
    uint32_t packageSize;
};
struct OrbisBgftTaskProgress {
    uint32_t bits;
    int32_t errorResult;
    uint32_t length;
    uint32_t transferred;
    uint32_t lengthTotal;
    uint32_t transferredTotal;
    uint32_t numIndex;
    uint32_t numTotal;
    uint32_t restSec;
    uint32_t restSecTotal;
    int32_t preparingPercent;
    int32_t localCopyPercent;
};

using BgftInitFn = int32_t (*)(OrbisBgftInitParams*);
using BgftTermFn = int32_t (*)();
using BgftRegisterFn = int32_t (*)(OrbisBgftDownloadParam*, OrbisBgftTaskId*);
using BgftStartFn = int32_t (*)(OrbisBgftTaskId);
using BgftProgressFn = int32_t (*)(OrbisBgftTaskId, OrbisBgftTaskProgress*);
using NetInitFn = int32_t (*)();
using NetCtlInitFn = int32_t (*)();
using NetPoolCreateFn = int32_t (*)(const char*, int32_t, int32_t);
using NetCtlGetInfoFn = int32_t (*)(int32_t, void*);
using UserInitFn = int32_t (*)(void*);
using UserGetForegroundFn = int32_t (*)(int32_t*);
using AppInstInitFn = int32_t (*)();
using SysmoduleLoadInternalFn = uint32_t (*)(uint32_t);

union Ny0NetCtlInfo { char ip_address[16]; char proxy_server[256]; };
static constexpr int ORBIS_NET_CTL_INFO_IP_ADDRESS = 14;

static constexpr int SERVER_PORT = 12800;
static constexpr size_t UPLOAD_CHUNK_MAX = 2 * 1024 * 1024;
static constexpr uint32_t BGFT_HEAP_SIZE = 1024 * 1024;

static BgftInitFn g_bgft_init = nullptr;
static BgftTermFn g_bgft_term = nullptr;
static BgftRegisterFn g_bgft_register = nullptr;
static BgftStartFn g_bgft_start = nullptr;
static BgftProgressFn g_bgft_progress = nullptr;
static NetInitFn g_net_init = nullptr;
static NetCtlInitFn g_netctl_init = nullptr;
static NetPoolCreateFn g_net_pool_create = nullptr;
static NetCtlGetInfoFn g_netctl_get_info = nullptr;
static UserInitFn g_user_init = nullptr;
static UserGetForegroundFn g_user_get_foreground = nullptr;
static AppInstInitFn g_appinst_init = nullptr;
static SysmoduleLoadInternalFn g_sysmodule_load_internal = nullptr;
static int32_t g_sysmodule_module = -1;
static int32_t g_bgft_module = -1;
static int32_t g_net_module = -1;
static int32_t g_netctl_module = -1;
static int32_t g_user_module = -1;
static int32_t g_appinst_module = -1;
static int32_t g_net_pool = -1;
static char g_ip[16] = "0.0.0.0";
static bool g_install_ready = false;
static OrbisBgftInitParams g_bgft_params{};
static int32_t g_last_task = -1;

static bool send_all(int fd, const void* data, size_t size) {
    const char* p = static_cast<const char*>(data);
    while (size) {
        ssize_t n = send(fd, p, size, 0);
        if (n <= 0) return false;
        p += n;
        size -= static_cast<size_t>(n);
    }
    return true;
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

static std::string header_value(const std::string& headers, const char* key) {
    std::string needle = std::string(key) + ":";
    std::string h = lower(headers);
    std::string n = lower(needle);
    size_t p = h.find(n);
    if (p == std::string::npos) return "";
    p += needle.size();
    while (p < headers.size() && (headers[p] == ' ' || headers[p] == '\t')) ++p;
    size_t e = headers.find("\r\n", p);
    if (e == std::string::npos) e = headers.size();
    return headers.substr(p, e - p);
}

static std::string query_value(const std::string& target, const char* key) {
    size_t q = target.find('?');
    if (q == std::string::npos) return "";
    std::string query = target.substr(q + 1);
    std::string needle = std::string(key) + "=";
    size_t p = query.find(needle);
    if (p == std::string::npos) return "";
    p += needle.size();
    size_t e = query.find('&', p);
    return query.substr(p, e == std::string::npos ? std::string::npos : e - p);
}

static bool safe_filename(const std::string& name) {
    if (name.empty() || name.size() > 180 || name.find("..") != std::string::npos) return false;
    if (name.size() < 4 || lower(name.substr(name.size() - 4)) != ".pkg") return false;
    for (unsigned char c : name) {
        if (!(isalnum(c) || c == '-' || c == '_' || c == '.')) return false;
    }
    return true;
}

static void response(int fd, int code, const char* reason, const char* type, const std::string& body) {
    char head[512];
    int n = snprintf(head, sizeof(head),
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
        "Connection: close\r\nCache-Control: no-store\r\n\r\n",
        code, reason, type, static_cast<unsigned long>(body.size()));
    if (n > 0) send_all(fd, head, static_cast<size_t>(n));
    if (!body.empty()) send_all(fd, body.data(), body.size());
}

static void json_error(int fd, int code, const char* message) {
    std::string body = "{\"status\":\"error\",\"message\":\"";
    for (const char* p = message; *p; ++p) {
        if (*p == '"' || *p == '\\') body.push_back('\\');
        if (static_cast<unsigned char>(*p) >= 32) body.push_back(*p);
    }
    body += "\"}";
    response(fd, code, code == 400 ? "Bad Request" : "Internal Server Error", "application/json; charset=utf-8", body);
}

static bool send_file(int fd, const char* path, const char* content_type) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    fseeko(fp, 0, SEEK_END);
    off_t total = ftello(fp);
    fseeko(fp, 0, SEEK_SET);
    if (total < 0) { fclose(fp); return false; }
    char head[512];
    int n = snprintf(head, sizeof(head),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %llu\r\n"
        "Connection: close\r\nCache-Control: no-store\r\n\r\n",
        content_type, static_cast<unsigned long long>(total));
    bool ok = n > 0 && send_all(fd, head, static_cast<size_t>(n));
    char buf[32768];
    while (ok) {
        size_t got = fread(buf, 1, sizeof(buf), fp);
        if (got && !send_all(fd, buf, got)) { ok = false; break; }
        if (got < sizeof(buf)) break;
    }
    fclose(fp);
    return ok;
}

static bool send_download(int fd, const std::string& name, const std::string& range) {
    if (!safe_filename(name)) return false;
    std::string path = "/data/ny0_uploads/" + name;
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) return false;
    fseeko(fp, 0, SEEK_END);
    off_t total = ftello(fp);
    if (total <= 0) { fclose(fp); return false; }

    off_t start = 0, end = total - 1;
    bool partial = false;
    if (!range.empty() && range.rfind("bytes=", 0) == 0) {
        const char* p = range.c_str() + 6;
        char* tail = nullptr;
        long long s = strtoll(p, &tail, 10);
        if (tail == p || s < 0 || s >= total || *tail != '-') { fclose(fp); return false; }
        start = static_cast<off_t>(s);
        if (*(tail + 1)) {
            long long e = strtoll(tail + 1, &tail, 10);
            if (e < start) { fclose(fp); return false; }
            end = static_cast<off_t>(std::min<long long>(e, total - 1));
        }
        partial = true;
    }
    off_t length = end - start + 1;
    fseeko(fp, start, SEEK_SET);

    char head[768];
    int n;
    if (partial) {
        n = snprintf(head, sizeof(head),
            "HTTP/1.1 206 Partial Content\r\nContent-Type: application/octet-stream\r\n"
            "Accept-Ranges: bytes\r\nContent-Range: bytes %llu-%llu/%llu\r\n"
            "Content-Length: %llu\r\nConnection: close\r\n\r\n",
            static_cast<unsigned long long>(start), static_cast<unsigned long long>(end),
            static_cast<unsigned long long>(total), static_cast<unsigned long long>(length));
    } else {
        n = snprintf(head, sizeof(head),
            "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
            "Accept-Ranges: bytes\r\nContent-Length: %llu\r\nConnection: close\r\n\r\n",
            static_cast<unsigned long long>(length));
    }
    bool ok = n > 0 && send_all(fd, head, static_cast<size_t>(n));
    char buf[65536];
    while (ok && length > 0) {
        size_t want = static_cast<size_t>(std::min<off_t>(length, sizeof(buf)));
        size_t got = fread(buf, 1, want, fp);
        if (!got) { ok = false; break; }
        if (!send_all(fd, buf, got)) { ok = false; break; }
        length -= static_cast<off_t>(got);
    }
    fclose(fp);
    return ok;
}

static int32_t load_module(const char* path) {
    return static_cast<int32_t>(sceKernelLoadStartModule(path, 0, nullptr, 0, nullptr, nullptr));
}

static bool resolve_symbol(int32_t module, const char* name, void** out) {
    return module > 0 && sceKernelDlsym(module, name, out) == 0 && *out != nullptr;
}

static bool init_bgft() {
    g_bgft_module = load_module("/system/common/lib/libSceBgft.sprx");
    if (g_bgft_module <= 0) return false;

    if (!resolve_symbol(g_bgft_module, "sceBgftServiceInit", reinterpret_cast<void**>(&g_bgft_init))) return false;
    if (!resolve_symbol(g_bgft_module, "sceBgftServiceTerm", reinterpret_cast<void**>(&g_bgft_term))) return false;
    if (!resolve_symbol(g_bgft_module, "sceBgftServiceDownloadRegisterTask", reinterpret_cast<void**>(&g_bgft_register))) return false;
    if (!resolve_symbol(g_bgft_module, "sceBgftServiceIntDownloadStartTask", reinterpret_cast<void**>(&g_bgft_start))) return false;
    if (!resolve_symbol(g_bgft_module, "sceBgftServiceIntDownloadGetProgress", reinterpret_cast<void**>(&g_bgft_progress))) return false;

    g_bgft_params.heapSize = BGFT_HEAP_SIZE;
    g_bgft_params.heap = malloc(g_bgft_params.heapSize);
    if (!g_bgft_params.heap) return false;
    memset(g_bgft_params.heap, 0, g_bgft_params.heapSize);
    if (g_bgft_init(&g_bgft_params) != 0) return false;
    return true;
}

static uint32_t read_be32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) |
           static_cast<uint32_t>(p[3]);
}

static uint64_t read_be64(const uint8_t* p) {
    return (static_cast<uint64_t>(read_be32(p)) << 32) | read_be32(p + 4);
}

static void bytes_to_hex(char* out, const uint8_t* bytes, size_t count) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < count; ++i) {
        out[i * 2] = hex[bytes[i] >> 4];
        out[i * 2 + 1] = hex[bytes[i] & 0x0f];
    }
    out[count * 2] = 0;
}

static bool start_install(const std::string& name, int* task_out, int* error_out) {
    if (error_out) *error_out = -1;
    if (!g_install_ready || !g_bgft_register || !g_bgft_start) return false;
    if (!safe_filename(name)) return false;

    std::string path = "/data/ny0_uploads/" + name;
    struct stat st{};
    if (stat(path.c_str(), &st) != 0 || st.st_size < 0x2000 ||
        static_cast<unsigned long long>(st.st_size) > 0xFFFFFFFFull) return false;

    uint8_t header[0x2000];
    FILE* pkg = fopen(path.c_str(), "rb");
    if (!pkg) return false;
    size_t header_size = fread(header, 1, sizeof(header), pkg);
    fclose(pkg);
    if (header_size != sizeof(header) ||
        header[0] != 0x7f || header[1] != 'C' || header[2] != 'N' || header[3] != 'T') return false;

    uint64_t package_size = read_be64(header + 0x430);
    if (package_size == 0 || package_size != static_cast<uint64_t>(st.st_size) ||
        package_size > 0xFFFFFFFFull) return false;

    char content_id[37];
    memcpy(content_id, header + 0x40, 36);
    content_id[36] = 0;
    if (content_id[6] != '-' || content_id[16] != '_' || content_id[19] != '-') return false;

    const char* package_type = nullptr;
    switch (read_be32(header + 0x74)) {
        case 0x1A: package_type = "PS4GD"; break;
        case 0x1B: package_type = "PS4AC"; break;
        case 0x1C: package_type = "PS4AL"; break;
        case 0x1E: package_type = "PS4DP"; break;
        default: return false;
    }

    char package_digest[65];
    bytes_to_hex(package_digest, header + 0xFE0, 32);
    char piece_digest[41];
    memset(piece_digest, '0', 40);
    piece_digest[40] = 0;

    std::string package_url = std::string("http://") + g_ip + ":" +
        std::to_string(SERVER_PORT) + "/download/" + name;
    std::string manifest_url = std::string("http://") + g_ip + ":" +
        std::to_string(SERVER_PORT) + "/manifest/" + name + ".json";
    std::string manifest_path = path + ".json";

    FILE* manifest = fopen(manifest_path.c_str(), "wb");
    if (!manifest) return false;
    int manifest_written = fprintf(manifest,
        "{\"originalFileSize\":%llu,\"packageDigest\":\"%s\","
        "\"numberOfSplitFiles\":1,\"pieces\":[{\"url\":\"%s\","
        "\"fileOffset\":0,\"fileSize\":%llu,\"hashValue\":\"%s\"}]}",
        static_cast<unsigned long long>(package_size), package_digest,
        package_url.c_str(), static_cast<unsigned long long>(package_size), piece_digest);
    bool manifest_ok = manifest_written > 0 && fclose(manifest) == 0;
    if (!manifest_ok) return false;

    int user_id = -1;
    int ret = g_user_get_foreground ? g_user_get_foreground(&user_id) : -1;
    if (ret != 0) { if (error_out) *error_out = ret; return false; }

    OrbisBgftDownloadParam params{};
    params.userId = user_id;
    params.entitlementType = 5;
    params.id = content_id;
    params.contentUrl = manifest_url.c_str();
    params.contentName = name.c_str();
    params.iconPath = "";
    params.option = ORBIS_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM;
    params.playgoScenarioId = "0";
    params.packageType = package_type;
    params.packageSubType = "";
    params.packageSize = static_cast<uint32_t>(package_size);

    OrbisBgftTaskId task = -1;
    ret = g_bgft_register(&params, &task);
    if (ret == 0) ret = g_bgft_start(task);
    if (error_out) *error_out = ret;
    if (ret != 0) return false;
    g_last_task = task;
    if (task_out) *task_out = task;
    return true;
}

static void handle_client(int fd) {
    char header_buf[8192];
    size_t used = 0;
    size_t header_end = std::string::npos;
    while (used < sizeof(header_buf) - 1 && header_end == std::string::npos) {
        ssize_t n = recv(fd, header_buf + used, sizeof(header_buf) - used - 1, 0);
        if (n <= 0) return;
        used += static_cast<size_t>(n);
        header_buf[used] = 0;
        std::string tmp(header_buf, used);
        header_end = tmp.find("\r\n\r\n");
    }
    if (header_end == std::string::npos) { json_error(fd, 400, "HTTP headers too large"); return; }

    std::string headers(header_buf, header_end);
    std::string first = headers.substr(0, headers.find("\r\n"));
    size_t sp1 = first.find(' ');
    size_t sp2 = first.find(' ', sp1 == std::string::npos ? sp1 : sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) { json_error(fd, 400, "Invalid request line"); return; }
    std::string method = first.substr(0, sp1);
    std::string target = first.substr(sp1 + 1, sp2 - sp1 - 1);
    size_t body_pos = header_end + 4;
    size_t body_have = used > body_pos ? used - body_pos : 0;
    std::string length_text = header_value(headers, "Content-Length");
    size_t content_length = length_text.empty() ? 0 : static_cast<size_t>(strtoull(length_text.c_str(), nullptr, 10));

    if (method == "GET" || method == "HEAD") {
        if (target == "/" || target == "/index.html") {
            send_file(fd, "/app0/web/index.html", "text/html; charset=utf-8");
        } else if (target == "/style.css") {
            send_file(fd, "/app0/web/style.css", "text/css; charset=utf-8");
        } else if (target == "/app.js") {
            send_file(fd, "/app0/web/app.js", "application/javascript; charset=utf-8");
        } else if (target == "/api/status") {
            std::string body = std::string("{\"status\":\"online\",\"ip\":\"") + g_ip +
                "\",\"port\":12800,\"install_ready\":" + (g_install_ready ? "true" : "false") + "}";
            response(fd, 200, "OK", "application/json; charset=utf-8", body);
        } else if (target.rfind("/api/progress", 0) == 0) {
            OrbisBgftTaskProgress p{};
            int ret = (g_last_task >= 0 && g_bgft_progress) ? g_bgft_progress(g_last_task, &p) : -1;
            if (ret != 0) {
                json_error(fd, 500, "No active installation task");
            } else {
                char body[512];
                snprintf(body, sizeof(body),
                    "{\"status\":\"success\",\"task_id\":%d,\"error\":%d,"
                    "\"transferred\":%u,\"total\":%u,\"preparing_percent\":%d,"
                    "\"local_copy_percent\":%d,\"remaining_seconds\":%u}",
                    g_last_task, p.errorResult, p.transferredTotal, p.lengthTotal,
                    p.preparingPercent, p.localCopyPercent, p.restSecTotal);
                response(fd, 200, "OK", "application/json; charset=utf-8", body);
            }
        } else if (target.rfind("/manifest/", 0) == 0) {
            std::string manifest_name = target.substr(strlen("/manifest/"));
            if (manifest_name.size() > 5 && manifest_name.substr(manifest_name.size() - 5) == ".json") {
                manifest_name.resize(manifest_name.size() - 5);
            }
            std::string manifest_path = "/data/ny0_uploads/" + manifest_name + ".json";
            if (!safe_filename(manifest_name) || !send_file(fd, manifest_path.c_str(), "application/json; charset=utf-8")) {
                json_error(fd, 404, "Package manifest not found");
            }
        } else if (target.rfind("/download/", 0) == 0) {
            std::string name = target.substr(strlen("/download/"));
            if (!send_download(fd, name, header_value(headers, "Range"))) json_error(fd, 404, "Package file not found or invalid range");
        } else {
            json_error(fd, 404, "Not found");
        }
        return;
    }

    if (method != "POST") { json_error(fd, 400, "Unsupported method"); return; }

    if (target.rfind("/api/upload?", 0) == 0) {
        std::string name = query_value(target, "name");
        std::string offset_text = query_value(target, "offset");
        if (!safe_filename(name) || content_length == 0 || content_length > UPLOAD_CHUNK_MAX) {
            json_error(fd, 400, "Invalid upload name or chunk size"); return;
        }
        off_t offset = static_cast<off_t>(strtoll(offset_text.c_str(), nullptr, 10));
        if (offset < 0) { json_error(fd, 400, "Invalid upload offset"); return; }
        mkdir("/data/ny0_uploads", 0777);
        std::string path = "/data/ny0_uploads/" + name;
        FILE* fp = fopen(path.c_str(), offset == 0 ? "wb" : "r+b");
        if (!fp) { json_error(fd, 500, "Cannot open upload file"); return; }
        if (fseeko(fp, offset, SEEK_SET) != 0) { fclose(fp); json_error(fd, 500, "Cannot seek upload file"); return; }

        size_t written = 0;
        if (body_have) {
            size_t take = std::min(body_have, content_length);
            if (fwrite(header_buf + body_pos, 1, take, fp) != take) { fclose(fp); json_error(fd, 500, "Cannot write upload chunk"); return; }
            written += take;
        }
        char chunk[32768];
        while (written < content_length) {
            size_t want = std::min(sizeof(chunk), content_length - written);
            ssize_t n = recv(fd, chunk, want, 0);
            if (n <= 0) { fclose(fp); json_error(fd, 400, "Upload connection interrupted"); return; }
            if (fwrite(chunk, 1, static_cast<size_t>(n), fp) != static_cast<size_t>(n)) { fclose(fp); json_error(fd, 500, "Cannot write upload chunk"); return; }
            written += static_cast<size_t>(n);
        }
        fflush(fp);
        fclose(fp);
        char body[256];
        snprintf(body, sizeof(body), "{\"status\":\"success\",\"received\":%lu,\"offset\":%lld}",
            static_cast<unsigned long>(written), static_cast<long long>(offset));
        response(fd, 200, "OK", "application/json; charset=utf-8", body);
        return;
    }

    if (target == "/api/install") {
        if (content_length == 0 || content_length > 4096) { json_error(fd, 400, "Invalid install request"); return; }
        std::string body;
        body.resize(content_length);
        size_t copied = std::min(body_have, content_length);
        if (copied) memcpy(&body[0], header_buf + body_pos, copied);
        size_t got = copied;
        while (got < content_length) {
            ssize_t n = recv(fd, &body[got], content_length - got, 0);
            if (n <= 0) { json_error(fd, 400, "Install request interrupted"); return; }
            got += static_cast<size_t>(n);
        }
        size_t key = body.find("\"name\"");
        size_t colon = key == std::string::npos ? key : body.find(':', key);
        size_t q1 = colon == std::string::npos ? colon : body.find('"', colon);
        size_t q2 = q1 == std::string::npos ? q1 : body.find('"', q1 + 1);
        if (q1 == std::string::npos || q2 == std::string::npos) { json_error(fd, 400, "Missing package name"); return; }
        std::string name = body.substr(q1 + 1, q2 - q1 - 1);
        int task = -1, error = 0;
        if (!start_install(name, &task, &error)) {
            char msg[160];
            snprintf(msg, sizeof(msg), "Install registration failed (0x%08X). Check filename/content ID and GoldHEN readiness.", error);
            json_error(fd, 500, msg);
            return;
        }
        char out[256];
        snprintf(out, sizeof(out), "{\"status\":\"success\",\"task_id\":%d,\"name\":\"%s\"}", task, name.c_str());
        response(fd, 200, "OK", "application/json; charset=utf-8", out);
        return;
    }

    json_error(fd, 404, "Unknown API endpoint");
}

static bool init_network() {
    g_netctl_module = load_module("/system/common/lib/libSceNetCtl.sprx");
    g_net_module = load_module("/system/common/lib/libSceNet.sprx");
    if (g_netctl_module <= 0 || g_net_module <= 0) return false;
    if (!resolve_symbol(g_netctl_module, "sceNetCtlInit", reinterpret_cast<void**>(&g_netctl_init))) return false;
    if (!resolve_symbol(g_netctl_module, "sceNetCtlGetInfo", reinterpret_cast<void**>(&g_netctl_get_info))) return false;
    if (!resolve_symbol(g_net_module, "sceNetInit", reinterpret_cast<void**>(&g_net_init))) return false;
    if (!resolve_symbol(g_net_module, "sceNetPoolCreate", reinterpret_cast<void**>(&g_net_pool_create))) return false;
    if (g_netctl_init() != 0) return false;
    if (g_net_init() != 0) return false;
    g_net_pool = g_net_pool_create("ny0_remote_installer", 1024 * 1024, 0);
    if (g_net_pool < 0) return false;
    Ny0NetCtlInfo info{};
    if (g_netctl_get_info(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info) == 0 && info.ip_address[0]) {
        strncpy(g_ip, info.ip_address, sizeof(g_ip) - 1);
    }
    return true;
}

static bool init_system_services() {
    g_sysmodule_module = load_module("/system/common/lib/libSceSysmodule.sprx");
    if (!resolve_symbol(g_sysmodule_module, "sceSysmoduleLoadModuleInternal", reinterpret_cast<void**>(&g_sysmodule_load_internal))) return false;
    const uint32_t modules[] = {
        0x80000004, // SYSCORE
        0x80000010, // SYSTEM_SERVICE
        0x80000011, // USER_SERVICE
        0x80000009, // NETCTL
        0x8000001C, // NET
        0x8000000A, // HTTP
        0x8000000B, // SSL
        0x80000014, // APP_INST_UTIL
        0x8000002A, // BGFT
        0x8000000C  // NP_COMMON
    };
    for (uint32_t module_id : modules) {
        if (g_sysmodule_load_internal(module_id) != 0) return false;
    }

    g_user_module = load_module("/system/common/lib/libSceUserService.sprx");
    g_appinst_module = load_module("/system/common/lib/libSceAppInstUtil.sprx");
    if (g_user_module <= 0 || g_appinst_module <= 0) return false;
    if (!resolve_symbol(g_user_module, "sceUserServiceInitialize", reinterpret_cast<void**>(&g_user_init))) return false;
    if (!resolve_symbol(g_user_module, "sceUserServiceGetForegroundUser", reinterpret_cast<void**>(&g_user_get_foreground))) return false;
    if (!resolve_symbol(g_appinst_module, "sceAppInstUtilInitialize", reinterpret_cast<void**>(&g_appinst_init))) return false;
    if (g_user_init(nullptr) != 0) return false;
    if (g_appinst_init() != 0) return false;
    g_install_ready = init_bgft();
    return true;
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("NY0 Remote Installer starting...\n");

    if (!init_system_services()) {
        printf("System service initialization incomplete.\n");
    }
    if (!init_network()) {
        printf("Network initialization failed.\n");
        for (;;) sceKernelUsleep(1000000);
    }

    mkdir("/data/ny0_uploads", 0777);
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        printf("socket() failed\n");
        for (;;) sceKernelUsleep(1000000);
    }
    int yes = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr{};
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0 || listen(server, 4) != 0) {
        printf("Cannot listen on port %d\n", SERVER_PORT);
        close(server);
        for (;;) sceKernelUsleep(1000000);
    }

    printf("NY0 HTTP server: http://%s:%d (install adapter: %s)\n",
        g_ip, SERVER_PORT, g_install_ready ? "ready" : "unavailable");

    for (;;) {
        struct sockaddr_in peer{};
        socklen_t peer_len = sizeof(peer);
        int client = accept(server, reinterpret_cast<struct sockaddr*>(&peer), &peer_len);
        if (client < 0) {
            sceKernelUsleep(10000);
            continue;
        }
        handle_client(client);
        shutdown(client, SHUT_RDWR);
        close(client);
    }
    return 0;
}
