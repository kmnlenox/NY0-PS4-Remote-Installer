# Build and test status

## Latest completed native build

- GitHub Actions run: [#54 — successful build and header verification](https://github.com/kmnlenox/NY0-PS4-Remote-Installer/actions/runs/41252005156)
- Artifact: [ny0-remote-installer-fpkg](https://github.com/kmnlenox/NY0-PS4-Remote-Installer/actions/runs/41252005156/artifacts/13061204109)
- PKG SHA-256: `e5810569238870f202e895848eba2de74cd0061df0db191b1cc490d8ad015db3`
- PKG size: 6,619,136 bytes
- The C++ executable compiled and linked; OpenOrbis generated the fSELF/PKG; `PkgTool.Core pkg_validate` passed, including PKG header and Debug RIF signature checks.
- The independent PKG-header test passed: Content ID `IV0000-NY0R00001_00-NY0REMOTEINSTALL`, content type `0x1A` (PS4GD), size `6,619,136`, and a non-empty package digest. These are the fields the runtime BGFT manifest builder reads.

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
- [x] Dedicated PKG-header verification script passes (run #54)
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
