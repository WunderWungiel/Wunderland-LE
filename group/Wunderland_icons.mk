# Builds WunderlandLE_0xE4F76EAD.mif from ..\gfx\qgn_menu_wunderland.svg (app icon), and
# WunderlandLE_0xE4F76EAD_tabs.mif + its .mbg header from the ..\gfx\tab_*.svg tab icons.
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
TABSTARGETFILENAME=$(TARGETDIR)\WunderlandLE_0xE4F76EAD_tabs.mif
TABSHEADERFILENAME=$(EPOCROOT)epoc32\include\wunderlandle_0xe4f76ead_tabs.mbg
ICONDIR=..\gfx

# Order matters: CWunderlandTabBar computes ids as first + (tab * 4 + state) * 2.
# States: normal, active (_a), focused (_f), focused + active (_fa).
TABICONS=$(ICONDIR)\tab_home.svg $(ICONDIR)\tab_home_a.svg \
         $(ICONDIR)\tab_home_f.svg $(ICONDIR)\tab_home_fa.svg \
         $(ICONDIR)\tab_categories.svg $(ICONDIR)\tab_categories_a.svg \
         $(ICONDIR)\tab_categories_f.svg $(ICONDIR)\tab_categories_fa.svg \
         $(ICONDIR)\tab_search.svg $(ICONDIR)\tab_search_a.svg \
         $(ICONDIR)\tab_search_f.svg $(ICONDIR)\tab_search_fa.svg

do_nothing :
	@rem do_nothing

MAKMAKE : do_nothing
BLD : do_nothing
CLEAN : do_nothing
LIB : do_nothing
CLEANLIB : do_nothing

RESOURCE : $(ICONTARGETFILENAME) $(TABSTARGETFILENAME)

$(ICONTARGETFILENAME) : $(ICONDIR)\qgn_menu_wunderland.svg
	mifconv $(ICONTARGETFILENAME) /c32 $(ICONDIR)\qgn_menu_wunderland.svg

$(TABSTARGETFILENAME) : $(TABICONS)
	mifconv $(TABSTARGETFILENAME) /h$(TABSHEADERFILENAME) /c32,8 $(TABICONS)

FREEZE : do_nothing
SAVESPACE : do_nothing
RELEASABLES :
	@echo $(ICONTARGETFILENAME)
	@echo $(TABSTARGETFILENAME)
	@echo $(TABSHEADERFILENAME)
FINAL : do_nothing
