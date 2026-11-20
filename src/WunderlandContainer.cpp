#include <akndef.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <eikbtgpc.h>
#include <eikon.hrh>
#include <e32math.h>
#include <gdi.h>
#include "WunderlandAppList.h"
#include "WunderlandCategoryList.h"
#include "WunderlandContainer.h"
#include "WunderlandContentTypePage.h"
#include "WunderlandLE_0xE4F76EAD.rsg"
#include "WunderlandSearchPage.h"
#include "WunderlandSearchResultsPage.h"
#include "WunderlandTabBar.h"
#include "WunderlandTooltip.h"

namespace
    {
    const TInt KCategoryGranularity = 16;
    const TInt KContentTypes = 3;     // apps, games, themes
    const TInt KAppsPerType = 30;     // sample entries per content type
    const TInt KSampleSeed = 0x5EED;  // fixed: the same sample list on every start
    const TInt KTooltipTimeout = 1000000;   // microseconds the tab tooltip stays up
    const TInt KSampleResultStep = 5;       // every 5th entry is a sample search result

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
    : iOverlay(EOverlayNone), iOpenType(-1)
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
    iSearchPage = CWunderlandSearchPage::NewL(*this, *this);
    iSearchPage->SetObserver(this);
    iContentTypePage = CWunderlandContentTypePage::NewL(*this);
    iContentTypePage->SetObserver(this);
    iContentTypePage->MakeVisible(EFalse);
    iResultsPage = CWunderlandSearchResultsPage::NewL(*this, *this);
    iResultsPage->MakeVisible(EFalse);
    iTooltip = CWunderlandTooltip::NewL(*this);
    iTooltip->MakeVisible(EFalse);
    iTooltipTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    iContentTypes = new (ELeave) CDesCArrayFlat(KContentTypes);
    EnableDragEvents();   // touch scrolling of the list

    AddContentTypesL();
    AddSampleCategoriesL();
    AddSampleAppsL();
    UpdateAppListL();
    UpdatePage();

    SetRect(aRect);   // lays out the children via SizeChanged()
    FocusListL();     // start on the first entry, like Ovi Store
    ActivateL();
    // ActivateL() only reaches the page that is a component control right
    // now; a page that isn't activated ignores its own DrawDeferred() calls.
    iAppList->ActivateL();
    iCategoryList->ActivateL();
    iSearchPage->ActivateL();
    iContentTypePage->ActivateL();
    iResultsPage->ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    iApps.Close();
    iCategories.ResetAndDestroy();
    delete iContentTypes;
    if (iTooltipTimer)
        {
        iTooltipTimer->Cancel();
        }
    delete iTooltipTimer;
    delete iTooltip;
    delete iResultsPage;
    delete iContentTypePage;
    delete iSearchPage;
    delete iCategoryList;
    delete iAppList;
    delete iTabBar;
    }

// One category list per content type, in the order of iContentTypes.
// TODO: fetch the categories from the store API once there is one.
void CWunderlandContainer::AddSampleCategoriesL()
    {
    const TText* const apps[] =
        {
        _S("Camera, photos, videos"), _S("Emulator"), _S("Business"), _S("Communication"),
        _S("Entertainment"), _S("Maps & navigation"), _S("Music & audio"), _S("News & weather"),
        _S("Productivity"), _S("Social networks"), _S("Tools & utilities"), _S("Travel")
        };
    const TText* const games[] =
        {
        _S("Action"), _S("Adventure"), _S("Arcade"), _S("Board"), _S("Card & casino"),
        _S("Educational"), _S("Puzzle"), _S("Racing"), _S("Sports"), _S("Strategy")
        };
    const TText* const themes[] =
        {
        _S("Abstract"), _S("Animals"), _S("Cartoons"), _S("Holidays"), _S("Nature"),
        _S("Sports"), _S("Technology")
        };
    #define WUNDERLAND_COUNT(a) (static_cast<TInt>(sizeof(a) / sizeof((a)[0])))
    AddCategoriesL(apps, WUNDERLAND_COUNT(apps));
    AddCategoriesL(games, WUNDERLAND_COUNT(games));
    AddCategoriesL(themes, WUNDERLAND_COUNT(themes));
    #undef WUNDERLAND_COUNT
    }

void CWunderlandContainer::AddCategoriesL(const TText* const* aNames, TInt aCount)
    {
    CDesCArrayFlat* list = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    CleanupStack::PushL(list);
    for (TInt i = 0; i < aCount; ++i)
        {
        list->AppendL(TPtrC(aNames[i]));
        }
    iCategories.AppendL(list);
    CleanupStack::Pop(list);
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
    for (TInt type = 0; type < KContentTypes; ++type)
        {
        const TInt categories = iCategories[type]->MdcaCount();
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
            app.iType = type;
            app.iCategory = categories > 0 ? Random(seed, categories) : 0;
            app.iRating = 1 + Random(seed, 5);
            iApps.AppendL(app);
            }
        }
    #undef WUNDERLAND_COUNT
    }

