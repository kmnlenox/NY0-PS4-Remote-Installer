# Build and compatibility status

Updated: 2026-10-31

## Current milestone

The repository is being prepared for a reproducible OpenOrbis PS4 homebrew package build. The first package is a diagnostic shell only; it is not yet the remote installer.

## Verified from upstream documentation

- OpenOrbis provides a PS4 homebrew toolchain and sample Makefile/build workflow.
- GoldHEN release notes list Remote Package Install, FTP on port 2121, and BinLoader on port 9090.
- GoldHEN's README describes internal PKG installation support under `/data/pkg`.
- GoldHEN warns that BinLoader is experimental and that payloads may crash the console.

## Not yet verified

- A documented, stable HTTP upload endpoint provided by the user's GoldHEN build.
- A supported local API/command that this homebrew can invoke to install an uploaded PKG.
- Compatibility of the user's reported PS4 firmware 13.52 with the exact GoldHEN build in use.
- Runtime operation on the user's console.

## Hard rule

The application must fail closed when the installation backend is unavailable. It must never show “Installed” based only on a successful HTTP upload or file write.

## Next engineering tasks

1. Confirm the OpenOrbis package builds in GitHub Actions.
2. Implement and test a PS4-side HTTP server using APIs available in the OpenOrbis SDK.
3. Verify the exact GoldHEN Remote Package Install interface from authoritative source or console-side testing.
4. Connect the frontend to that backend and expose real transfer/install states.
5. Test on the target console before marking any release as usable.
