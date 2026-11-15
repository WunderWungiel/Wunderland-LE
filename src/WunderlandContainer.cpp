#include <akndef.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <eikbtgpc.h>
#include <eikenv.h>
#include <eikon.hrh>
#include <e32math.h>
#include <gdi.h>
#include "WunderlandAppList.h"
#include "WunderlandCategoryBar.h"
#include "WunderlandContainer.h"
#include "WunderlandDropdown.h"
#include "WunderlandTabBar.h"

namespace
    {
    const TInt KCategoryGranularity = 16;
    const TInt KHomeTab = 0;
    const TInt KDropdownMargin = 4;   // drop-down inset from the screen edges
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
    iCategoryBar = CWunderlandCategoryBar::NewL(*this);
    iAppList = CWunderlandAppList::NewL(*this);
    iAppList->SetObserver(this);
    iDropdown = CWunderlandDropdown::NewL(*this);
    iDropdown->SetObserver(this);
    iCategories = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    EnableDragEvents();   // touch scrolling of the list

    AddSampleContentTypesL();
    AddSampleCategoriesL();
    AddSampleAppsL();
    UpdateCategoryBarL();
    UpdateAppListL();

    SetRect(aRect);   // lays out the bars via SizeChanged()
    ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    iSelectedCategory.Close();
    iApps.Close();
    delete iCategories;
    delete iDropdown;
    delete iAppList;
    delete iCategoryBar;
    delete iTabBar;
    }

// Sample data until the store has an API. "Home" always exists and comes first.
void CWunderlandContainer::AddSampleContentTypesL()
    {
    _LIT(KHome, "Home");
    _LIT(KApps, "Apps");
    _LIT(KGames, "Games");
    _LIT(KThemes, "Themes");
    iTabBar->AppendTabL(KHome);
    iTabBar->AppendTabL(KApps);
    iTabBar->AppendTabL(KGames);
    iTabBar->AppendTabL(KThemes);
    for (TInt i = 0; i < iTabBar->Count(); ++i)
        {
        iSelectedCategory.AppendL(0);
        }
    }

// The drop-down shows "All <type>" first, then these.
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

// Random names, publishers, categories and ratings, generated from a fixed
// seed. Home has no entries of its own: it lists everything.
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
    const TText* const publishers[] =
        {
        _S("Moonbit"), _S("Pixel Forge"), _S("Gizmo Studio"), _S("Northwind Soft"),
        _S("Bluebird Mobile"), _S("Wunder Apps")
        };
    #define WUNDERLAND_COUNT(a) (static_cast<TInt>(sizeof(a) / sizeof((a)[0])))

    TInt64 seed = KSampleSeed;
    const TInt types = iTabBar->Count();
    const TInt categories = iCategories->MdcaCount();
    for (TInt type = KHomeTab + 1; type < types; ++type)
        {
        const TText* const* first;
        const TText* const* second;
        TInt firstCount;
        TInt secondCount;
        switch (type)
            {
            case 1:   // Apps
                first = appFirst;     firstCount = WUNDERLAND_COUNT(appFirst);
                second = appSecond;   secondCount = WUNDERLAND_COUNT(appSecond);
                break;
            case 2:   // Games
                first = gameFirst;    firstCount = WUNDERLAND_COUNT(gameFirst);
                second = gameSecond;  secondCount = WUNDERLAND_COUNT(gameSecond);
                break;
            default:  // Themes
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
            const TPtrC publisher(publishers[Random(seed, WUNDERLAND_COUNT(publishers))]);
            app.iPublisher.Copy(publisher.Left(app.iPublisher.MaxLength()));
            app.iContentType = type;
            app.iCategory = categories > 0 ? Random(seed, categories) : 0;
            app.iRating = 1 + Random(seed, 5);
            iApps.AppendL(app);
            }
        }
    #undef WUNDERLAND_COUNT
    }

TBool CWunderlandContainer::HasCategoryBar() const
    {
    return iTabBar->Selected() != KHomeTab;
    }

