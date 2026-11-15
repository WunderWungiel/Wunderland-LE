#include <akndef.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <eikbtgpc.h>
#include <eikenv.h>
#include <eikon.hrh>
#include <gdi.h>
#include "WunderlandCategoryBar.h"
#include "WunderlandContainer.h"
#include "WunderlandDropdown.h"
#include "WunderlandTabBar.h"

namespace
    {
    const TInt KCategoryGranularity = 16;
    const TInt KHomeTab = 0;
    const TInt KDropdownMargin = 4;   // drop-down inset from the screen edges
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
    iDropdown = CWunderlandDropdown::NewL(*this);
    iDropdown->SetObserver(this);
    iCategories = new (ELeave) CDesCArrayFlat(KCategoryGranularity);

    AddSampleContentTypesL();
    AddSampleCategoriesL();
    UpdateCategoryBarL();

    SetRect(aRect);   // lays out the bars via SizeChanged()
    ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    iSelectedCategory.Close();
    delete iCategories;
    delete iDropdown;
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

void CWunderlandContainer::OpenDropdownL()
    {
    if (!HasCategoryBar() || iDropdown->IsOpen())
        {
        return;
        }
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
    return iTabBar ? 3 : 0;
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
        DrawDeferred();   // content and layout depend on the content type
        }
    else if (aControl == iDropdown)
        {
        iSelectedCategory[iTabBar->Selected()] = iDropdown->Highlighted();
        CloseDropdownL();
        UpdateCategoryBarL();
        DrawDeferred();
        }
    }

void CWunderlandContainer::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    const TInt top = HasCategoryBar() ? iCategoryBar->Rect().iBr.iY : iTabBar->Rect().iBr.iY;
    const TRect content(rect.iTl.iX, top, rect.iBr.iX, rect.iBr.iY);

    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KRgbWhite);
    gc.DrawRect(content);

    // Placeholder content: name of the selected content type.
    const CFont* font = CEikonEnv::Static()->NormalFont();
    gc.UseFont(font);
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KRgbBlack);
    gc.SetBrushStyle(CGraphicsContext::ENullBrush);
    const TInt baseline = content.Height() / 2;
    gc.DrawText(iTabBar->SelectedLabel(), content, baseline, CGraphicsContext::ECenter);
    gc.DiscardFont();
    }

TKeyResponse CWunderlandContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (iDropdown->IsOpen())
        {
        return iDropdown->OfferKeyEventL(aKeyEvent, aType);
        }
    if (aType == EEventKey && HasCategoryBar())
        {
        // Up/down move the highlight between the tab bar and the category
        // bar; Select on the category bar opens the drop-down.
        switch (aKeyEvent.iCode)
            {
            case EKeyDownArrow:
                if (!iCategoryBar->IsHighlighted())
                    {
                    iCategoryBar->SetHighlighted(ETrue);
                    return EKeyWasConsumed;
                    }
                break;
            case EKeyUpArrow:
                if (iCategoryBar->IsHighlighted())
                    {
                    iCategoryBar->SetHighlighted(EFalse);
                    return EKeyWasConsumed;
                    }
                break;
            case EKeyOK:
            case EKeyEnter:
                if (iCategoryBar->IsHighlighted())
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
    CCoeControl::HandlePointerEventL(aPointerEvent);   // tab bar
    }
