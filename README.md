---

# u64-remote-cpp

A Linux **C++ command-line client** for interacting with the **Commodore 64 Ultimate** over its REST API.

This project is a C++ port of the original Go reference implementation and allows remote control and inspection of a C64 Ultimate from a modern Linux system.

---

## Features

* Upload and run `.prg` files remotely
* Query device version and status
* Optional memory peek/poke via REST endpoints
* Automatic device discovery on the local network (mDNS / Avahi)
* Simple credential handling via JSON

---

## How it works (high level)

1. Loads credentials (address + password) from `creds.json`
2. Discovers the C64 Ultimate on the LAN if no address is provided
3. Establishes a REST client using `libcurl`
4. Sends authenticated API requests using the `X-Password` header
5. Uploads and executes PRG files on the device

---

## Build

### Dependencies (Pop!_OS / Ubuntu)

```bash
sudo apt install -y cmake pkg-config \
  libavahi-client-dev libavahi-common-dev \
  libcurl4-openssl-dev
```

### Build steps

```bash
git clone https://github.com/<your-username>/u64-remote-cpp.git
cd u64-remote-cpp
cmake -S . -B build
cmake --build build -j
```

The executable will be located at:

```bash
build/u64-remote
```

---

## Configure credentials

Copy the example file and edit as needed:

```bash
cp creds.json.example creds.json
nano creds.json
```

You may also pass a credentials file explicitly:

```bash
./build/u64-remote --creds /path/to/creds.json myprog.prg
```

---

## Device discovery

If no address is provided (or `--discover` is specified), the program will:

* Discover C64 Ultimate devices via mDNS (Avahi)
* Resolve hostname, IP address, and port
* Prompt if multiple devices are found

Example:

```bash
./build/u64-remote --discover myprog.prg
```

---

## Usage

```bash
u64-remote \
  [--creds /path/to/creds.json] \
  [--address http://10.0.0.183] \
  [--password XXXXX] \
  [--discover] \
  file.prg
```

**Notes:**

* `address` must include the scheme (`http://`)
* Passwords are sent via the `X-Password` HTTP header
* Do **not** commit real credentials to GitHub

---

## Attribution

This project is a C++ port of the original Go implementation:

* **Original project:** [https://github.com/AllMeatball/u64-remote](https://github.com/AllMeatball/u64-remote)
* **Original GO Code author:** Levi Spencer
* **License:** MIT

All original architectural credit belongs to the upstream author.
