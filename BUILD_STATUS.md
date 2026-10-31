# Build and test status

## Latest completed native build

- GitHub Actions run: [#52 — successful build](https://github.com/kmnlenox/NY0-PS4-Remote-Installer/actions/runs/41251916332)
- Artifact: [ny0-remote-installer-fpkg](https://github.com/kmnlenox/NY0-PS4-Remote-Installer/actions/runs/41251916332/artifacts/13061089249)
- PKG SHA-256: `1b7f7d71a9331c8111dd55df05f62f6c2fb46e56d692006a2fa05e3a3268cfe6`
- PKG size: approximately 6.4 MB
- The C++ executable compiled and linked; OpenOrbis generated the fSELF/PKG; `PkgTool.Core pkg_validate` passed, including the PKG header signature and Debug RIF signature checks.

## Implemented in source

- [x] PS4 HTTP server on TCP 12800
- [x] Safari static UI routes
- [x] Chunked upload endpoint with filename/path validation
- [x] HTTP byte-range delivery endpoint for uploaded PKGs
- [x] PKG header inspection: Content ID, content type, package size and digest
- [x] BGFT reference-manifest generation matching the Remote PKG Installer protocol
- [x] BGFT task registration/start and progress endpoint
- [x] Patch-aware BGFT registration path
- [x] SFO metadata aligned to the established homebrew convention (`CATEGORY=gd`, `ATTRIBUTE=0`, standard paid ID; no custom authinfo blob)

## Verification boundary

- [x] Native source compiled and linked (run #52)
- [x] PKG passes OpenOrbis package validation (run #52)
- [ ] Dedicated PKG-header verification script passes (added to the next CI run)
- [ ] Package installs on the user's PS4
- [ ] App launches and reports BGFT ready
- [ ] Safari can upload a real PKG
- [ ] PS4 BGFT task downloads and installs the uploaded PKG

A GitHub Actions success proves compilation and package validation only. It does not prove the PS4 will accept, launch, or run the app. Do not describe the installer as console-tested until the hardware checks are completed.

## Known constraints

- Uploads are single-file `.pkg` packages; split-package sets are not implemented.
- Keep NY0 running while BGFT fetches the uploaded file from the app's local HTTP server.
- The final install result is reported by the PS4 system notification; the web UI reports BGFT transfer progress, not a fabricated success state.
- The service is intended for a trusted LAN or private VPN. Do not expose port 12800 directly to the public internet.
