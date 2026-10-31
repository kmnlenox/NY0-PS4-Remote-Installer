# Protocol and security notes

## Why the PS4 IP and GoldHEN ports are not enough

The FTP service on TCP 2121 speaks FTP, not HTTP. iOS Safari does not provide an FTP client interface. BinLoader on TCP 9090 is a raw TCP payload receiver, not a browser upload endpoint. A normal Safari page cannot open arbitrary raw TCP sockets.

## Required architecture

- A PS4-side HTTP service bound to the console's LAN interface.
- A bounded multipart or streaming upload endpoint with free-space checks.
- A unique upload identifier and server-reported byte counts.
- A verified installation adapter that invokes a supported GoldHEN/package-install mechanism.
- Separate transfer and installation states.
- A status endpoint that reports the real result, including errors.

## Security requirements

- Bind to the LAN interface only; never expose the service to WAN.
- Validate package extension, size, free space, and upload integrity.
- Use generated filenames rather than trusting client paths.
- Prevent path traversal and concurrent writes to the same target.
- Do not invoke a shell with user-supplied data.
- Do not allow arbitrary payload execution.
- Do not claim installation success until the console-side installer confirms it.
- Provide cancellation and cleanup of incomplete uploads.

## Compatibility gate

The user's reported firmware and GoldHEN version must be confirmed on-console. Upstream release notes document Remote Package Install, FTP, BinLoader, and internal package support, but do not by themselves define a stable HTTP API for this project.
