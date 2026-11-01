#!/usr/bin/env bash
set -euxo pipefail
export OO_PS4_TOOLCHAIN=/lib/OpenOrbisSDK
cd /workspace/ps4
test -x tools/create-gp4
test -x tools/create-fself
test -x "$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core"

# Stage browser UI from repository root.
mkdir -p web
cp -v /workspace/web/index.html /workspace/web/style.css /workspace/web/app.js web/
test -s web/index.html
test -s web/style.css
test -s web/app.js

# Include the official SDK runtime resources.
mkdir -p sce_sys/about sce_module
cp -v "$OO_PS4_TOOLCHAIN/bin/data/right.sprx" sce_sys/about/right.sprx
cp -v "$OO_PS4_TOOLCHAIN/bin/data/modules/libc.prx" sce_module/libc.prx
cp -v "$OO_PS4_TOOLCHAIN/bin/data/modules/libSceFios2.prx" sce_module/libSceFios2.prx

# Build and validate the full NY0 package.
make clean
make
"$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core" pkg_validate IV0000-NYOR00001_00-NY0REMOTEINSTALL.pkg
sha256sum IV0000-NYOR00001_00-NY0REMOTEINSTALL.pkg
ls -lh IV0000-NYOR00001_00-NY0REMOTEINSTALL.pkg eboot.bin

# Build and validate the independent launch smoke test.
cd /workspace/ps4/diagnostics/launch_test
mkdir -p tools sce_sys sce_sys/about sce_module
cp -v /workspace/ps4/tools/create-gp4 tools/create-gp4
cp -v /workspace/ps4/tools/create-fself tools/create-fself
cp -v /workspace/ps4/sce_sys/icon0.png sce_sys/icon0.png
cp -v /workspace/ps4/sce_sys/about/right.sprx sce_sys/about/right.sprx
cp -v /workspace/ps4/sce_module/libc.prx sce_module/libc.prx
cp -v /workspace/ps4/sce_module/libSceFios2.prx sce_module/libSceFios2.prx
make clean
make
"$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core" pkg_validate IV0000-NYOD00001_00-NY0LAUNCHTEST000.pkg
ls -lh IV0000-NYOD00001_00-NY0LAUNCHTEST000.pkg eboot.bin
cp -v IV0000-NYOD00001_00-NY0LAUNCHTEST000.pkg /workspace/ps4/NY0-LAUNCH-TEST.pkg
cp -v eboot.bin /workspace/ps4/NY0-LAUNCH-TEST-eboot.bin
