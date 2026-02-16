# CS2 File Dumper

Automated binary collector for CS2. Grabs all game DLL and EXE files after each update and organizes them into timestamped folders.

## How it works

Finds CS2 through Windows registry and Steam library folders. Reads the `LastUpdated` timestamp from Steam's `appmanifest_730.acf` to determine when the game was last patched. Copies all binaries into a new folder named after that timestamp. Skips if already dumped.

## What gets collected

All binaries from `game/bin/win64/` and `game/csgo/bin/win64/` — around 90 files including `client.dll` · `server.dll` · `engine2.dll` · `schemasystem.dll` · `tier0.dll` · `panorama.dll` · `vphysics2.dll` · `cs2.exe` and other modules.

## Purpose

Keeping old versions of game binaries across updates for reverse engineering. Diff DLLs between patches, track schema and offset changes, update signatures and patterns without searching for old game builds manually.
