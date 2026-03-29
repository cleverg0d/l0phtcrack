# Building L0phtCrack 7 — Platform Guide

L0phtCrack 7 uses CMake + Qt5. Each platform has its own build environment.
All platform builds share the same source tree and produce self-contained distributions.

---

## Shared Resources

Wordlists and rule files live in `resources/` at the repo root:

```
resources/
├── wordlists/
│   ├── wordlist-small.txt
│   ├── wordlist-medium.txt
│   ├── wordlist-big.txt
│   └── wordlist-huge.txt
└── rules/
    ├── buka_400k.rule
    ├── best64.rule
    ├── dive.rule
    ├── d3ad0ne.rule
    └── ... (12 rule files total)
```

CMake copies these into every platform's output automatically during build.

---

## macOS (Apple Silicon M1/M2/M3/M4)

### Requirements
- macOS 13+
- Xcode Command Line Tools: `xcode-select --install`
- Homebrew: https://brew.sh
- Qt5: `brew install qt@5`
- hashcat: `brew install hashcat`
- CMake: `brew install cmake`
- libesedb (for NTDS.DIT import): `brew install libesedb`

### Build

```bash
mkdir build-macos && cd build-macos
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@5) -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.logicalcpu) lc7
```

### Output
```
build-macos/dist/lc7.app   ← self-contained .app bundle, drag to /Applications
```

The `.app` bundle contains Qt frameworks, all plugins, wordlists, and rules.
No installation required — copy the folder and run.

---

## Linux ARM64 (Kali / Debian on Apple Silicon via VMware Fusion)

### Requirements
- Docker Desktop (on the macOS host) with `linux/arm64` platform support
- OR a native ARM64 Linux machine with the packages below

### Build via Docker (recommended, from macOS host)

```bash
# Build the Docker builder image (first time only, ~5 min):
docker build --platform linux/arm64 \
    -f platform/linux/Dockerfile \
    -t lc7-linux \
    .

# Build L0phtCrack (output goes to /tmp/lc7-linux-build/dist/):
docker run --rm --platform linux/arm64 \
    -v $(pwd):/src \
    -v /tmp/lc7-linux-build:/build \
    lc7-linux

# Deploy to Kali VM (adjust IP/credentials as needed):
rsync -av /tmp/lc7-linux-build/dist/ user@kali-vm-ip:~/lc7/dist/
```

### Run on Kali
```bash
cd ~/lc7/dist
./lc7
```

### Build natively (on ARM64 Linux)

```bash
sudo apt-get install -y \
    build-essential cmake ninja-build \
    qt5-qmake qtbase5-dev qtbase5-private-dev \
    qttools5-dev libqt5keychain1 qt5keychain-dev \
    libssl-dev zlib1g-dev libssh2-1-dev \
    libesedb-dev hashcat

mkdir build-linux-arm64 && cd build-linux-arm64
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) lc7
```

### Output
```
build-linux-arm64/dist/    ← self-contained distribution folder
  lc7                      ← main binary
  liblc7core.so
  lcplugins/               ← all plugins (.so + manifest.json)
  common/
    wordlists/             ← wordlists (copied from resources/)
    rules/                 ← rule files (copied from resources/)
```

---

## Linux x86_64 (Kali / Debian / Ubuntu on Intel/AMD)

### Build via Docker (from any x86_64 host)

```bash
docker build --platform linux/amd64 \
    -f platform/linux/Dockerfile \
    -t lc7-linux-x86 \
    .

docker run --rm --platform linux/amd64 \
    -v $(pwd):/src \
    -v /tmp/lc7-linux-x86-build:/build \
    lc7-linux-x86

rsync -av /tmp/lc7-linux-x86-build/dist/ user@linux-host:~/lc7/dist/
```

### Build natively (on x86_64 Linux)
Same as ARM64 above, substitute `build-linux-x86_64` for the build dir.

---

## Windows x64 (Intel/AMD — original upstream platform)

### Requirements
- Visual Studio 2019 or 2022 (Desktop C++ workload)
- Qt5 for MSVC: https://www.qt.io/download
- CMake 3.15+

### Build

```bat
mkdir build-win64
cd build-win64
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64
cmake --build . --config Release
```

### Output
```
build-win64/dist/Release/
  lc7.exe
  lcplugins/
  common/
    wordlists/
    rules/
```

---

## Project Structure

```
l0phtcrack/
├── CMakeLists.txt           # Root build — detects platform, adds all modules
├── cmake/                   # CMake helper modules
├── resources/               # Shared assets — copied into ALL platform builds
│   ├── wordlists/           # 4 wordlist files
│   └── rules/               # 12 hashcat/JTR rule files
├── platform/
│   └── linux/
│       └── Dockerfile       # Docker builder image for Linux (ARM64 + x86_64)
├── artwork/                 # Source icon files (PSD, AI, PNG at all sizes)
├── lc7/                     # Main Qt application
├── lc7base/                 # Base UI framework plugin
├── lc7core/                 # Core shared library
├── lc7hashcat/              # Hashcat cracking engine plugin (macOS primary)
├── lc7importunix/           # Unix/Linux hash import (shadow, etc.)
├── lc7importwin/            # Windows hash import (SAM, NTDS.DIT, PWDump)
├── lc7jtr/                  # JTR engine plugin (Linux + Windows)
├── lc7password/             # Password management plugin
├── lc7reports/              # Report generation plugin
├── lc7wizard/               # Setup wizard plugin
├── jtrdll_hashcat/          # Hashcat shim source (non-Windows jtrdll API)
│   ├── jtrdll_apple_silicon.c   # macOS ARM64 shim
│   └── jtrdll_linux_x86_64.c   # Linux ARM64 + x86_64 shim
├── external/                # Third-party dependencies (submodules + vendored)
│   ├── jtrdll/              # John the Ripper DLL (Windows GPU cracking)
│   ├── openssl/             # OpenSSL
│   ├── quazip/              # ZIP handling
│   ├── libssh2/             # SSH (Windows remote agent)
│   ├── qtkeychain/          # Qt keychain
│   ├── qtpropertybrowser/   # Qt property browser widget
│   ├── qtsingleapplication/ # Single instance app
│   └── zlib/                # Compression
├── dist/                    # Static distribution assets (manifests, Windows agents)
│   ├── common/              # Shared across all Windows build types
│   └── win64/               # Windows x64 specific
├── api/                     # Plugin API headers and examples
├── tools/                   # Release and packaging scripts
├── doc/                     # Documentation
└── tests/                   # Test suites
```

---

## After Making Code Changes

Any code change automatically applies to all platforms on the next build.
Platform-specific paths use `#if defined(__APPLE__)` / `#if defined(__linux__)` / `#ifdef _WIN32` guards.

**macOS rebuild:**
```bash
cd build-macos && make -j$(sysctl -n hw.logicalcpu) lc7
```

**Linux rebuild (Docker):**
```bash
docker run --rm --platform linux/arm64 \
    -v $(pwd):/src \
    -v /tmp/lc7-linux-build:/build \
    lc7-linux
```

**Partial rebuild (single plugin):**
```bash
# macOS — just rebuild a plugin without full macdeployqt:
make -C build-macos lc7jtr
# Linux:
docker run --rm --platform linux/arm64 -v $(pwd):/src -v /tmp/lc7-linux-build:/build lc7-linux \
    bash -c "cd /build && make -j$(nproc) lc7jtr"
```
