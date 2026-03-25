# L0phtCrack 7 (community macOS / portability work)

Unofficial tree for **macOS Apple Silicon** builds and small reliability patches. Upstream product and licensing: **L0pht Holdings, LLC**. Use only on systems you are authorized to test.

## Where everything is

| Topic | Location |
|-------|----------|
| Who runs builds, agent checklist | **`AGENTS.md`** |
| macOS build, rollback, crash logs, session journal | **`macos-fork/`** (`README.md`, `RECOVERY.md`, `BUILD.md`, `SESSION_LOG.md`, `PROJECT_MACOS.md`) |
| Functional change history (no icon-only noise) | `CHANGELOG.md` |
| Compiled app | Your CMake build dir, usually **`build-macos/dist/lc7.app`** (binary dir ≠ source dir; this is expected) |

## Credits

- **L0phtCrack 7**: L0pht Holdings, LLC / L0pht lineage.
- **John the Ripper**: Solar Designer and contributors (`https://www.openwall.com/john/`).
- Fork version in `lc7/include/appversion.h` is **not** a vendor release id.
