#include <aknutils.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandTooltip.h"

namespace
    {
    const TInt KHPad = 5;
    const TInt KVPad = 2;

    const TRgb KBackColor() { return TRgb(0x2C, 0x2F, 0x34); }   // darker than the focused tab
    const TRgb KTextColor() { return TRgb(0xC4, 0xC8, 0xCE); }
    }

CWunderlandTooltip* CWunderlandTooltip::NewL(const CCoeControl& aParent)
    {
    CWunderlandTooltip* self = new (ELeave) CWunderlandTooltip;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandTooltip::CWunderlandTooltip()
    {
    }

void CWunderlandTooltip::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iText = KNullDesC().AllocL();
    }

CWunderlandTooltip::~CWunderlandTooltip()
    {
    delete iText;
    }

void CWunderlandTooltip::SetTextL(const TDesC& aText)
    {
    HBufC* text = aText.AllocL();
    delete iText;
    iText = text;
    DrawDeferred();
    }

const CFont* CWunderlandTooltip::Font() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);
    }

TSize CWunderlandTooltip::PreferredSize() const
    {
    const CFont* font = Font();
    return TSize(font->TextWidthInPixels(*iText) + 2 * KHPad,
                 font->HeightInPixels() + font->DescentInPixels() + 2 * KVPad);
    }

void CWunderlandTooltip::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KBackColor());
    gc.DrawRect(rect);

    const CFont* font = Font();
    gc.UseFont(font);
    gc.SetBrushStyle(CGraphicsContext::ENullBrush);   // DrawText(box) must not clear
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KTextColor());
    const TInt baseline = (rect.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    gc.DrawText(*iText, rect, baseline, CGraphicsContext::ECenter);
    gc.DiscardFont();
    }
