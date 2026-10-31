# NY0 PS4 Remote Installer

An iPhone Safari-controlled homebrew installer for user-owned PS4 PKG files over a trusted local network.

## Current implementation

- PS4-side HTTP service on TCP port 12800.
- Safari web interface served by the PS4 application.
- Chunked upload from iPhone Safari to `/data/ny0_uploads`.
- HTTP package delivery with byte-range support for the PS4 download manager.
- BGFT task registration/start and progress reporting through the PS4 system service.
- PKG header inspection to extract Content ID, package type, size and digest.
- BGFT reference-manifest generation matching the Remote PKG Installer protocol.

## Use

1. Install the NY0 fake-signed homebrew PKG through GoldHEN's Package Installer.
2. Launch **NY0 Remote Installer** and leave it open.
3. On the iPhone, open `http://PS4-IP:12800` in Safari while on the same trusted LAN.
4. Select a single `.pkg` file, then choose **Upload & Install**. NY0 reads its Content ID and package metadata from the PKG header; the filename does not need a special format.
5. Keep the app open until the PS4 download task has received the package. The PS4 system notification is the authority for the final installation result.

The current upload path supports single-file PKGs up to 4 GB; split-package sets are not implemented. The app is designed for private LAN use. Do not expose port 12800 directly to the public internet. For remote access, use a private VPN/subnet-router arrangement rather than public port forwarding.

## Build and verification

GitHub Actions builds a fake-signed homebrew PKG using the OpenOrbis toolchain. A successful CI build verifies compilation and package integrity only; it does not prove the PS4 will accept, launch, or run the app. The BGFT integration and full Safari upload/install flow still require console testing on the target firmware/GoldHEN combination.

## References and attribution

The BGFT task flow and PS4-side HTTP server behavior were researched against flatz's Remote Package Installer and the OpenOrbis port by Backporter. NY0's current implementation is a separate implementation; no third-party binary payload is bundled.

- https://github.com/flatz/ps4_remote_pkg_installer
- https://github.com/Backporter/ps4_remote_pkg_installer-OOSDK
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain

## Safety

Only install PKG files you own or are authorized to use. Keep the service on a trusted private network. GoldHEN's BinLoader is not used by this application.
