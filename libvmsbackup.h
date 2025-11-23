/*
 * Lightweight embedding surface for vmsbackup.
 *
 * This header exposes a callable entry point that mirrors the existing
 * command-line interface. It currently shares process-global state with
 * the CLI and will exit(3) on fatal errors, so it is best suited for
 * simple embedding until the core is fully refactored into a re-entrant
 * library. Build with -DVMSBACKUP_NO_MAIN to omit the CLI's main().
 */

#ifndef LIBVMSBACKUP_H
#define LIBVMSBACKUP_H

#ifdef __cplusplus
extern "C" {
#endif

int vmsbackup_main(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* LIBVMSBACKUP_H */
