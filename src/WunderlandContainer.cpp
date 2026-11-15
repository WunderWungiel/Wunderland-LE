#include <eikenv.h>
#include <gdi.h>
#include "WunderlandContainer.h"

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
    SetRect(aRect);
    ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    }

void CWunderlandContainer::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KRgbWhite);
    gc.Clear(Rect());

    _LIT(KPlaceholder, "WunderlandLE");
    const CFont* font = CEikonEnv::Static()->NormalFont();
    gc.UseFont(font);
    gc.SetPenColor(KRgbBlack);
    const TInt baseline = Rect().Height() / 2;
    gc.DrawText(KPlaceholder, Rect(), baseline, CGraphicsContext::ECenter);
    gc.DiscardFont();
    }

TKeyResponse CWunderlandContainer::OfferKeyEventL(const TKeyEvent& /*aKeyEvent*/, TEventCode /*aType*/)
    {
    return EKeyWasNotConsumed;   // let the framework handle softkeys etc.
    }
