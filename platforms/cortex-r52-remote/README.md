# Virtual Platform of ARM Cortex-R52 (Remote CPU + GICv3)

## Overview

This platform mirrors `cortex-m55-remote`: the Cortex-R52 CPU and GICv3 run in a remote process connected to the main VP via TLM RemotePass.

Peripherals on the main side:

- **RAM / firmware** at `0x00000000`
- **GICv3** (remote) at `0x2F000000` (GICD) and `0x2F100000` (GICR)
- **PL011 UART** at `0xC0000000` (GIC SPI 0, INTID 32)
- **IRQ test block** at `0xC0001000` (GIC SPI 1, INTID 33)

See **`addr-map.xlsx`** for the full memory map used by embedded software.

## Build

From the repository root:

```bash
cmake -B build
cmake --build build --target cortex-r52-vp remote_cpu_r52
```

## Build firmware

Requires `arm-none-eabi-gcc`:

```bash
make -C platforms/cortex-r52-remote/fw/cortex-r52
```

## Run

Simulation must be started from the **`build/`** directory (not the repository root), so that `executable_path` resolves correctly for the remote CPU process.

### Linux / WSL

Build firmware first, then run the VP:

```bash
cd build

export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:$(pwd)/_deps/systemclanguage-build/src:$(pwd)/_deps/systemccci-build/configuration/src:$(pwd)/_deps/report-build:$(pwd)/_deps/fmt-build:$(pwd)/_deps/libqemu-build/qemu-prefix/lib:$(pwd)"

make -C ../platforms/cortex-r52-remote/fw/cortex-r52

./cortex-r52-vp --gs_luafile ../platforms/cortex-r52-remote/conf.lua
```

On **Windows**, use **WSL** and the same commands with paths under `/mnt/d/...` (adjust the drive letter to match your checkout).

Optional: increase log verbosity:

```bash
./cortex-r52-vp --gs_luafile ../platforms/cortex-r52-remote/conf.lua --param log_level=3
```

Expected output includes:

```
hello world
Test program is running. Listening for interrupts.
SPI 33 happened
```

Press **Ctrl+C** or **Ctrl+\\** to stop the simulation.

## Debugging with GDB

The Cortex-R52 CPU runs in the **remote** process (`remote_cpu_r52`). Qbox
starts a GDB server on that CPU when `gdb_port` is set. The simulation
pauses at CPU reset until a debugger connects.

> **Important:** On this platform, `--param platform.plugin_0.cpu_0.gdb_port=4321`
> on the main VP does **not** reach the remote CPU. Pass the port to
> `remote_cpu_r52` via `remote_argv` (see below).

### Start VP and wait for GDB (from `build/`)

```bash
cd build
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:$(pwd)/_deps/systemclanguage-build/src:$(pwd)/_deps/systemccci-build/configuration/src:$(pwd)/_deps/report-build:$(pwd)/_deps/fmt-build:$(pwd)/_deps/libqemu-build/qemu-prefix/lib:$(pwd)"

./cortex-r52-vp --gs_luafile ../platforms/cortex-r52-remote/conf.lua \
    --param 'platform.plugin_0.remote_argv.2="--param"' \
    --param 'platform.plugin_0.remote_argv.3=remote_platform.cpu_0.gdb_port=4321'
```

From **`usr_tool/cortex-r52/`** (or any directory where `cortex-r52-vp`,
`remote_cpu_r52`, and `conf.lua` sit together), after setting `LD_LIBRARY_PATH`
(see `run.sh` / `run_gdb.sh`):

```bash
cd usr_tool/cortex-r52
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:$(pwd)/lib:$(pwd)/lib/libqemu:$(pwd)"

./cortex-r52-vp --gs_luafile ./conf.lua \
    --param 'platform.plugin_0.remote_argv.2="--param"' \
    --param 'platform.plugin_0.remote_argv.3=remote_platform.cpu_0.gdb_port=4321'
```

Or use the wrapper script: `./run_gdb.sh` (same effect).

The quotes around `"--param"` are required (otherwise CCI JSON parsing fails).

**Success indicators:**

- Log shows `Setting param remote_platform.cpu_0.gdb_port to value 4321`
- No `hello world` yet — VP is waiting for GDB
- Port 4321 is listening (`ss -tln | grep 4321`)

