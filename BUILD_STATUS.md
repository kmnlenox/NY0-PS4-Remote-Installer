# Build and test status

## Implemented in source

- [x] PS4 HTTP server on TCP 12800
- [x] Safari static UI routes
- [x] Chunked upload endpoint with filename/path validation
- [x] HTTP byte-range delivery endpoint for uploaded PKGs
- [x] BGFT service symbol loading, task registration/start, and progress endpoint
- [x] SFO metadata aligned to the OpenOrbis Remote Package Installer convention (`CATEGORY=gd`, `ATTRIBUTE=0`, standard paid ID; no custom authinfo blob)

## Verification boundary

- [ ] Latest source compiles successfully
- [ ] PKG passes OpenOrbis validation
- [ ] Package installs on the user's PS4
- [ ] App launches and reports BGFT ready
- [ ] Safari can upload a real PKG
- [ ] PS4 BGFT task downloads and installs the uploaded PKG

A GitHub Actions success only proves the build and PKG validation steps. It does not prove runtime compatibility. Do not describe the installer as console-tested until the final four checks are completed on hardware.

## Known constraints

- Upload filenames must begin with the 36-character PS4 Content ID; the app uses this to register the BGFT task.
- Keep NY0 running while the PS4 download manager fetches the uploaded file from the app's local HTTP server.
- The service is intended for a trusted LAN or private VPN. Do not expose it to the public internet.
- The final install result is reported by the PS4 system notification; the web UI reports BGFT transfer progress, not a fabricated success state.
