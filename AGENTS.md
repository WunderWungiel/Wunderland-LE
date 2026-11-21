# WunderlandLE — native Symbian client for Wunderland Store

Native S60 3rd/5th Edition (Symbian 9.1–9.4) client for Wunderland Store (https://ovi.wunderwungiel.pl,
server: https://github.com/WunderWungiel/Wunderland-Store).

## Store API
The store has a JSON API (v1). **Before any API work, read its documentation from the server repo:
`docs/api.md` in https://github.com/WunderWungiel/Wunderland-Store** (raw:
https://raw.githubusercontent.com/WunderWungiel/Wunderland-Store/master/docs/api.md). It's the only spec;
don't guess endpoints, fields or error codes. The client is `CWunderlandApiClient` (`inc/WunderlandApiClient.h`),
with typed views in `inc/WunderlandApiTypes.h`.
- From the API: the content types (Categories tab) and each type's categories (content type page).
- Still sample data: the Home list and the search results (`CWunderlandContainer::StartLoadL`).

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

**After every code/resource/build change, rebuild the release SIS** with the 9.1 SDK and report the
result (errors/warnings from our files; SDK-header warnings are noise). Environment from a plain shell:
```bat
set EPOCROOT=\Symbian\9.1\S60_3rd\
set PATH=C:\Perl\bin;C:\Program Files (x86)\Common Files\Symbian\Tools;C:\Symbian\9.1\S60_3rd\epoc32\tools;C:\Program Files (x86)\CSL Arm Toolchain\bin;%PATH%
build.bat
```
Call `build.bat` by its full path from `cmd /c`. A `build.bat sbs` run overwrites `sis\WunderlandLE.sis`
with a non-shippable binary, so always finish with the 9.1 build.

## Git
Don't commit until the user confirms a change builds and runs on a device.
