<<<<<<< HEAD
# c64u-cpp_remote
=======
# u64-remote-cpp

A small Linux C++ CLI that talks to a **Commodore 64 Ultimate** over its REST API:
- Run a `.prg` remotely
- (Optional) Peek / poke memory via API endpoints

This is a C++ port of the Go example you provided.

## What the original Go code does (in plain English)

1. Loads credentials (IP/URL + password) from a `creds.json`.
2. Creates an HTTP client wrapper (`U64Server`) that always sends `X-Password: <password>`.
3. Tests connectivity by calling `GET /v1/version`.
4. Uploads a PRG file with `POST /v1/runners:run_prg` as raw bytes (`application/octet-stream`).

It also has helper calls for:
- `GET /v1/machine:readmem?address=....&length=...` (peek memory)
- `POST /v1/machine:writemem?address=....` (poke memory)

## Build

```bash
cd u64-remote-cpp
mkdir -p build
cd build
cmake ..
make -j
```

## Configure credentials

Copy and edit:

```bash
cp creds.json.example creds.json
nano creds.json
```

Or keep your credentials in your existing folder and pass `--creds`:

```bash
./u64-remote --creds /home/eric/development/c++/u64-remote/json_examples/creds.json myprog.prg
```

## Discovery (auto-find your C64U on the LAN)

If `address` is empty in creds (or you pass `--discover`), the program will:
- Detect your local IPv4 interface(s)
- Scan the local subnet (defaults to /24 style ranges)
- Probe `http://<ip>/v1/version` with a short timeout
- If one device is found, it uses it
- If multiple are found, it lists them and asks you to pick

Example:

```bash
./u64-remote --discover myprog.prg
```

## Usage

```bash
u64-remote [--creds /path/to/creds.json] [--address http://10.0.0.183] [--password XXXXX] [--discover] file.prg
```

Notes:
- `address` should include scheme, e.g. `http://10.0.0.183`
- The API password goes into `X-Password` header.

>>>>>>> c51b04d (Initial import: u64-remote C++ port (Linux + libcurl + discovery))
