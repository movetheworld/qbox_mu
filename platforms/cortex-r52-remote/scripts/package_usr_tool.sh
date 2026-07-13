#!/usr/bin/env bash
# Package Cortex-R52 VP runtime (no source) into usr_tool/cortex-r52/
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD="${ROOT}/build"
PKG="${ROOT}/usr_tool/cortex-r52"

if [[ ! -x "${BUILD}/cortex-r52-vp" || ! -x "${BUILD}/remote_cpu_r52" ]]; then
    echo "error: build cortex-r52-vp and remote_cpu_r52 first:" >&2
    echo "  cmake -B build && cmake --build build --target cortex-r52-vp remote_cpu_r52" >&2
    exit 1
fi

echo "packaging into ${PKG} ..."
rm -rf "${PKG}"
mkdir -p "${PKG}/fw/cortex-r52" "${PKG}/lib/libqemu"

copy_file() {
    local src="$1" dst="$2"
    if [[ ! -f "${src}" ]]; then
        echo "error: missing ${src}" >&2
        exit 1
    fi
    cp -a "${src}" "${dst}"
}

# binaries
copy_file "${BUILD}/cortex-r52-vp" "${PKG}/cortex-r52-vp"
copy_file "${BUILD}/remote_cpu_r52" "${PKG}/remote_cpu_r52"

# platform config + firmware (no C/asm source)
copy_file "${ROOT}/platforms/cortex-r52-remote/conf.lua" "${PKG}/conf.lua"
copy_file "${ROOT}/platforms/cortex-r52-remote/fw/cortex-r52/cortex-r52.bin" "${PKG}/fw/cortex-r52/cortex-r52.bin"
if [[ -f "${ROOT}/platforms/cortex-r52-remote/fw/cortex-r52/cortex-r52.elf" ]]; then
    copy_file "${ROOT}/platforms/cortex-r52-remote/fw/cortex-r52/cortex-r52.elf" "${PKG}/fw/cortex-r52/cortex-r52.elf"
fi

# qbox / systemc runtime libraries
copy_file "${BUILD}/libqbox.so" "${PKG}/lib/libqbox.so"
copy_file "${BUILD}/liblua.so" "${PKG}/lib/liblua.so"
copy_file "${BUILD}/_deps/report-build/libreporting.so" "${PKG}/lib/libreporting.so"
copy_file "${BUILD}/_deps/rpclib-build/librpc.so" "${PKG}/lib/librpc.so"
copy_file "${BUILD}/_deps/systemccci-build/configuration/src/libcci-config.so.1.0" "${PKG}/lib/libcci-config.so.1.0"
copy_file "${BUILD}/_deps/systemclanguage-build/src/libsystemc.so.3.0" "${PKG}/lib/libsystemc.so.3.0"

# platform plugin modules (loaded by dylib_path)
for plug in uart-pl011 arm_gicv3 cpu_arm_cortexR52; do
    copy_file "${BUILD}/${plug}.so" "${PKG}/${plug}.so"
done

# libqemu (bundled QEMU backend)
QEMU_LIB="${BUILD}/_deps/libqemu-build/qemu-prefix/lib"
if [[ -d "${QEMU_LIB}" ]]; then
    cp -a "${QEMU_LIB}/"*.so "${PKG}/lib/libqemu/" 2>/dev/null || true
fi
if [[ ! -f "${PKG}/lib/libqemu/libqemu-system-aarch64.so" ]]; then
    echo "error: libqemu-system-aarch64.so not found under ${QEMU_LIB}" >&2
    exit 1
fi

cat > "${PKG}/run.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:${DIR}/lib:${DIR}/lib/libqemu:${DIR}"
exec "${DIR}/cortex-r52-vp" --gs_luafile "${DIR}/conf.lua" "$@"
EOF

cat > "${PKG}/run_gdb.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:${DIR}/lib:${DIR}/lib/libqemu:${DIR}"
exec "${DIR}/cortex-r52-vp" --gs_luafile "${DIR}/conf.lua" \
    --param 'platform.plugin_0.remote_argv.2="--param"' \
    --param 'platform.plugin_0.remote_argv.3=remote_platform.cpu_0.gdb_port=4321' \
    "$@"
EOF

chmod +x "${PKG}/run.sh" "${PKG}/run_gdb.sh" "${PKG}/cortex-r52-vp" "${PKG}/remote_cpu_r52"

cat > "${PKG}/README.md" <<'EOF'
# Cortex-R52 Virtual Platform (Runtime Package)

Prebuilt runtime for the ARM Cortex-R52 remote VP. No source code is included.

## Contents

| Path | Description |
|------|-------------|
| `cortex-r52-vp` | Main virtual platform executable |
| `remote_cpu_r52` | Remote CPU process (spawned automatically) |
| `conf.lua` | Platform configuration |
| `fw/cortex-r52/cortex-r52.bin` | Prebuilt firmware image |
| `fw/cortex-r52/cortex-r52.elf` | Firmware ELF (for GDB symbols) |
| `*.so` | Platform plugin modules |
| `lib/` | Qbox / SystemC / CCI runtime libraries |
| `lib/libqemu/` | Bundled QEMU backend |

## Requirements

- Linux x86_64 or WSL2 (Ubuntu 22.04+ recommended)
- Standard system libraries: `libstdc++`, `libfmt9`, `libz`, `glib`, etc.

## Run

```bash
cd usr_tool/cortex-r52
./run.sh
```

Expected output:

```
hello world
Test program is running. Listening for interrupts.
SPI 33 happened
```

## Debug with gdb-multiarch

**Terminal 1** — start VP and wait for GDB:

```bash
./run_gdb.sh
```

**Terminal 2** — attach and stop at `c_entry`:

```bash
gdb-multiarch fw/cortex-r52/cortex-r52.elf
(gdb) set architecture arm
(gdb) target remote localhost:4321
(gdb) break c_entry
(gdb) continue
```

## Notes

- Always run from this directory (`run.sh` sets `LD_LIBRARY_PATH` automatically).
- `remote_cpu_r52` must stay alongside `cortex-r52-vp` (same folder).
- To use a different firmware image, replace `fw/cortex-r52/cortex-r52.bin`.
EOF

echo "done: ${PKG}"
du -sh "${PKG}"
