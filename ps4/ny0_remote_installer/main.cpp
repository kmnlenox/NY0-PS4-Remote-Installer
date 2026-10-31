#include <orbis/libkernel.h>
#include <stdio.h>

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\nNY0 Remote Installer\n");
    printf("Diagnostic build 0.1.0\n");
    printf("----------------------\n");
    printf("This package verifies the OpenOrbis build pipeline only.\n");
    printf("The HTTP bridge and PKG installation adapter are not enabled.\n");
    printf("Do not treat this diagnostic package as the finished installer.\n");

    for (;;)
        sceKernelUsleep(1000000);

    return 0;
}
