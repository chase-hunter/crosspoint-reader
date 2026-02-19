# t1.1 checkpoint

Created: 2026-02-19

This checkpoint captures the requested baseline state (`t1.1`) for quick manual restore in a non-git workspace.

Includes boot screen refresh:
- Stylized CH monogram logo
- Title lines: "CrossPoint Reworked" and "github.com/chase-hunter"
- "BOOTING" line preserved

## Snapshot files

- `src__activities__home__HomeActivity.cpp`
- `src__activities__boot_sleep__BootActivity.cpp`
- `platformio.ini`

## Restore steps

1. Replace current files with these snapshot copies:
   - `src/activities/home/HomeActivity.cpp`
   - `src/activities/boot_sleep/BootActivity.cpp`
   - `platformio.ini`
2. Rebuild firmware.

When you say **"revert to t1.1"**, I will restore these exact files.
