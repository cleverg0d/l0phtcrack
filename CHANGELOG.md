# Changelog

All notable changes in **this fork** are listed here. The upstream commercial product maintains its own release notes.

## [7.3.0] - 2025-03-24

### Fixed

- **macOS app launch (bundle):** `libqtsingleapplication` / `libqtpropertybrowser` referenced Qt via broken `@loader_path/lc7.app/...` paths. `liblc7core.dylib` was not shipped next to `lc7` inside `Contents/MacOS`, so core load failed. `lc7` used a build-tree-only RPATH. **`macdeployqt`** is run automatically after linking `lc7` so **PlugIns** stop pulling a second Qt from Homebrew (duplicate QtCore previously caused immediate SIGSEGV and sometimes no crash reporter dialog).
- **Audit / Technique tree**: `RefreshContent()` connected `QAbstractItemView::entered` and `viewportEntered` on every rebuild, stacking duplicate slots and risking crashes when selecting techniques such as **User Info**. Connections are now made once in `CAuditPage` construction; `QItemSelectionModel::currentChanged` is disconnected before replacing the model.
- **Audit**: `FindComponentByID` null checks before `Execute` in `CAuditPage` (avoids silent crash if a plugin component is missing).
- **JtR technique GUI**: bounds checks on `args` and null `pagewidget` handling in `CTechniqueJTR*GUI::ExecuteCommand` (`create` / `store` / `queue` paths).

### Changed

- **Display version** bumped to **7.3.0** in `lc7/include/appversion.h` and macOS bundle metadata strings.

### Notes

- **Diagnostic log**: `LC7DiagnosticLog` / `TRDBG` append to `lc7-diagnostic.log` (path printed at session start). **Audit** technique selection logs action name, component id, and create success or failure lines.
- **Fork process / build layout**: see `macos-fork/README.md`, `macos-fork/BUILD.md`, and the running journal `macos-fork/SESSION_LOG.md`.
