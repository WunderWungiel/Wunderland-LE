#include <akndef.h>
#include <avkon.hrh>
#include <eikon.hrh>
#include <e32math.h>
#include <gdi.h>
#include "WunderlandAppList.h"
#include "WunderlandContainer.h"
#include "WunderlandTabBar.h"
#include "WunderlandTooltip.h"

namespace
    {
    const TInt KCategoryGranularity = 16;
    const TInt KContentTypes = 3;     // sample apps, games, themes
    const TInt KAppsPerType = 30;     // sample entries per content type
    const TInt KSampleSeed = 0x5EED;  // fixed: the same sample list on every start

    // Pseudo-random number in 0..aRange-1
    TInt Random(TInt64& aSeed, TInt aRange)
        {
        return (Math::Rand(aSeed) & KMaxTInt) % aRange;
        }
    }

CWunderlandContainer* CWunderlandContainer::NewL(const TRect& aRect)
    {
    CWunderlandContainer* self = new (ELeave) CWunderlandContainer;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandContainer::CWunderlandContainer()
    {
    }

void CWunderlandContainer::ConstructL(const TRect& aRect)
    {
    CreateWindowL();

    iTabBar = CWunderlandTabBar::NewL(*this);
    iTabBar->SetObserver(this);
    iAppList = CWunderlandAppList::NewL(*this);
    iAppList->SetObserver(this);
    iTooltip = CWunderlandTooltip::NewL(*this);
    iTooltip->MakeVisible(EFalse);
    iCategories = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    EnableDragEvents();   // touch scrolling of the list

    AddSampleCategoriesL();
    AddSampleAppsL();
    UpdateAppListL();

    SetRect(aRect);   // lays out the children via SizeChanged()
    FocusListL();     // start on the first entry, like Ovi Store
    ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    iApps.Close();
    delete iCategories;
    delete iTooltip;
    delete iAppList;
    delete iTabBar;
    }

void CWunderlandContainer::AddSampleCategoriesL()
    {
    _LIT(KCat1,  "Camera, photos, videos");
    _LIT(KCat2,  "Emulator");
    _LIT(KCat3,  "Business");
    _LIT(KCat4,  "Communication");
    _LIT(KCat5,  "Entertainment");
    _LIT(KCat6,  "Maps & navigation");
    _LIT(KCat7,  "Music & audio");
    _LIT(KCat8,  "News & weather");
    _LIT(KCat9,  "Productivity");
    _LIT(KCat10, "Social networks");
    _LIT(KCat11, "Tools & utilities");
    _LIT(KCat12, "Travel");
    iCategories->AppendL(KCat1);
    iCategories->AppendL(KCat2);
    iCategories->AppendL(KCat3);
    iCategories->AppendL(KCat4);
    iCategories->AppendL(KCat5);
    iCategories->AppendL(KCat6);
    iCategories->AppendL(KCat7);
    iCategories->AppendL(KCat8);
    iCategories->AppendL(KCat9);
    iCategories->AppendL(KCat10);
    iCategories->AppendL(KCat11);
    iCategories->AppendL(KCat12);
    }

// Random names, categories and ratings, generated from a fixed seed.
void CWunderlandContainer::AddSampleAppsL()
    {
    const TText* const appFirst[] =
        {
        _S("Smart"), _S("Pocket"), _S("Quick"), _S("Easy"), _S("Mobile"), _S("Tiny"),
        _S("Handy"), _S("Simple"), _S("Night"), _S("Open"), _S("Super"), _S("Daily")
        };
    const TText* const appSecond[] =
        {
        _S("Notes"), _S("Radio"), _S("Camera"), _S("Tracker"), _S("Browser"), _S("Weather"),
        _S("Tasks"), _S("Player"), _S("Maps"), _S("Chat"), _S("Clock"), _S("Reader")
        };
    const TText* const gameFirst[] =
        {
        _S("Space"), _S("Pixel"), _S("Turbo"), _S("Crazy"), _S("Jungle"), _S("Galaxy"),
        _S("Mini"), _S("Retro"), _S("Ninja"), _S("Castle")
        };
    const TText* const gameSecond[] =
        {
        _S("Racer"), _S("Golf"), _S("Quest"), _S("Blocks"), _S("Pinball"), _S("Chess"),
        _S("Rally"), _S("Puzzle"), _S("Defender"), _S("Bowling")
        };
    const TText* const themeFirst[] =
        {
        _S("Blue"), _S("Dark"), _S("Neon"), _S("Autumn"), _S("Ocean"), _S("Carbon"),
        _S("Spring"), _S("Sunset")
        };
    const TText* const themeSecond[] =
        {
        _S("Glass"), _S("Waves"), _S("Lines"), _S("Dreams"), _S("Leaves"), _S("Shine"),
        _S("Bubbles"), _S("Metal")
        };
    #define WUNDERLAND_COUNT(a) (static_cast<TInt>(sizeof(a) / sizeof((a)[0])))

    TInt64 seed = KSampleSeed;
    const TInt categories = iCategories->MdcaCount();
    for (TInt type = 0; type < KContentTypes; ++type)
        {
        const TText* const* first;
        const TText* const* second;
        TInt firstCount;
        TInt secondCount;
        switch (type)
            {
            case 0:   // apps
                first = appFirst;     firstCount = WUNDERLAND_COUNT(appFirst);
                second = appSecond;   secondCount = WUNDERLAND_COUNT(appSecond);
                break;
            case 1:   // games
                first = gameFirst;    firstCount = WUNDERLAND_COUNT(gameFirst);
                second = gameSecond;  secondCount = WUNDERLAND_COUNT(gameSecond);
                break;
            default:  // themes
                first = themeFirst;   firstCount = WUNDERLAND_COUNT(themeFirst);
                second = themeSecond; secondCount = WUNDERLAND_COUNT(themeSecond);
                break;
            }
        for (TInt i = 0; i < KAppsPerType; ++i)
            {
            TApp app;
            const TPtrC firstWord(first[Random(seed, firstCount)]);
            const TPtrC secondWord(second[Random(seed, secondCount)]);
            app.iName.Copy(firstWord.Left(app.iName.MaxLength()));
            if (app.iName.Length() + 1 + secondWord.Length() <= app.iName.MaxLength())
                {
                app.iName.Append(' ');
                app.iName.Append(secondWord);
                }
            app.iCategory = categories > 0 ? Random(seed, categories) : 0;
            app.iRating = 1 + Random(seed, 5);
            iApps.AppendL(app);
            }
        }
    #undef WUNDERLAND_COUNT
    }

// Every entry, with its category under the name. The Categories and Search
// pages don't exist yet, so the list is the same on every tab.
void CWunderlandContainer::UpdateAppListL()
    {
    iAppList->Reset();
    const TInt categories = iCategories->MdcaCount();
    const TInt count = iApps.Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TApp& app = iApps[i];
        TPtrC subtitle(KNullDesC);
        if (app.iCategory >= 0 && app.iCategory < categories)
            {
            subtitle.Set(iCategories->MdcaPoint(app.iCategory));
            }
        iAppList->AppendL(app.iName, subtitle, app.iRating);
        }
    }

