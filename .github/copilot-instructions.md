## Purpose
This repository contains C utilities for decoding VAX/VMS backup savesets and related tape image tools. These instructions are focused, actionable guidance to help AI coding agents be productive immediately when editing, building, or extending this codebase.

## Big picture (what to read first)
- **Primary executable:** `vmsbackup` implemented in `vmsbackup.c` — contains command-line parsing, record structures, and the main decode/extract logic. Read this file to understand the program flow and decoding state machine.
- **Helpers & utilities:** `match.c` (pattern matching used by selection flags), `cp_tape.c`, `unpack_tap.c`, `dmp_tfile.c`, `ext_tfile.c` — these build into companion tools and show I/O/tape-handling code paths.
- **Build configuration:** `Makefile.common` centralizes flags and rules; platform-specific overrides live in `Makefile.linux`, `Makefile.msys2`, `Makefile.mingw32`, `Makefile.pi32`.

## Architecture & data flow (short)
- Input: physical tape device or tape-image file (SIMH, Atari/DVD formats) passed via `--file`/`-f`.
- `vmsbackup` reads sequential blocks, decodes record headers (`bbh`, `brh` structs), builds `file_details` state for each file, and writes output files or directory listings.
- Record parsing, VFC/VAX/VAR handling and error recovery all live in `vmsbackup.c` — changing record handling usually requires edits there and careful testing with real/representative images.

## Build & run (concrete commands)
- Build with the platform makefile that best matches your environment. Examples:
  - Linux (32-bit build configured in tree):
    ```bash
    make -f Makefile.linux
    ```
  - Generic (explicit file):
    ```bash
    make -f Makefile.common
    ```
- After build, run the help or list mode to sanity-check a sample image:
  ```bash
  ./vmsbackup -t -f path/to/tape.img    # list contents
  ./vmsbackup -x -f path/to/tape.img    # extract files
  ./vmsbackup -I -f path/to/simh.img    # read SIMH-format image
  ```

## Project-specific conventions & patterns
- Configuration via compile-time macros in `Makefile.common` (e.g., `HAVE_MTIO`, `MINGW`, `MSYS2`, `EXTRA_DEFINES`). Modify `Makefile.*` to enable/disable platform features.
- Global mutable state: the code uses a package-global `struct file_details file;` and many globals (flags like `vflag`, `dflag`). When adding features prefer keeping changes local to `vmsbackup.c` and update top-level structures consistently.
- Naming and constants: look for `FAB_dol_C_*` and `FREC_*` enums/constants — these are used across decode code; add new record IDs or formats there and update parsing logic accordingly.
- Error handling approach: record and decode errors are generally non-fatal and handled by setting skip masks (`SKIP_TO_BLOCK`, etc.) — prefer following that pattern instead of aborting.

## Files to inspect for common change types
- Parsing/decoding behaviour: `vmsbackup.c` (main), `match.c` (pattern matching used by selection flags).
- Build tweaks or new compile-time flags: `Makefile.common` and the relevant `Makefile.<platform>`.
- New helper tools or commands: add new .c and update the `default:` target in `Makefile.common` to include them (see how `cp_tape` and `unpack_tap` are included).

## Quick grep examples useful to agents
- Find platform-conditional code: `grep -R "HAVE_MTIO\|MSYS2\|MINGW" -n`.
- Find where a record ID is handled: `grep -n "FREC_.*" -n vmsbackup.c` or search for `switch`/`case` blocks that reference FREC_* constants.

## Tests & debugging notes (what I could discover)
- There are no automated test suites in the repo. Validate changes manually using sample tape images and these invocations:
  - `./vmsbackup -t -f sample.img` — verify listing output.
  - `./vmsbackup -x -f sample.img` — verify extracted files and names (pay attention to `--delimiter`, `-R`, `-L` options).
- When reproducing record-level bugs, prefer small, controlled tape images (SIMH format supported via `-I`).

## When to update docs
- If you change CLI options, update both `README.md` and `vmsbackup.1` (manpage). The help text in `README.md` is used as the canonical options reference.

## If you (the agent) add features
- Update `Makefile.common` to add any required `DEFS` or `LIBS` and add new targets to the `default:` rule.
- Run the same example commands above and verify no regressions in listing/extraction behaviour.

## Questions for the maintainer
- Are there canonical sample tape images or a test corpus you prefer agents use for verification? If so, state their path or where to download them.
- Do you prefer changes to be backwards-compatible for older savesets (VAX vs AXP), or is a breaking change acceptable if documented?

---
If anything here is unclear or you want the document to include additional details (example images, preferred build flags for macOS, or sample invocation scripts), tell me what to add and I'll update this file.
