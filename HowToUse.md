# How to Use `u64-remote` (C++)

`u64-remote` is a lightweight command-line tool that sends a **`.prg` file** from your Linux machine to a **Commodore 64 Ultimate / Ultimate-64** over the network and runs it remotely.

It is designed to be fast, scriptable, and developer-friendly.

---

## Prerequisites (One-Time Setup)

### On the Commodore 64 Ultimate

Ensure the following:

- Ultimate firmware supports the REST API
- Network is enabled
- REST password is set (or empty if disabled)
- The device is reachable on your network  
  (example private IP: `10.0.0.250` — **edit to reflect your setup**)

#### Verify connectivity from Linux

```bash
curl http://10.0.0.250/v1/version