void CWunderlandContainer::UpdateCategoryBarL()
    {
    const TBool show = HasCategoryBar();
    iCategoryBar->MakeVisible(show);
    if (!show)
        {
        iCategoryBar->SetHighlighted(EFalse);
        return;
        }

    // Items for this content type: "All <type, lower case>" + the categories
    const TPtrC type(iTabBar->SelectedLabel());
    _LIT(KAllPrefix, "All ");
    HBufC* all = HBufC::NewLC(KAllPrefix().Length() + type.Length());
    TPtr allPtr(all->Des());
    allPtr.Append(KAllPrefix);
    allPtr.Append(type);
    allPtr.MidTPtr(KAllPrefix().Length()).LowerCase();

    CDesCArrayFlat* items = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    CleanupStack::PushL(items);
    items->AppendL(*all);
    const TInt count = iCategories->MdcaCount();
    for (TInt i = 0; i < count; ++i)
        {
        items->AppendL(iCategories->MdcaPoint(i));
        }
    iDropdown->SetItemsL(*items);

    TInt& selected = iSelectedCategory[iTabBar->Selected()];
    if (selected < 0 || selected >= items->MdcaCount())
        {
        selected = 0;
        }
    iCategoryBar->SetPathL(type, items->MdcaPoint(selected));

    CleanupStack::PopAndDestroy(items);
    CleanupStack::PopAndDestroy(all);
    }

// Entries of the selected content type (all of them on Home), filtered by the
// selected category. The category is shown under the name only when the
// list isn't already limited to one category.
void CWunderlandContainer::UpdateAppListL()
    {
    iAppList->Reset();
    const TInt type = iTabBar->Selected();
    const TInt category = HasCategoryBar() ? iSelectedCategory[type] - 1 : -1;   // -1: all
    const TInt categories = iCategories->MdcaCount();
    const TInt count = iApps.Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TApp& app = iApps[i];
        if ((type != KHomeTab && app.iContentType != type) ||
            (category >= 0 && app.iCategory != category))
            {
            continue;
            }
        TPtrC subtitle(KNullDesC);
        if (category < 0 && app.iCategory >= 0 && app.iCategory < categories)
            {
            subtitle.Set(iCategories->MdcaPoint(app.iCategory));
            }
        iAppList->AppendL(app.iName, subtitle, app.iRating);
        }
    }

TRect CWunderlandContainer::ContentRect() const
    {
    const TRect rect(Rect());
    const TInt top = HasCategoryBar() ? iCategoryBar->Rect().iBr.iY : iTabBar->Rect().iBr.iY;
    return TRect(rect.iTl.iX, top, rect.iBr.iX, rect.iBr.iY);
    }

void CWunderlandContainer::OpenDropdownL()
    {
    if (!HasCategoryBar() || iDropdown->IsOpen())
        {
        return;
        }
    iAppList->SetHighlighted(-1);
    iCategoryBar->SetHighlighted(ETrue);
    TRect area(Rect().iTl.iX, iCategoryBar->Rect().iBr.iY, Rect().iBr.iX, Rect().iBr.iY);
    area.Shrink(KDropdownMargin, 0);
    area.iBr.iY -= KDropdownMargin;
    iDropdown->Open(area, iSelectedCategory[iTabBar->Selected()]);
    if (iDropdown->IsOpen())
        {
        SetSoftkeysL(R_AVKON_SOFTKEYS_SELECT_CANCEL);
        }
    }

