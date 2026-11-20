#include <aknutils.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandSearchPage.h"

namespace
    {
    const TInt KHintLineGap = 2;    // extra space between the hint's lines

    _LIT(KHint, "Search for apps, games, themes and more in Wunderland Store!");

    const TRgb KPageColor()         { return KRgbWhite; }
    const TRgb KHintColor()         { return TRgb(0x5A, 0x5A, 0x5A); }
    }

CWunderlandSearchPage* CWunderlandSearchPage::NewL(const CCoeControl& aParent,
                                                   MWunderlandSearchObserver& aObserver)
    {
    CWunderlandSearchPage* self = new (ELeave) CWunderlandSearchPage(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandSearchPage::CWunderlandSearchPage(MWunderlandSearchObserver& aObserver)
    : iObserver(aObserver)
    {
    }

void CWunderlandSearchPage::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iBar = CWunderlandSearchBar::NewL(*this, *this);
    iHint = KHint().AllocL();
    iHintLines = new (ELeave) CArrayFixFlat<TPtrC>(4);
    }

CWunderlandSearchPage::~CWunderlandSearchPage()
    {
    delete iHintLines;
    delete iHint;
    delete iBar;
    }

void CWunderlandSearchPage::SetQueryL(const TDesC& aQuery)
    {
    iBar->SetQueryL(aQuery);
    }

TInt CWunderlandSearchPage::Count() const
    {
    return 1;   // the search bar
    }

TInt CWunderlandSearchPage::Highlighted() const
    {
    return iBar->FocusSpot() != CWunderlandSearchBar::EFocusNone ? 0 : -1;
    }

void CWunderlandSearchPage::SetHighlighted(TInt aIndex)
    {
    if (aIndex == 0)
        {
        if (iBar->FocusSpot() == CWunderlandSearchBar::EFocusNone)
            {
            iBar->SetFocusSpot(CWunderlandSearchBar::EFocusField);
            }
        }
    else
        {
        iBar->SetFocusSpot(CWunderlandSearchBar::EFocusNone);
        }
    }

TBool CWunderlandSearchPage::MoveHighlight(TInt /*aDelta*/)
    {
    return EFalse;   // one row: Up leaves for the tab bar, Down stays
    }

TKeyResponse CWunderlandSearchPage::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    return iBar->OfferKeyEventL(aKeyEvent, aType);
    }

void CWunderlandSearchPage::HandleSearchBarTappedL(CWunderlandSearchBar& /*aBar*/)
    {
    ReportEventL(MCoeControlObserver::EEventStateChanged);   // the content takes the focus
    }

void CWunderlandSearchPage::HandleSearchRequestL(CWunderlandSearchBar& aBar)
    {
    TBuf<KWunderlandMaxQuery> query;
    aBar.GetQuery(query);
    iObserver.HandleSearchL(query);
    }

TInt CWunderlandSearchPage::CountComponentControls() const
    {
    return iBar ? 1 : 0;
    }

CCoeControl* CWunderlandSearchPage::ComponentControl(TInt aIndex) const
    {
    return aIndex == 0 ? iBar : NULL;
    }

TRect CWunderlandSearchPage::BodyRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl.iX, iBar->Rect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    }

void CWunderlandSearchPage::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect body(BodyRect());
    gc.SetClippingRect(body);
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KPageColor());
    gc.DrawRect(body);
    DrawHint(gc);
    gc.CancelClippingRect();
    }

// The fonts follow the layout, which always comes with a SetRect(), so the
// bar is resized and the hint wrapped again here. If wrapping fails, the
// hint is simply not shown.
void CWunderlandSearchPage::SizeChanged()
    {
    if (!iBar)
        {
        return;
        }
    const TRect rect(Rect());
    iBar->SetRect(TRect(rect.iTl, TSize(rect.Width(), iBar->PreferredHeight())));
    TRAPD(err, WrapHintL());
    if (err != KErrNone)
        {
        iHintLines->Reset();
        }
    }

const CFont* CWunderlandSearchPage::HintFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimaryFont);
    }

// About four fifths of the page, like a centred paragraph.
TInt CWunderlandSearchPage::HintLineWidth() const
    {
    return Rect().Width() * 4 / 5;
    }

void CWunderlandSearchPage::WrapHintL()
    {
    iHintLines->Reset();
    const TInt width = HintLineWidth();
    if (width <= 0)
        {
        return;
        }
    // WrapToArrayL() writes into the text it wraps, even through a const
    // TDesC&: given the KHint literal (read-only memory) it panics KERN-EXEC 3.
    // So it gets a heap copy, refreshed each time in case the last wrap
    // changed it. The lines point into iHint, which lives as long as they do.
    TPtr hint(iHint->Des());
    hint.Copy(KHint);
    AknTextUtils::WrapToArrayL(hint, width, *HintFont(), *iHintLines);
    }

// Centred horizontally on the page and vertically in the space below the bar.
void CWunderlandSearchPage::DrawHint(CWindowGc& aGc) const
    {
    const TInt count = iHintLines->Count();
    if (count == 0)
        {
        return;
        }
    const CFont* font = HintFont();
    const TInt lineH = font->HeightInPixels() + KHintLineGap;
    const TRect body(BodyRect());
    const TInt blockH = count * lineH - KHintLineGap;
    TInt top = (body.Height() - blockH) / 2;
    if (top < 0)
        {
        top = 0;   // too short: keep the first lines under the bar
        }

    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KHintColor());
    TRect line(body.iTl.iX, body.iTl.iY + top, body.iBr.iX, 0);
    for (TInt i = 0; i < count; ++i)
        {
        line.iBr.iY = line.iTl.iY + font->HeightInPixels();
        aGc.DrawText((*iHintLines)[i], line, font->AscentInPixels(), CGraphicsContext::ECenter);
        line.iTl.iY += lineH;
        }
    aGc.DiscardFont();
    }