TRect CWunderlandContainer::ContentRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl.iX, iTabBar->Rect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    }

void CWunderlandContainer::FocusTabBarL()
    {
    iAppList->SetHighlighted(-1);
    iTabBar->SetBarFocused(ETrue);
    UpdateTooltipL();
    }

void CWunderlandContainer::FocusListL()
    {
    if (iAppList->Count() == 0)
        {
        return;
        }
    iTabBar->SetBarFocused(EFalse);
    iAppList->SetHighlighted(0);
    UpdateTooltipL();
    }

// Under the focused tab, left-aligned with it, kept on screen.
void CWunderlandContainer::UpdateTooltipL()
    {
    const TBool show = iTabBar->IsBarFocused();
    if (show)
        {
        iTooltip->SetTextL(iTabBar->FocusedLabel());
        const TRect tab(iTabBar->TabRect(iTabBar->Focused()));
        const TSize size(iTooltip->PreferredSize());
        TInt x = tab.iTl.iX;
        if (x + size.iWidth > Rect().iBr.iX)
            {
            x = Rect().iBr.iX - size.iWidth;
            }
        iTooltip->SetRect(TRect(TPoint(x, iTabBar->Rect().iBr.iY), size));
        }
    if (show || iTooltip->IsVisible())
        {
        iTooltip->MakeVisible(show);
        DrawDeferred();   // repaints the list where the tooltip was
        }
    }