// Every entry (the Home page).
void CWunderlandContainer::UpdateAppListL()
    {
    iAppList->Reset();
    const TInt count = iApps.Count();
    for (TInt i = 0; i < count; ++i)
        {
        AppendAppL(*iAppList, i);
        }
    }

// One row: the entry's name, its category under it, its rating.
void CWunderlandContainer::AppendAppL(CWunderlandAppList& aList, TInt aIndex) const
    {
    const TApp& app = iApps[aIndex];
    TPtrC subtitle(KNullDesC);
    if (app.iType >= 0 && app.iType < iCategories.Count()
        && app.iCategory >= 0 && app.iCategory < iCategories[app.iType]->MdcaCount())
        {
        subtitle.Set(iCategories[app.iType]->MdcaPoint(app.iCategory));
        }
    aList.AppendL(app.iName, subtitle, app.iRating);
    }

// The same fixed sample results for any query: every few entries, from all
// content types.
// TODO: ask the store API for aQuery once there is one.
void CWunderlandContainer::UpdateSearchResultsL(const TDesC& /*aQuery*/)
    {
    CWunderlandAppList& list = iResultsPage->List();
    list.Reset();
    const TInt count = iApps.Count();
    for (TInt i = 0; i < count; i += KSampleResultStep)
        {
        AppendAppL(list, i);
        }
    }

// TODO: fetch the content types from the store API once there is one.
void CWunderlandContainer::AddContentTypesL()
    {
    _LIT(KTypeApps,   "Apps");
    _LIT(KTypeGames,  "Games");
    _LIT(KTypeThemes, "Themes");
    iContentTypes->Reset();
    iContentTypes->AppendL(KTypeApps);
    iContentTypes->AppendL(KTypeGames);
    iContentTypes->AppendL(KTypeThemes);
    iCategoryList->Reset();
    const TInt count = iContentTypes->MdcaCount();
    for (TInt i = 0; i < count; ++i)
        {
        iCategoryList->AppendL(iContentTypes->MdcaPoint(i));
        }
    }

void CWunderlandContainer::HandleBackL()
    {
    switch (iOverlay)
        {
        case EOverlayContentType:
            CloseContentTypeL();
            break;
        case EOverlaySearchResults:
            CloseSearchResultsL();
            break;
        default:
            break;
        }
    }

// Covers the tab bar and the tab pages; the Categories tab stays active
// underneath with the opened row highlighted, ready for Back.
void CWunderlandContainer::OpenContentTypeL(TInt aType)
    {
    if (aType < 0 || aType >= iContentTypes->MdcaCount() || aType >= iCategories.Count())
        {
        return;
        }
    iContentTypePage->SetContentL(iContentTypes->MdcaPoint(aType), *iCategories[aType]);
    SetSoftkeysL(R_WUNDERLAND_SOFTKEYS_STOREMENU_BACK);
    iOverlay = EOverlayContentType;
    iOpenType = aType;
    HideTooltip();
    iContentTypePage->MakeVisible(ETrue);
    iContentTypePage->SetHighlighted(0);
    DrawDeferred();
    }

void CWunderlandContainer::CloseContentTypeL()
    {
    if (iOverlay != EOverlayContentType)
        {
        return;
        }
    SetSoftkeysL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    iContentTypePage->SetHighlighted(-1);
    iContentTypePage->MakeVisible(EFalse);
    iTabBar->SetBarFocused(EFalse);
    iCategoryList->SetHighlighted(iOpenType);
    iOverlay = EOverlayNone;
    iOpenType = -1;
    DrawDeferred();
    }

// From the Search tab or from the results page itself (a new search).
// Empty queries are ignored.
void CWunderlandContainer::HandleSearchL(const TDesC& aQuery)
    {
    TBuf<KWunderlandMaxQuery> query(aQuery.Left(KWunderlandMaxQuery));
    query.Trim();
    if (query.Length() == 0)
        {
        return;
        }
    OpenSearchResultsL(query);
    }

// Covers the tab bar and the tab pages; the Search tab stays active
// underneath, ready for Back. Both search bars keep the last query.
void CWunderlandContainer::OpenSearchResultsL(const TDesC& aQuery)
    {
    iSearchPage->SetQueryL(aQuery);
    iResultsPage->SetQueryL(aQuery);
    UpdateSearchResultsL(aQuery);
    if (iOverlay != EOverlaySearchResults)
        {
        iSearchPage->SetHighlighted(-1);   // only one editor may have the focus
        SetSoftkeysL(R_WUNDERLAND_SOFTKEYS_STOREMENU_BACK);
        iOverlay = EOverlaySearchResults;
        HideTooltip();
        iResultsPage->MakeVisible(ETrue);
        }
    // First result, or the search bar when there are none
    iResultsPage->SetHighlighted(iResultsPage->Count() > 1 ? 1 : 0);
    DrawDeferred();
    }

