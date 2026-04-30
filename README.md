# distributed_compiler

A distributed compilation experiment written in **C**. The goal is to compile source files on a remote machine (server) from a local machine (client), and send the build output back to the client.

> Status: learning/WIP project.

## Features
- Remote compilation: client sends a file (or a compile request) to the server
- Server runs the compilation command and captures output
- Client receives compiler output (stdout/stderr) and a success/failure status
- Client can enter an **admin/interactive mode** when no file is provided (per current implementation)

## How it works (high level)
1. **Client** connects to the server.
2. Client sends the file / request information.
3. **Server** runs the compiler and captures logs.
4. Server returns the result (errors/warnings/output) to the client.

## Requirements
- Linux/macOS (recommended)
- A C compiler on the **server** (and locally if you also build locally)
  - `gcc` or `clang`
- `make`

## Build

```sh
git clone https://github.com/Oct8Hacker/distributed_compiler.git
cd distributed_compiler
make
```

## Usage

### Start the server
```sh
./server
```

### Run the client
Compile a file remotely:
```sh
./client <path-to-file>
```

Run client without a file (admin/interactive mode):
```sh
./client
```

## Configuration
This repo is intentionally simple, so some settings may be hard-coded.

Common things you may want to adjust in code:
- server host / port
- compiler command (`gcc` vs `clang`)
- where received files / build artifacts are stored
- request/response size limits

## Project layout
- `src/` — C source code
- `Makefile` — build rules
- `scripts/` — helper scripts (if present)

## Troubleshooting
- **Client can’t connect**: make sure the server is running and host/port match.
- **Compilation fails on server**: ensure `gcc/clang` is installed on the server and it can write files.
- **Large output gets cut off**: there may be buffer limits; increase them in the networking code.

## Security note
Remote compilation can be dangerous if the server accepts untrusted input. Avoid exposing the server publicly until you add authentication/sandboxing/resource limits.

## License
MIT — see [LICENSE](LICENSE).