void CWunderlandContainer::SizeChanged()
    {
    if (!iTabBar)
        {
        return;
        }
    const TRect rect(Rect());
    iTabBar->SetRect(TRect(rect.iTl, TSize(rect.Width(), iTabBar->PreferredHeight(rect.Size()))));
    iAppList->SetRect(ContentRect());
    TRAP_IGNORE(UpdateTooltipL());   // only leaves on OOM; the old tooltip stays
    }

void CWunderlandContainer::HandleResourceChange(TInt aType)
    {
    CCoeControl::HandleResourceChange(aType);   // forwards to the child controls
    if (aType == KEikDynamicLayoutVariantSwitch)
        {
        SizeChanged();   // bar height and fonts depend on the layout
        DrawDeferred();
        }
    }

TInt CWunderlandContainer::CountComponentControls() const
    {
    return iTabBar ? 3 : 0;
    }

CCoeControl* CWunderlandContainer::ComponentControl(TInt aIndex) const
    {
    switch (aIndex)
        {
        case 0:
            return iTabBar;
        case 1:
            return iAppList;
        case 2:
            return iTooltip;   // last: drawn over the list
        default:
            return NULL;
        }
    }

void CWunderlandContainer::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
    {
    if (aEventType != EEventStateChanged)
        {
        return;
        }
    if (aControl == iTabBar)
        {
        // Focus moved or a tab was activated (key or tap)
        if (iTabBar->IsBarFocused())
            {
            iAppList->SetHighlighted(-1);
            }
        UpdateTooltipL();
        }
    else if (aControl == iAppList)
        {
        // A tapped row takes the focus
        iTabBar->SetBarFocused(EFalse);
        UpdateTooltipL();
        }
    }

void CWunderlandContainer::Draw(const TRect& /*aRect*/) const
    {
    // The child controls cover everything; this only keeps stale pixels
    // from showing through under the list.
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KRgbWhite);
    gc.DrawRect(ContentRect());
    }

TKeyResponse CWunderlandContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (aType != EEventKey)
        {
        return EKeyWasNotConsumed;
        }
    // Focus runs tab bar -> list rows; up/down move it.
    if (iTabBar->IsBarFocused())
        {
        if (aKeyEvent.iCode == EKeyDownArrow)
            {
            FocusListL();
            return EKeyWasConsumed;
            }
        return iTabBar->OfferKeyEventL(aKeyEvent, aType);   // left/right/select
        }
    switch (aKeyEvent.iCode)
        {
        case EKeyDownArrow:
            if (iAppList->Highlighted() < 0)
                {
                FocusListL();
                }
            else
                {
                iAppList->MoveHighlight(1);   // stays on the last row
                }
            return EKeyWasConsumed;
        case EKeyUpArrow:
            if (iAppList->Highlighted() < 0 || !iAppList->MoveHighlight(-1))
                {
                FocusTabBarL();   // first row: up into the tab bar
                }
            return EKeyWasConsumed;
        default:
            return EKeyWasNotConsumed;
        }
    }
