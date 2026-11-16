#include <gdi.h>
#include <w32std.h>
#include "WunderlandSearchPage.h"

CWunderlandSearchPage* CWunderlandSearchPage::NewL(const CCoeControl& aParent)
    {
    CWunderlandSearchPage* self = new (ELeave) CWunderlandSearchPage;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandSearchPage::CWunderlandSearchPage()
    {
    }

void CWunderlandSearchPage::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    }

CWunderlandSearchPage::~CWunderlandSearchPage()
    {
    }

TInt CWunderlandSearchPage::Count() const
    {
    return 0;
    }

TInt CWunderlandSearchPage::Highlighted() const
    {
    return -1;
    }

void CWunderlandSearchPage::SetHighlighted(TInt /*aIndex*/)
    {
    }

TBool CWunderlandSearchPage::MoveHighlight(TInt /*aDelta*/)
    {
    return EFalse;
    }

void CWunderlandSearchPage::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KRgbWhite);
    gc.DrawRect(Rect());
    }