If you see `hello world` immediately, the GDB port was not applied. Stop any
old VP (`pkill -f cortex-r52-vp`) and retry with the command above.

`gdb_port` pauses at **reset**, not at `c_entry`. Set a breakpoint after
connecting (see below).

### Build firmware with debug symbols

For source-level breakpoints and single-stepping, rebuild with `-g -O0`.
Either edit `fw/cortex-r52/Makefile` temporarily:

```makefile
CFLAGS=-nostdinc -nostdlib -mcpu=cortex-r52 -g -O0 -marm -I.
```

Or compile manually:

```bash
make -C platforms/cortex-r52-remote/fw/cortex-r52 clean
make -C platforms/cortex-r52-remote/fw/cortex-r52 \
    CC="arm-none-eabi-gcc" CFLAGS="-nostdinc -nostdlib -mcpu=cortex-r52 -g -O0 -marm -I."
```

### Connect with gdb-multiarch (two terminals)

Firmware is 32-bit ARM (`-marm`). Use `gdb-multiarch` if `arm-none-eabi-gdb`
is not installed:

```bash
sudo apt install gdb-multiarch
```

**Terminal 1** — start the VP (command from previous section).

**Terminal 2** — attach and stop at `c_entry`:

```bash
gdb-multiarch ../platforms/cortex-r52-remote/fw/cortex-r52/cortex-r52.elf
(gdb) set architecture arm
(gdb) target remote localhost:4321
(gdb) break c_entry
(gdb) continue
```

One-liner equivalent:

```bash
gdb-multiarch -q ../platforms/cortex-r52-remote/fw/cortex-r52/cortex-r52.elf \
    -ex 'set architecture arm' \
    -ex 'target remote localhost:4321' \
    -ex 'break c_entry' \
    -ex 'continue'
```

When the breakpoint hits, GDB stops in `c_entry()` before any UART output.

If you have the full toolchain, `arm-none-eabi-gdb` works the same way
(substitute for `gdb-multiarch` above).

### Single-step debugging

Once stopped at a breakpoint:

| Command | Alias | Description |
|---------|-------|-------------|
| `step` | `s` | Step **into** the next source line |
| `next` | `n` | Step **over** the next source line |
| `stepi` | `si` | Step one **machine instruction** |
| `nexti` | `ni` | Step one instruction, skipping over `bl` calls |
| `continue` | `c` | Run until the next breakpoint |
| `finish` | | Run until the current function returns |

Example session:

```text
(gdb) break c_entry
(gdb) continue
Breakpoint 1, c_entry () at main.c:41

(gdb) next
(gdb) next
(gdb) stepi
(gdb) info registers
(gdb) x/8i $pc
(gdb) x/4xw 0x2F000000       # GICD
(gdb) x/4xw 0xC0000000       # UART
(gdb) continue
```

Useful commands while stopped:

```text
(gdb) list                    # source around current line
(gdb) info breakpoints
(gdb) bt                      # backtrace
(gdb) break handle_irq        # break in IRQ handler
```

## Regenerate address map Excel

```bash
python3 platforms/cortex-r52-remote/scripts/gen_addr_map.py
```

Constants are also in `fw/cortex-r52/addr_map.h` and must stay in sync with `conf.lua`.

## Runtime package (no source, for end users)

To bundle the VP binaries, libraries, config, and firmware into `usr_tool/cortex-r52/`
(without C/asm source):

```bash
cmake -B build
cmake --build build --target cortex-r52-vp remote_cpu_r52
make -C platforms/cortex-r52-remote/fw/cortex-r52

platforms/cortex-r52-remote/scripts/package_usr_tool.sh
```

Distribute the `usr_tool/cortex-r52/` folder. End users run:

```bash
cd usr_tool/cortex-r52
./run.sh          # normal simulation
./run_gdb.sh      # wait for GDB on port 4321
```

GDB debug (equivalent to `./run_gdb.sh`):

```bash
cd usr_tool/cortex-r52
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:$(pwd)/lib:$(pwd)/lib/libqemu:$(pwd)"

./cortex-r52-vp --gs_luafile ./conf.lua \
    --param 'platform.plugin_0.remote_argv.2="--param"' \
    --param 'platform.plugin_0.remote_argv.3=remote_platform.cpu_0.gdb_port=4321'
```

See `usr_tool/cortex-r52/README.md` in the generated package for details.
