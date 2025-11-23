# vmsbackup GUI (planning stub)

Toolkit: Qt (Qt 6 preferred, Qt 5 fallback) for macOS/Linux/Windows. The GUI will link against `libvmsbackup.a` (built with `-DVMSBACKUP_NO_MAIN`) and later move to a fully re-entrant API once the core is refactored.

Planned features:
- Open raw/DVD/SIMH tape images, list savesets, show files with filters, extract selections with progress/cancel.
- Surface advanced options: delimiter, lowercase/noversions, binary export, VFC handling, hierarchy/prompt/verbose masks.
- Log pane for decode/warnings; threading to keep UI responsive.

Build outline (once sources land):
1) Install Qt (e.g., Homebrew `brew install qt`, Debian/Ubuntu `sudo apt install qtbase5-dev`, MSYS2 `pacman -S mingw-w64-x86_64-qt6-base`).
2) Build the library: `make -f ../Makefile.common libvmsbackup.a`.
3) Configure the GUI: `cmake -S . -B build -DUSE_QT6=ON` (fallback to Qt5 if not found).
4) Build: `cmake --build build`.

Status: this is scaffolding to document the plan; the Qt sources and CMakeLists will be added alongside the continuing core refactor.
