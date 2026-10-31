const picker = document.querySelector("#pkg");
const info = document.querySelector("#file-info");
const upload = document.querySelector("#upload");
const status = document.querySelector("#status");
const bar = document.querySelector("#bar");
const connection = document.querySelector("#connection");
const CHUNK_SIZE = 1024 * 1024;

let serviceReady = false;
let busy = false;

function setStatus(message) {
  status.textContent = message;
}

function safeName(name) {
  return name.replace(/[^A-Za-z0-9._-]/g, "_");
}


async function checkService() {
  try {
    const response = await fetch("/api/status", { cache: "no-store" });
    const data = await response.json();
    serviceReady = response.ok && data.status === "online" && data.install_ready;
    connection.innerHTML = serviceReady
      ? "<i></i> PS4 READY"
      : "<i></i> SERVICE LIMITED";
    connection.classList.toggle("ready", serviceReady);
    setStatus(serviceReady
      ? "Connected to the PS4. Select a package to begin."
      : "PS4 web service is online, but the package installation service did not initialize.");
  } catch {
    serviceReady = false;
    connection.textContent = "PS4 OFFLINE";
    setStatus("Cannot reach the PS4 service. Keep the NY0 app open and check the network.");
  }
  updateButton();
}

function updateButton() {
  const file = picker.files && picker.files[0];
  upload.disabled = busy || !serviceReady || !file || !file.name.toLowerCase().endsWith(".pkg");
}

picker.addEventListener("change", () => {
  const file = picker.files && picker.files[0];
  if (!file) {
    info.textContent = "No package selected";
    bar.style.width = "0%";
    updateButton();
    return;
  }
  const name = safeName(file.name);
  info.textContent = file.name + " · " + (file.size / 1024 / 1024).toFixed(1) + " MB";
  if (!file.name.toLowerCase().endsWith(".pkg")) {
    setStatus("Choose a .pkg file.");
  } else {
    setStatus("Package selected. NY0 reads the Content ID and digest from the PKG header automatically.");
  }
  updateButton();
});

async function uploadPackage(file) {
  const name = safeName(file.name);
  let offset = 0;
  while (offset < file.size) {
    const end = Math.min(offset + CHUNK_SIZE, file.size);
    const chunk = file.slice(offset, end);
    const url = "/api/upload?name=" + encodeURIComponent(name) + "&offset=" + offset;
    const response = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/octet-stream" },
      body: chunk
    });
    const result = await response.json().catch(() => ({}));
    if (!response.ok || result.status !== "success") {
      throw new Error(result.message || "Upload failed at " + (offset / file.size * 100).toFixed(0) + "%.");
    }
    offset = end;
    bar.style.width = Math.round((offset / file.size) * 70) + "%";
    setStatus("Uploading to PS4… " + Math.round((offset / file.size) * 100) + "%");
  }
  return name;
}

async function startInstall(name) {
  const response = await fetch("/api/install", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ name })
  });
  const result = await response.json().catch(() => ({}));
  if (!response.ok || result.status !== "success") {
    throw new Error(result.message || "PS4 rejected the installation task.");
  }
  return result.task_id;
}

async function watchProgress(taskId) {
  for (let i = 0; i < 1800; i++) {
    await new Promise(resolve => setTimeout(resolve, 2000));
    const response = await fetch("/api/progress", { cache: "no-store" });
    const data = await response.json();
    if (!response.ok || data.status !== "success") {
      setStatus("Task " + taskId + " was accepted. The progress service is temporarily unavailable.");
      return;
    }
    if (data.error) {
      throw new Error("PS4 installation task reported error 0x" + Number(data.error >>> 0).toString(16).toUpperCase());
    }
    const total = Number(data.total || 0);
    const transferred = Number(data.transferred || 0);
    if (total > 0) {
      const percent = Math.min(100, Math.round((transferred / total) * 100));
      bar.style.width = (70 + percent * 0.3) + "%";
      setStatus("PS4 download task " + percent + "% · " + Number(data.remaining_seconds || 0) + "s remaining");
      if (transferred >= total) {
        setStatus("PS4 has received the package. Check the console notification for the final installation result.");
        return;
      }
    } else {
      setStatus("PS4 accepted task " + taskId + ". Preparing package…");
    }
  }
  setStatus("The PS4 task is still running. Check the console notification for its final result.");
}

upload.addEventListener("click", async () => {
  const file = picker.files && picker.files[0];
  if (!file || busy) return;
  busy = true;
  updateButton();
  bar.style.width = "0%";
  try {
    setStatus("Preparing upload…");
    const name = await uploadPackage(file);
    setStatus("Registering installation with the PS4…");
    const taskId = await startInstall(name);
    bar.style.width = "70%";
    setStatus("PS4 accepted installation task " + taskId + ".");
    await watchProgress(taskId);
  } catch (error) {
    setStatus(error.message || "Unexpected error.");
  } finally {
    busy = false;
    updateButton();
  }
});

checkService();
setInterval(checkService, 15000);