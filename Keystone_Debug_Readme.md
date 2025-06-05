# Keystone Debugging with GDB and PMP Inspection

This guide walks through a successful Keystone QEMU debug session using GDB, including PMP inspection using the `pmp.py` script.

---

## 1. Clean and Rebuild Keystone

```bash
make BUILDROOT_TARGET=keystone-examples-dirclean
make -j$(nproc)
```

---

## 2. Launch QEMU in Debug Mode

```bash
KEYSTONE_DEBUG=1 make run
```
This will start QEMU and pause waiting for a GDB connection on port `:9822`.

---

## 3. Connect to GDB in a New Terminal

```bash
riscv64-unknown-elf-gdb keystone/build-generic64/buildroot.build/build/keystone-examples-ab4b2a87905fc4d2/hello/hello
```

GDB Output:
```
GNU gdb (GDB) 15.2
...
warning: A handler for the OS ABI "GNU/Linux" is not built into this configuration of GDB.  
Attempting to continue with the default riscv:rv64 settings.
```

Then connect to QEMU:

```gdb
(gdb) target remote :9822
```
You should see:
```
Remote debugging using :9822
0x0000000000001000 in ?? ()
```

---

## 4. Source the Keystone PMP Helper Script

```gdb
(gdb) source keystone/scripts/gdb/pmp.py
```
You might see:
```
-0x7fffffffffebee53
```
(This is harmless.)

Then inspect the PMP registers:

```gdb
(gdb) pmp-dump
```

---

## 5. Set a Breakpoint and Continue

```gdb
(gdb) break main
(gdb) continue
```

---

## 6. In QEMU Terminal (Terminal 1)

### Login
```sh
buildroot login: root
Password: sifive
```

### Load the Keystone Driver and Run the Enclave
```sh
modprobe keystone-driver
/usr/share/keystone/examples/hello.ke
```
You will see:
```
Verifying archive integrity... MD5 checksums are OK. All good.
Uncompressing Keystone Enclave Package
```

---

## 7. Back in GDB Terminal (Terminal 2)

GDB will show:
```
[Switching to Thread 1.3]
Thread 3 hit Breakpoint 1, 0x00000000000105a8 in main ()
```

Resource the PMP script again (optional):
```gdb
(gdb) source keystone/scripts/gdb/pmp.py
```

Then inspect PMP config:
```gdb
(gdb) pmp-dump
```

Example output:
```
PMP reg 0 NAPOT
	cfg	0x18 
	addr	0x2003ffff = 0x80000000 -> 0x80200000
PMP reg 1 NAPOT
	cfg	0x1f RWX
	addr	0x20cbffff = 0x83200000 -> 0x83400000
PMP reg 2 NAPOT
	cfg	0x1f RWX
	addr	0x0 = 0x0 -> 0x8
PMP reg 7 NAPOT
	cfg	0x1f RWX
	addr	0x20c77fff = 0x831c0000 -> 0x83200000
```

### Explanation of PMP Dump

| PMP Entry | Mode | Config | Address Range | Meaning |
|-----------|------|--------|----------------|---------|
| **reg 0** | NAPOT | `0x18` (RWX=000) | `0x80000000 -> 0x80200000` (2 MiB) | Protected memory for the **security monitor** (SM). M-mode only; S/U cannot access. |
| **reg 1** | NAPOT | `0x1f` (RWX=111) | `0x83200000 -> 0x83400000` (2 MiB) | EPM (Enclave Physical Memory) for an active enclave. S-mode allowed to read/write/execute. |
| **reg 2** | NAPOT | `0x1f` (RWX=111) | `0x0 -> 0x8` (8 bytes) | Guard region to catch null-pointer dereferences. Smallest valid NAPOT region. |
| **reg 7** | NAPOT | `0x1f` (RWX=111) | `0x831c0000 -> 0x83200000` (256 KiB) | UTM (Untrusted Memory) shared between host and enclave. Fully accessible to S-mode. |

#### Notes:
- NAPOT = Naturally Aligned Power-of-Two region.
- PMP entries are matched in order; first match wins.
- `pmp-clear` can be used to zero all PMP CSRs.
- The `cfg` byte: `0x1f` = RWX enabled, NAPOT mode, not locked.
- Each `addr` is decoded to show the base and top (exclusive) of the region it protects.

---

## Notes
- `pmp-dump` gives you visibility into memory protection settings.
- PMP entries are ordered: first match wins.
- Use `pmp-clear` if you want to temporarily disable all PMP regions during debugging.

This setup enables effective Keystone debugging and enclave memory protection inspection in QEMU using GDB, but however it stills on progress..