void CWunderlandContainer::CloseDropdownL()
    {
    if (iDropdown->IsOpen())
        {
        iDropdown->Close();
        SetSoftkeysL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
        DrawDeferred();
        }
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

TBool CWunderlandContainer::HandleCommandL(TInt aCommand)
    {
    if (!iDropdown->IsOpen())
        {
        return EFalse;
        }
    switch (aCommand)
        {
        case EAknSoftkeySelect:
            iDropdown->ChooseL();   // comes back through HandleControlEventL
            return ETrue;
        case EAknSoftkeyCancel:
            CloseDropdownL();
            return ETrue;
        default:
            return EFalse;
        }
    }

void CWunderlandContainer::SizeChanged()
    {
    if (!iTabBar)
        {
        return;
        }
    const TRect rect(Rect());
    iTabBar->SetRect(TRect(rect.iTl, TSize(rect.Width(), iTabBar->PreferredHeight())));
    iCategoryBar->SetRect(TRect(TPoint(rect.iTl.iX, iTabBar->Rect().iBr.iY),
                                TSize(rect.Width(), iCategoryBar->PreferredHeight())));
    iAppList->SetRect(ContentRect());
    }

void CWunderlandContainer::HandleResourceChange(TInt aType)
    {
    CCoeControl::HandleResourceChange(aType);   // forwards to the child controls
    if (aType == KEikDynamicLayoutVariantSwitch)
        {
        TRAP_IGNORE(CloseDropdownL());   // its size depends on the old layout
        SizeChanged();   // layout fonts (and so the bar heights) may have changed
        DrawDeferred();
        }
    }

TInt CWunderlandContainer::CountComponentControls() const
    {
    return iTabBar ? 4 : 0;
    }

CCoeControl* CWunderlandContainer::ComponentControl(TInt aIndex) const
    {
    switch (aIndex)
        {
        case 0:
            return iTabBar;
        case 1:
            return iCategoryBar;
        case 2:
            return iAppList;
        case 3:
            return iDropdown;
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
        CloseDropdownL();
        UpdateCategoryBarL();
        UpdateAppListL();
        SizeChanged();    // the list starts higher on Home (no category bar)
        DrawDeferred();
        }
    else if (aControl == iDropdown)
        {
        iSelectedCategory[iTabBar->Selected()] = iDropdown->Highlighted();
        CloseDropdownL();
        UpdateCategoryBarL();
        UpdateAppListL();
        DrawDeferred();
        }
    else if (aControl == iAppList)
        {
        iCategoryBar->SetHighlighted(EFalse);   // a tapped row takes the focus
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
    if (iDropdown->IsOpen())
        {
        return iDropdown->OfferKeyEventL(aKeyEvent, aType);
        }
    if (aType == EEventKey)
        {
        // Focus runs tab bar -> category bar (not on Home) -> list rows.
        // Up/down move it; Select on the category bar opens the drop-down.
        const TBool inList = iAppList->Highlighted() >= 0;
        switch (aKeyEvent.iCode)
            {
            case EKeyDownArrow:
                if (inList)
                    {
                    iAppList->MoveHighlight(1);   // stays on the last row
                    }
                else if (HasCategoryBar() && !iCategoryBar->IsHighlighted())
                    {
                    iCategoryBar->SetHighlighted(ETrue);
                    }
                else if (iAppList->Count() > 0)
                    {
                    iCategoryBar->SetHighlighted(EFalse);
                    iAppList->SetHighlighted(0);
                    }
                return EKeyWasConsumed;
            case EKeyUpArrow:
                if (inList)
                    {
                    if (!iAppList->MoveHighlight(-1))   // first row: leave the list
                        {
                        iAppList->SetHighlighted(-1);
                        iCategoryBar->SetHighlighted(HasCategoryBar());
                        }
                    return EKeyWasConsumed;
                    }
                if (iCategoryBar->IsHighlighted())
                    {
                    iCategoryBar->SetHighlighted(EFalse);
                    return EKeyWasConsumed;
                    }
                break;
            case EKeyOK:
            case EKeyEnter:
                if (HasCategoryBar() && iCategoryBar->IsHighlighted())
                    {
                    OpenDropdownL();
                    return EKeyWasConsumed;
                    }
                break;
            default:
                break;
            }
        }
    return iTabBar->OfferKeyEventL(aKeyEvent, aType);   // left/right; softkeys etc. fall through
    }

void CWunderlandContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
    // Only delivered on touch devices (S60 5th Edition). Taps inside the open
    // drop-down go to its own window; anything reaching us closes it.
    if (iDropdown->IsOpen())
        {
        if (aPointerEvent.iType == TPointerEvent::EButton1Down)
            {
            CloseDropdownL();
            iSwallowPointer = ETrue;
            }
        return;
        }
    if (iSwallowPointer)
        {
        if (aPointerEvent.iType == TPointerEvent::EButton1Up)
            {
            iSwallowPointer = EFalse;
            }
        return;
        }
    if (HasCategoryBar() && iCategoryBar->Rect().Contains(aPointerEvent.iPosition))
        {
        // Open on release: the up event would otherwise land on us and close it.
        if (aPointerEvent.iType == TPointerEvent::EButton1Down)
            {
            iCategoryBar->SetHighlighted(ETrue);
            }
        else if (aPointerEvent.iType == TPointerEvent::EButton1Up)
            {
            OpenDropdownL();
            }
        return;
        }
    CCoeControl::HandlePointerEventL(aPointerEvent);   // tab bar, list
    }
