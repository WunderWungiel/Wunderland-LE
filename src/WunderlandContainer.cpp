#include <akndef.h>
#include <eikenv.h>
#include <gdi.h>
#include "WunderlandContainer.h"
#include "WunderlandTabBar.h"

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
    AddSampleCategoriesL();

    SetRect(aRect);   // lays out the tab bar via SizeChanged()
    ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    delete iTabBar;
    }

// Sample data until the store has an API. "Home" always exists and comes first.
void CWunderlandContainer::AddSampleCategoriesL()
    {
    _LIT(KHome, "Home");
    _LIT(KApps, "Apps");
    _LIT(KGames, "Games");
    _LIT(KThemes, "Themes");
    iTabBar->AppendTabL(KHome);
    iTabBar->AppendTabL(KApps);
    iTabBar->AppendTabL(KGames);
    iTabBar->AppendTabL(KThemes);
    }

void CWunderlandContainer::SizeChanged()
    {
    if (iTabBar)
        {
        const TRect rect(Rect());
        iTabBar->SetRect(TRect(rect.iTl, TSize(rect.Width(), iTabBar->PreferredHeight())));
        }
    }

void CWunderlandContainer::HandleResourceChange(TInt aType)
    {
    CCoeControl::HandleResourceChange(aType);   // forwards to the tab bar
    if (aType == KEikDynamicLayoutVariantSwitch)
        {
        SizeChanged();   // layout fonts (and so the bar height) may have changed
        DrawDeferred();
        }
    }

TInt CWunderlandContainer::CountComponentControls() const
    {
    return iTabBar ? 1 : 0;
    }

CCoeControl* CWunderlandContainer::ComponentControl(TInt aIndex) const
    {
    return aIndex == 0 ? iTabBar : NULL;
    }

void CWunderlandContainer::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
    {
    if (aControl == iTabBar && aEventType == EEventStateChanged)
        {
        DrawDeferred();   // content depends on the selected category
        }
    }

void CWunderlandContainer::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    const TRect content(rect.iTl.iX, iTabBar->Rect().iBr.iY, rect.iBr.iX, rect.iBr.iY);

    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KRgbWhite);
    gc.DrawRect(content);

    // Placeholder content: name of the selected category.
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
    return iTabBar->OfferKeyEventL(aKeyEvent, aType);   // softkeys etc. fall through
    }
