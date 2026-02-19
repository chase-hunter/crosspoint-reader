# t1.2 checkpoint

Created: 2026-02-19

This checkpoint captures the current firmware state for reliable in-workspace restore.

## Snapshot files

- `src__activities__home__HomeActivity.cpp`
- `src__activities__boot_sleep__BootActivity.cpp`
- `platformio.ini`

## Restore steps (in workspace)

1. Replace these files in the project root:
   - `src/activities/home/HomeActivity.cpp`
   - `src/activities/boot_sleep/BootActivity.cpp`
   - `platformio.ini`
2. Rebuild firmware.

## Note about full filesystem backup

The current workspace is mounted via `vscode-vfs://...`, so creating a raw full-filesystem image from this agent is limited.
For a full machine-level backup, run locally from your real repo path:

```powershell
Compress-Archive -Path .\* -DestinationPath .\crosspoint-t1.2-full.zip -Force
```

When you say **"revert to t1.2"**, I will restore these exact snapshot files.
