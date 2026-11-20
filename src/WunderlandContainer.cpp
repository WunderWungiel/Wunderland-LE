#include <akndef.h>
#include <avkon.hrh>
#include <eikon.hrh>
#include <e32math.h>
#include <gdi.h>
#include "WunderlandAppList.h"
#include "WunderlandCategoryList.h"
#include "WunderlandContainer.h"
#include "WunderlandSearchPage.h"
#include "WunderlandTabBar.h"
#include "WunderlandTooltip.h"

namespace
    {
    const TInt KCategoryGranularity = 16;
    const TInt KContentTypes = 3;     // sample apps, games, themes
    const TInt KAppsPerType = 30;     // sample entries per content type
    const TInt KSampleSeed = 0x5EED;  // fixed: the same sample list on every start
    const TInt KTooltipTimeout = 1000000;   // microseconds the tab tooltip stays up

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
    iCategoryList = CWunderlandCategoryList::NewL(*this);
    iCategoryList->SetObserver(this);
    iSearchPage = CWunderlandSearchPage::NewL(*this);
    iSearchPage->SetObserver(this);
    iTooltip = CWunderlandTooltip::NewL(*this);
    iTooltip->MakeVisible(EFalse);
    iTooltipTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    iCategories = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    EnableDragEvents();   // touch scrolling of the list

    AddSampleCategoriesL();
    AddSampleAppsL();
    UpdateAppListL();
    AddContentTypesL();
    UpdatePage();

    SetRect(aRect);   // lays out the children via SizeChanged()
    FocusListL();     // start on the first entry, like Ovi Store
    ActivateL();
    // ActivateL() only reaches the page that is a component control right
    // now; a page that isn't activated ignores its own DrawDeferred() calls.
    iAppList->ActivateL();
    iCategoryList->ActivateL();
    iSearchPage->ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    iApps.Close();
    delete iCategories;
    if (iTooltipTimer)
        {
        iTooltipTimer->Cancel();
        }
    delete iTooltipTimer;
    delete iTooltip;
    delete iSearchPage;
    delete iCategoryList;
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

// Every entry, with its category under the name (the Home page).
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

// TODO: fetch the content types from the store API once there is one.
void CWunderlandContainer::AddContentTypesL()
    {
    _LIT(KTypeApps,   "Apps");
    _LIT(KTypeGames,  "Games");
    _LIT(KTypeThemes, "Themes");
    iCategoryList->Reset();
    iCategoryList->AppendL(KTypeApps);
    iCategoryList->AppendL(KTypeGames);
    iCategoryList->AppendL(KTypeThemes);
    }

MWunderlandPage& CWunderlandContainer::Page() const
    {
    switch (iTabBar->Active())
        {
        case CWunderlandTabBar::ETabCategories:
            return *iCategoryList;
        case CWunderlandTabBar::ETabSearch:
            return *iSearchPage;
        default:
            return *iAppList;
        }
    }

CCoeControl* CWunderlandContainer::PageControl() const
    {
    switch (iTabBar->Active())
        {
        case CWunderlandTabBar::ETabCategories:
            return iCategoryList;
        case CWunderlandTabBar::ETabSearch:
            return iSearchPage;
        default:
            return iAppList;
        }
    }

// Only the active page is a component control, so only it is drawn and
// gets pointer events. The others drop their highlight.
void CWunderlandContainer::UpdatePage()
    {
    const TInt active = iTabBar->Active();
    iAppList->MakeVisible(active == CWunderlandTabBar::ETabHome);
    iCategoryList->MakeVisible(active == CWunderlandTabBar::ETabCategories);
    iSearchPage->MakeVisible(active == CWunderlandTabBar::ETabSearch);
    if (active != CWunderlandTabBar::ETabHome)
        {
        iAppList->SetHighlighted(-1);
        }
    if (active != CWunderlandTabBar::ETabCategories)
        {
        iCategoryList->SetHighlighted(-1);
        }
    if (active != CWunderlandTabBar::ETabSearch)
        {
        iSearchPage->SetHighlighted(-1);
        }
    DrawDeferred();
    }

TRect CWunderlandContainer::ContentRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl.iX, iTabBar->Rect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    }

void CWunderlandContainer::FocusTabBarL()
    {
    Page().SetHighlighted(-1);
    iTabBar->SetBarFocused(ETrue);
    UpdateTooltipL();
    }

