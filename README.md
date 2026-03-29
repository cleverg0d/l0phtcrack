# L0phtCrack 7 — Multiplatform Community Fork

> A community fork of [L0phtCrack 7](https://gitlab.com/l0phtcrack/l0phtcrack) — the classic Windows password auditing tool — rebuilt as a fully cross-platform application with GPU acceleration via **hashcat**.

![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-blue)
![Architecture](https://img.shields.io/badge/arch-ARM64%20%7C%20x86--64-green)
![Engine](https://img.shields.io/badge/engine-hashcat%207.x-orange)
![Version](https://img.shields.io/badge/version-7.3.3-informational)

![L0phtCrack 7 macOS — main window](docs/screenshots/main-window.png)

---

## Platform support

| Platform | Status | Download |
|---|---|---|
| macOS 13+ Apple Silicon (M1/M2/M3/M4) | ✅ Working | [v7.3.3 release](https://github.com/cleverg0d/l0phtcrack/releases/tag/v7.3.3) |
| Linux ARM64 | ✅ Working | [v7.3.3 release](https://github.com/cleverg0d/l0phtcrack/releases/tag/v7.3.3) |
| Linux x86-64 (Intel/AMD) | 🔲 In progress | — |
| Windows x64 (Intel/AMD) | 🔲 In progress | — |

---

## About

L0phtCrack is a classic password auditing and recovery tool, originally created by the legendary hacker collective **L0pht Heavy Industries** and later developed by **L0pht Holdings, LLC**. For years it was the industry standard for Windows password auditing. The original product — available at [gitlab.com/l0phtcrack/l0phtcrack](https://gitlab.com/l0phtcrack/l0phtcrack) — was Windows-only and is no longer actively maintained.

**We are grateful to the original L0phtCrack team** for the years of work put into this product. This fork takes their open-sourced codebase and brings it to all major platforms.

---

## What's different in this fork

### Engine: hashcat instead of John the Ripper

This fork **replaces John the Ripper with [hashcat](https://hashcat.net/hashcat/)**, which:

- Uses **GPU acceleration** — Apple Metal on macOS, OpenCL/CUDA on Linux/Windows
- Finds **2–3× more passwords** in User Info mode compared to the original JtR binary
- Supports all major attack modes: Brute Force, Dictionary, and User Info (single)
- Runs at full GPU speed — NTLM cracking at multi-GH/s on modern hardware

### Finalyze technique

Added the **Finalyze** attack technique, which takes already-cracked passwords and runs them through an additional rule set. This allows recovering more passwords from hashes where a simple variation of a known password was used — a common real-world scenario.

### Statistics section

Full password audit statistics: crack rate by account type, top passwords with frequency analysis, password complexity breakdown, duplicate detection, and CSV export.

### Color scheme support

Switch between dark and light themes in Settings.

### Linux & multiplatform build

- Proper `$ORIGIN`-based RPATH in all `.so` files — no hardcoded build paths
- Bundled wordlists and rules in `resources/` — same source for all platforms
- Single `lc7` binary with plugin architecture — same codebase on all OS

---

## Installation

### macOS Apple Silicon

**Option 1 — Pre-built (recommended)**

1. Download `lc7-v7.3.3-macos-arm64.zip` from [Releases](https://github.com/cleverg0d/l0phtcrack/releases)
2. Unzip → drag `lc7.app` to **Applications**
3. Install hashcat: `brew install hashcat`
4. Open `lc7.app` (first launch: right-click → Open to bypass Gatekeeper)

**Option 2 — Build from source**

```bash
# Prerequisites
brew install cmake qt@5 quazip openssl hashcat

# Clone & build
git clone https://github.com/cleverg0d/l0phtcrack.git
cd l0phtcrack
mkdir build-macos && cd build-macos
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt@5)
make -j$(sysctl -n hw.logicalcpu)
# Result: build-macos/dist/lc7.app
```

---

### Linux ARM64

**Option 1 — Pre-built (recommended)**

```bash
# Download and extract
wget https://github.com/cleverg0d/l0phtcrack/releases/download/v7.3.3/lc7-v7.3.3-linux-arm64.tar.gz
tar xzf lc7-v7.3.3-linux-arm64.tar.gz

# Install hashcat (if not already installed)
sudo apt install hashcat   # Debian/Ubuntu/Kali

# Run
./dist/lc7
```

**Option 2 — Build from source (Docker, recommended for reproducible builds)**

```bash
# Prerequisites: Docker with ARM64 support (or native ARM64 Linux machine)
# Install build dependencies (Debian/Kali ARM64):
sudo apt install -y cmake qt5-default qtbase5-dev libqt5sql5 libssl-dev \
  libssh2-1-dev zlib1g-dev libesedb-dev

# Clone & build
git clone https://github.com/cleverg0d/l0phtcrack.git
cd l0phtcrack
mkdir build-linux-arm64 && cd build-linux-arm64
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
# Result: build-linux-arm64/dist/lc7
```

> For reproducible builds using the project's Docker environment, see [BUILDING.md](BUILDING.md).

---

## Supported attack modes

| Mode | Description |
|------|-------------|
| **Brute Force** | Fast / Standard / Extended — incremental mask attack via hashcat GPU |
| **Dictionary** | Wordlist attack with optional rules (best64, rockyou-30000, d3ad0ne, custom) |
| **User Info** | Generates candidates from usernames and full names (`Firstname123`, `lastname@2024`, etc.) |
| **Finalyze** | Post-crack rule pass — derives additional passwords from already-found ones |

---

## Changelog

### v7.3.3 (2026-03-29)
- **Linux ARM64 support** — fully working build (tested on Kali Linux ARM64)
- **Statistics UI** — Top Passwords table: two-line bold headers (password + count · %), no alternating rows, removed redundant Total row
- **Resources consolidation** — wordlists and rules unified in `resources/`; auto-copied into platform dist at build time
- **Finalyze path fix** — bundled `common/rules/` takes priority over system hashcat paths on all platforms
- **Linux RPATH fix** — `$ORIGIN`-based RPATH in all `.so` files via cmake, eliminates hardcoded build paths
- **lc7core loading fix** — explicit `applicationDirPath()`-based path prevents `dlopen()` RUNPATH failures

### v7.3.2 (2025-03-24)
- Statistics section: crack rate %, top passwords, duplicate detection, complexity analysis, Export CSV
- Folder wordlist mode, custom hashcat rule file per attack, Finalise all-rules sequential mode
- CPU load and temperature monitoring for Apple M-series

### v7.3.1
- CPU monitoring for Apple M-series processors

---

## Planned

1. Linux x86-64 release (Intel/AMD)
2. Windows x64 release (Intel/AMD)
3. Adapted UI for modern look and feel
4. Benchmark improvements

---

## Credits

- **L0phtCrack 7** — original product by L0pht Holdings, LLC. Source: [gitlab.com/l0phtcrack/l0phtcrack](https://gitlab.com/l0phtcrack/l0phtcrack)
- **hashcat** — [hashcat.net](https://hashcat.net/hashcat/) by atom and contributors
- **Qt** — [qt.io](https://www.qt.io/)

---

## Legal

This tool is intended for **authorized password auditing only**. Use only on systems and accounts you own or have explicit written permission to test. Unauthorized use is illegal.

This fork is provided under the same license terms as the upstream project (see `LICENSE.MIT` / `LICENSE-2.0.APACHE.txt`).
