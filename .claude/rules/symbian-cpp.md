# Symbian C++ — always use Symbian documentation

This project is **native Symbian C++** (Symbian OS 9.1–9.4, S60 3rd/5th Edition). It is NOT standard
C++: no STL, no exceptions, no `std::string`, and its own memory and error model (leaves, cleanup stack,
two-phase construction, descriptors, active objects). General C++ knowledge is often wrong here.

## Rules

1. **Before writing or changing any Symbian code, MMP, bld.inf, .rss, .pkg or build step, consult the
   Symbian C++ docs.** Primary source: `docs/symbian-cpp-docs/` (git submodule of
   https://github.com/WunderWungiel/symbian-cpp-docs). If it isn't checked out, run
   `git submodule update --init` or read it on GitHub.
   - Start with `docs/symbian-cpp-docs/AGENTS.md`. It has the hard rules, default choices, the
     pre-commit checklist and common mistakes. Follow it.
   - Then read the chapter for the task (`docs/symbian-cpp-docs/docs/NN-*.md`, index in `llms.txt`).
2. **Use Symbian-specific sources only.** Don't fill gaps with standard C++, Qt, POSIX or modern-C++
   habits. If our docs don't cover something, look at the SDK headers (`%EPOCROOT%epoc32\include`),
   the S60 SDK Help (`S60Doc`), the SDK examples (`S60Ex`, `Examples`), or archived Forum Nokia /
   Symbian Developer material. If you still can't verify an API, **say so**. Never invent API names.
3. **Target constraints:**
   - Run on Symbian **9.1, 9.2, 9.3 and 9.4**. Only use APIs present in **S60 3rd Edition (9.1)**
     unless the code checks the platform at runtime and the feature is optional (e.g. touch on 5th Ed.).
   - Release builds use the **oldest** S60 SDK (S60 3rd Ed. MR / 9.1) with GCCE. Binaries built with
     the Symbian^3 (Qt SDK) SDK link to newer avkon ordinals and won't load on S60 3rd/5th phones. Use
     that SDK only as a compile check (`build.bat sbs`).
   - The app ships as an **unsigned** SIS: **user capabilities only** (e.g. `NetworkServices`, `ReadUserData`).
   - Test devices: Nokia N95 (9.2, 240x320, non-touch), Nokia E52 (9.3, 320x240 landscape,
     non-touch), Samsung Omnia HD (9.4, 360x640, touch). Layouts must handle portrait, landscape and
     touch (`HandleResourceChangeL` / `KEikDynamicLayoutVariantSwitch`).
4. **Keep UID3 `0xE4F76EAD` consistent** across the MMP, `_reg.rss`, `KUidWunderlandApp`, the PKG
   header and any `\private\E4F76EAD\` paths.
5. Before saying code is done, go through the checklist in `AGENTS.md` §4.
