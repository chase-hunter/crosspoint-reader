#pragma once

/**
 * Sync the ESP32 system clock with an NTP server.
 * Blocks until time is synced or timeout (5 seconds) is reached.
 * Safe to call multiple times; restarts SNTP if already running.
 */
void syncTimeWithNTP();