void CWunderlandContainer::FocusListL()
    {
    if (Page().Count() == 0)
        {
        return;
        }
    iTabBar->SetBarFocused(EFalse);
    Page().SetHighlighted(0);
    UpdateTooltipL();
    }

// Right under the focused tab and as wide as it, text centred. Wider only if
// the text doesn't fit; then centred on the tab and kept on screen.
void CWunderlandContainer::UpdateTooltipL(TBool aRestart)
    {
    const TBool show = iTabBar->IsBarFocused() && (aRestart || iTooltip->IsVisible());
    if (show)
        {
        iTooltip->SetTextL(iTabBar->FocusedLabel());
        const TRect tab(iTabBar->TabRect(iTabBar->Focused()));
        TSize size(iTooltip->PreferredSize());
        TInt x = tab.iTl.iX;
        if (size.iWidth <= tab.Width())
            {
            size.iWidth = tab.Width();
            }
        else
            {
            x = tab.iTl.iX - (size.iWidth - tab.Width()) / 2;
            if (x + size.iWidth > Rect().iBr.iX)
                {
                x = Rect().iBr.iX - size.iWidth;
                }
            if (x < Rect().iTl.iX)
                {
                x = Rect().iTl.iX;
                }
            }
        iTooltip->SetRect(TRect(TPoint(x, iTabBar->Rect().iBr.iY), size));
        }
    iTooltipTimer->Cancel();
    if (show && aRestart)
        {
        iTooltipTimer->Start(KTooltipTimeout, KTooltipTimeout,
                             TCallBack(CWunderlandContainer::HideTooltipCallBack, this));
        }
    if (show || iTooltip->IsVisible())
        {
        iTooltip->MakeVisible(show);
        DrawDeferred();   // repaints the list where the tooltip was
        }
    }

TInt CWunderlandContainer::HideTooltipCallBack(TAny* aSelf)
    {
    static_cast<CWunderlandContainer*>(aSelf)->HideTooltip();
    return EFalse;
    }

void CWunderlandContainer::HideTooltip()
    {
    iTooltipTimer->Cancel();   // CPeriodic repeats; one tick is enough
    if (iTooltip->IsVisible())
        {
        iTooltip->MakeVisible(EFalse);
        DrawDeferred();
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
    iCategoryList->SetRect(ContentRect());
    iSearchPage->SetRect(ContentRect());
    TRAP_IGNORE(UpdateTooltipL(EFalse));   // only leaves on OOM; the old tooltip stays
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
            return PageControl();
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
        if (!PageControl()->IsVisible())
            {
            UpdatePage();   // the active tab changed
            }
        if (iTabBar->IsBarFocused())
            {
            Page().SetHighlighted(-1);
            }
        UpdateTooltipL();
        }
    else if (aControl == iAppList || aControl == iCategoryList || aControl == iSearchPage)
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
        const TKeyResponse response = iTabBar->OfferKeyEventL(aKeyEvent, aType);   // left/right/select
        if (aKeyEvent.iCode == EKeyOK || aKeyEvent.iCode == EKeyEnter)
            {
            // Selecting a tab (even the open one) goes to its first row.
            // An empty page has none, so the bar keeps the focus.
            FocusListL();
            }
        return response;
        }
    // In the page content, left/right switch tabs (S60 tab convention). Controls
    // that need these keys themselves (a future Search editor, an open category
    // drop-down) must be offered the key before this point and get to keep it.
    switch (aKeyEvent.iCode)
        {
        case EKeyLeftArrow:
        case EKeyRightArrow:
            if (iTabBar->SwitchTabL(aKeyEvent.iCode == EKeyLeftArrow ? -1 : 1))
                {
                FocusListL();   // focus stays in the content: first row of the new page
                }
            return EKeyWasConsumed;
        case EKeyDownArrow:
            if (Page().Highlighted() < 0)
                {
                FocusListL();
                }
            else
                {
                Page().MoveHighlight(1);   // stays on the last row
                }
            return EKeyWasConsumed;
        case EKeyUpArrow:
            if (Page().Highlighted() < 0 || !Page().MoveHighlight(-1))
                {
                FocusTabBarL();   // first row: up into the tab bar
                }
            return EKeyWasConsumed;
        default:
            return EKeyWasNotConsumed;
        }
    }
