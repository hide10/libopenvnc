# libopenvnc

A cross-platform VNC (RFB protocol) library written in C.

## Overview

libopenvnc is an implementation of the Remote Framebuffer (RFB) protocol as defined in [RFC 6143](https://datatracker.ietf.org/doc/html/rfc6143). It provides both client and server functionality with a permissive MIT license.

### Why not use existing libraries?

The well-known [LibVNCServer/LibVNCClient](https://github.com/LibVNC/libvncserver) is GPL v2 licensed, which restricts commercial and proprietary use. libopenvnc aims to provide equivalent functionality under the MIT license.

## Project Goals

- **RFC 6143 full compliance**: Support all features defined in the specification
- **RFB versions**: 3.3, 3.7, 3.8
- **Client and Server**: Client is the primary focus, server will also be implemented
- **Cross-platform**: Windows first, then Linux and other platforms
- **Permissive license**: MIT license for unrestricted use

## Architecture (Planned)

```
┌─────────────────────────────┐
│  Application Layer          │  ← Recording app, viewer, etc.
├─────────────────────────────┤
│  Client API / Server API    │  ← Public API
├─────────────────────────────┤
│  RFB Protocol Core          │  ← Encodings, auth, message handling
├─────────────────────────────┤
│  Transport (TCP/TLS)        │  ← Network abstraction
├─────────────────────────────┤
│  Platform Abstraction       │  ← OS-specific code (Win/Linux)
└─────────────────────────────┘
```

## RFC 6143 Scope

### Encodings
- Raw (0), CopyRect (1), RRE (2), Hextile (5), TRLE (15), ZRLE (16)
- Pseudo-encodings: Cursor (-239), DesktopSize (-223)

### Security Types
- None (1), VNC Authentication (2)

### Client-to-Server Messages
- SetPixelFormat, SetEncodings, FramebufferUpdateRequest, KeyEvent, PointerEvent, ClientCutText

### Server-to-Client Messages
- FramebufferUpdate, SetColourMapEntries, Bell, ServerCutText

## Roadmap

1. **Phase 1**: RFC 6143 specification analysis and detailed design
2. **Phase 2**: Core protocol implementation (handshake, message types)
3. **Phase 3**: Client implementation (connect, receive framebuffer, input events)
4. **Phase 4**: Encoding implementations (Raw → CopyRect → RRE → Hextile → TRLE → ZRLE)
5. **Phase 5**: Security (VNC Authentication)
6. **Phase 6**: Server implementation
7. **Phase 7**: Platform-specific optimization and testing

Current status: **Phase 1 - Not yet started**

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Requires: CMake 3.15+, C11 compiler

## License

MIT License. See [LICENSE](LICENSE) for details.
