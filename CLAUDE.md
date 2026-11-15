# WunderlandLE — native Symbian client for Wunderland Store

Native S60 3rd/5th Edition (Symbian 9.1–9.4) client for Wunderland Store (https://ovi.wunderwungiel.pl,
server: https://github.com/WunderWungiel/Wunderland-Store). There is no API yet, so use sample data.

**Read `.claude/rules/symbian-cpp.md` and `docs/symbian-cpp-docs/AGENTS.md` before touching code.**

## Layout
- `group/` — `bld.inf`, `Wunderland.mmp`, `Wunderland_icons.mk` (SVG → MIF; 9.1 SDK mifconv has no `/X`)
- `inc/`, `src/` — Avkon app: Application → Document → AppUi → Container
- `data/` — `WunderlandLE_0xE4F76EAD.rss` (main) and `_reg.rss` (menu registration)
- `gfx/` — SVG-Tiny app icon
- `sis/` — `WunderlandLE.pkg` (packaged unsigned)
- `docs/symbian-cpp-docs/` — documentation submodule

## Build
From an SDK command prompt (`EPOCROOT` set, project on the same drive as the SDK):
- `build.bat` — `bldmake`/`abld` GCCE UREL + makesis → unsigned `sis\WunderlandLE.sis` (release)
- `build.bat sbs` — Raptor build with the Qt SDK's Symbian^3 SDK (compile check only, don't ship)
- `build.bat clean`

Qt SDK env for the compile check: `call C:\QtSDK\Symbian\SDKs\Symbian3Qt474\env.bat`

## Git
Don't commit until the user confirms a change builds and runs on a device.
