# distributed_compiler

A simple distributed compiler experiment written in **C**.

## What it does
- Sends compile requests from a client to a server
- Runs the compilation remotely
- Returns build output/errors back to the client

## Requirements
- Linux/macOS
- `gcc` (or another C compiler)
- `make`

## Build
```sh
make
```

## Run (example)
> Adjust commands/args to match your files and binaries.

1. Start the server:
```sh
./server
```

2. In another terminal, run the client:
```sh
./client
```

## Project layout
- `src/` — C source code
- `Makefile` — build rules
- `scripts/` — helper scripts (if present)

## Notes
This is a learning project and is still a work in progress.

## License
Add a license if you plan to share or reuse this code.
