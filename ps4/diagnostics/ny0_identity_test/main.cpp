#include <orbis/libkernel.h>

// Identity-matched launch isolation test.
// Uses NY0's exact TITLE_ID, CONTENT_ID, SFO metadata, icon and package layout.
// No C++ runtime features beyond the SDK entry point, no services, no filesystem,
// and no networking. A black screen while this runs is expected.
int main() {
    for (;;) {
        sceKernelUsleep(1000000);
    }
    return 0;
}
