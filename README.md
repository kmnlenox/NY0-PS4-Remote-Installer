# NY0 PS4 Remote Installer

An iPhone Safari-controlled interface for transferring user-owned PKG files to a PS4 on the same local network.

## Project status

**Early development — not ready to install or use.** This milestone establishes a reproducible OpenOrbis build pipeline and separates the Safari frontend from the PS4-side network/installation service. A successful CI build is not proof of console compatibility or package installation.

## Intended workflow

1. Open the PS4's LAN address in iPhone Safari.
2. Choose a PKG from the iOS Files picker.
3. Transfer the file over the local network with visible progress.
4. Request installation through a verified PS4/GoldHEN integration.
5. Display the actual result returned by the console.

## Important protocol distinction

GoldHEN documents FTP on TCP 2121 and a raw BinLoader service on TCP 9090. Neither is an HTTP website that Safari can directly use. The Safari experience therefore needs an HTTP service running on the PS4 or another explicitly supported bridge. The project will not pretend that uploading a file equals installing it.

GoldHEN also documents Remote Package Install and internal package installation support. The exact interface exposed by the user's installed GoldHEN build must be verified before enabling an install button.

## Build

The PS4 diagnostic package is built by GitHub Actions using the OpenOrbis toolchain. Pushes to this branch trigger the workflow. Download the generated PKG from the workflow run's Artifacts.

## Safety

- Only use PKG files you own or are authorized to install.
- Keep the PS4 and iPhone on a trusted private LAN.
- Do not expose a package upload/install service to the public internet.
- Do not send arbitrary payloads to BinLoader.
- Do not treat an unverified firmware/GoldHEN combination as supported.

## Milestones

- [x] Repository and implementation branch
- [ ] Reproducible PS4 diagnostic PKG build
- [ ] PS4 HTTP service and Safari upload endpoint
- [ ] Verified GoldHEN install adapter
- [ ] Console testing on the target firmware and GoldHEN version
- [ ] End-to-end Safari upload and installation progress

See [BUILD_STATUS.md](BUILD_STATUS.md) for the current verification boundary.
