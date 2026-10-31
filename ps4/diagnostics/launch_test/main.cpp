#include <orbis/libkernel.h>

// Known-good minimal process: no C++ runtime features, networking,
// filesystem access, system modules, or GoldHEN APIs.
int main() {
    for (;;) {
        sceKernelUsleep(1000000);
    }
    return 0;
}
