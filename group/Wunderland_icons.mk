# Builds WunderlandLE_0xE4F76EAD.mif from ..\gfx\qgn_menu_wunderland.svg
# Recipe lines MUST start with a TAB character.
# Build with the S60 3rd Ed. (9.1) SDK: its mifconv keeps plain SVG-T (no /X option), which every
# 9.x phone can read. FP1+ SDKs binary-encode by default; add /X there to keep 9.1 icons working.

ifeq (WINS,$(findstring WINS, $(PLATFORM)))
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)epoc32\data\z
endif

TARGETDIR=$(ZDIR)\resource\apps
ICONTARGETFILENAME=$(TARGETDIR)\WunderlandLE_0xE4F76EAD.mif
ICONDIR=..\gfx

do_nothing :
	@rem do_nothing

MAKMAKE : do_nothing
BLD : do_nothing
CLEAN : do_nothing
LIB : do_nothing
CLEANLIB : do_nothing

RESOURCE : $(ICONTARGETFILENAME)

$(ICONTARGETFILENAME) : $(ICONDIR)\qgn_menu_wunderland.svg
	mifconv $(ICONTARGETFILENAME) /c32 $(ICONDIR)\qgn_menu_wunderland.svg

FREEZE : do_nothing
SAVESPACE : do_nothing
RELEASABLES :
	@echo $(ICONTARGETFILENAME)
FINAL : do_nothing
