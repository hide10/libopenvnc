# AI Development Guide for libopenvnc

This document provides context for AI assistants working on this project.

## Project Summary

VNC (RFB protocol) library in C, MIT licensed. The owner's end goal is to build a VNC client that can record the remote screen. The library itself does NOT include recording functionality — that will be built as a separate application on top of the library.

## Key Decisions Made

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Language | C | AI-driven development, maximum compatibility/FFI |
| License | MIT | GPL (used by LibVNCServer) prevents commercial use |
| Build system | CMake | Standard for cross-platform C libraries |
| Spec | RFC 6143 | Official RFB protocol specification |
| Scope | Full RFC 6143 | All encodings, security types, message types, RFB 3.3/3.7/3.8 |
| Client priority | Client first | Owner's immediate need is a VNC client for screen recording |
| Server | Will implement | Public library, so server functionality is also needed |
| Platform | Windows first | Base layer cross-platform (Linux etc.) |
| Recording | Not in library | Application-level concern, not library scope |

## Current Phase

**Phase 1: RFC 6143 specification analysis**

The RFC has NOT been read/analyzed yet. This is the first task to complete.

### What needs to happen in Phase 1:
1. Read RFC 6143 thoroughly
2. Document the protocol in detail (handshake flow, message formats, encoding formats, security mechanisms)
3. Document differences between RFB 3.3, 3.7, and 3.8
4. Create a detailed library design (module structure, public API, data structures)
5. Commit documentation to the repo (in docs/ directory)

## RFB Protocol Quick Reference

- **RFB** = Remote Framebuffer Protocol (the protocol VNC uses)
- **RFC 6143** = https://datatracker.ietf.org/doc/html/rfc6143
- RFB versions: 3.3 (original), 3.7 (security negotiation), 3.8 (error handling, standardized in RFC)
- Client-pull model: client must request framebuffer updates; server does not push automatically
- Handshake: version negotiation → security type negotiation → authentication → initialization

## Reference: LibVNCServer (existing GPL library)

Investigated and rejected due to GPL v2 license. However, useful design insights:
- Callback-based API design works well (MallocFrameBuffer, GotFrameBufferUpdate, FinishedFrameBufferUpdate)
- `client->frameBuffer` direct pointer access pattern is efficient for recording
- They have a `vnc2mpg.c` example that records VNC to MP4 via FFmpeg
- RFB is pull-based: continuous `SendFramebufferUpdateRequest` is needed for recording

## Architecture Plan

```
┌─────────────────────────────┐
│  Application Layer          │  ← Not part of library
├─────────────────────────────┤
│  Client API / Server API    │  ← Public API (ovnc_client.h / ovnc_server.h)
├─────────────────────────────┤
│  RFB Protocol Core          │  ← Encodings, auth, message parsing
├─────────────────────────────┤
│  Transport (TCP/TLS)        │  ← Network I/O abstraction
├─────────────────────────────┤
│  Platform Abstraction       │  ← Win32/POSIX socket, threading, etc.
└─────────────────────────────┘
```

## Code Conventions (TBD)

- C11 standard
- Naming convention: to be decided in design phase (likely `ovnc_` prefix)
- Error handling pattern: to be decided

## File Structure (Current)

```
libopenvnc/
├── .gitignore
├── CMakeLists.txt
├── LICENSE          (MIT)
├── README.md        (project overview and roadmap)
└── CLAUDE.md        (this file)
```

## Next Steps

1. Read and analyze RFC 6143
2. Create docs/ directory with specification notes
3. Design public API
4. Begin implementation (Phase 2)