void CWunderlandContainer::CloseSearchResultsL()
    {
    if (iOverlay != EOverlaySearchResults)
        {
        return;
        }
    SetSoftkeysL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    iResultsPage->SetHighlighted(-1);
    iResultsPage->MakeVisible(EFalse);
    iOverlay = EOverlayNone;
    iTabBar->SetBarFocused(EFalse);
    iSearchPage->SetHighlighted(0);   // back in the field
    DrawDeferred();
    }

void CWunderlandContainer::SetSoftkeysL(TInt aResourceId)
    {
    CEikButtonGroupContainer* cba = CEikButtonGroupContainer::Current();
    if (cba)
        {
        cba->SetCommandSetL(aResourceId);
        cba->DrawDeferred();
        }
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
    iContentTypePage->SetRect(rect);   // no tab bar over them
    iResultsPage->SetRect(rect);
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
    if (!iTabBar)
        {
        return 0;
        }
    return iOverlay != EOverlayNone ? 1 : 3;   // a full-screen page covers everything
    }

CCoeControl* CWunderlandContainer::ComponentControl(TInt aIndex) const
    {
    if (iOverlay != EOverlayNone)
        {
        if (aIndex != 0)
            {
            return NULL;
            }
        if (iOverlay == EOverlayContentType)
            {
            return iContentTypePage;
            }
        return iResultsPage;
        }
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
        if (aControl == iCategoryList)
            {
            OpenContentTypeL(iCategoryList->Highlighted());   // one tap opens, like Ovi Store
            }
        }
    // A tapped row on iContentTypePage is only highlighted.
    // TODO: open the category's entries once there is a page for them.
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
    // A focused search bar gets the keys first (text, cursor, Go); it leaves
    // Up/Down and the keys at its edges to the navigation below.
    CCoeControl* searchPage = NULL;
    if (iOverlay == EOverlaySearchResults)
        {
        searchPage = iResultsPage;
        }
    else if (iOverlay == EOverlayNone && iTabBar->Active() == CWunderlandTabBar::ETabSearch
             && !iTabBar->IsBarFocused())
        {
        searchPage = iSearchPage;
        }
    if (searchPage && searchPage->OfferKeyEventL(aKeyEvent, aType) == EKeyWasConsumed)
        {
        return EKeyWasConsumed;
        }
    if (aType != EEventKey)
        {
        return EKeyWasNotConsumed;
        }
    if (iOverlay == EOverlaySearchResults)
        {
        // Up/down through the bar and the results; Back is the right softkey.
        // TODO: OK opens the highlighted entry.
        switch (aKeyEvent.iCode)
            {
            case EKeyDownArrow:
            case EKeyUpArrow:
                if (iResultsPage->Highlighted() < 0)
                    {
                    iResultsPage->SetHighlighted(0);
                    }
                else
                    {
                    // No wrapping: stays on the bar / the last result
                    iResultsPage->MoveHighlight(aKeyEvent.iCode == EKeyDownArrow ? 1 : -1);
                    }
                return EKeyWasConsumed;
            default:
                return EKeyWasNotConsumed;
            }
        }
    if (iOverlay == EOverlayContentType)
        {
        // Content type page: up/down only; Back is the right softkey.
        // TODO: OK opens the highlighted category's entries.
        switch (aKeyEvent.iCode)
            {
            case EKeyDownArrow:
            case EKeyUpArrow:
                if (iContentTypePage->Highlighted() < 0)
                    {
                    iContentTypePage->SetHighlighted(0);
                    }
                else
                    {
                    // No wrapping: stays on the first / last row
                    iContentTypePage->MoveHighlight(aKeyEvent.iCode == EKeyDownArrow ? 1 : -1);
                    }
                return EKeyWasConsumed;
            default:
                return EKeyWasNotConsumed;
            }
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
    // that need these keys themselves (the search bar above, a future category
    // drop-down) must be offered the key before this point and get to keep it.
    switch (aKeyEvent.iCode)
        {
        case EKeyOK:
        case EKeyEnter:
            if (iTabBar->Active() == CWunderlandTabBar::ETabCategories && Page().Highlighted() >= 0)
                {
                OpenContentTypeL(Page().Highlighted());
                return EKeyWasConsumed;
                }
            return EKeyWasNotConsumed;
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
