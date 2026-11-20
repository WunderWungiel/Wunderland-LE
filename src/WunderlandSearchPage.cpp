#include <aknutils.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"
#include "WunderlandSearchPage.h"

namespace
    {
    const TInt KHeaderMargin = 9;   // around the field and button, inside the header
    const TInt KFieldGap = 8;       // between the field and the button
    const TInt KFieldVPad = 5;      // text to the field's top/bottom edge
    const TInt KFieldHPad = 7;      // text to the field's left edge
    const TInt KButtonHPad = 16;    // "Go" to the button's left/right edge
    const TInt KHintLineGap = 2;    // extra space between the hint's lines

    _LIT(KHint, "Search for apps, games, themes and more in Wunderland Store!");

    const TRgb KHeaderColor()       { return TRgb(0x5E, 0x62, 0x68); }
    const TRgb KHeaderShadeColor()  { return TRgb(0x45, 0x48, 0x4D); }   // bottom edge
    const TRgb KFieldColor()        { return KRgbWhite; }
    const TRgb KFieldShadeColor()   { return TRgb(0xB4, 0xB8, 0xBE); }   // inner top edge
    const TRgb KPlaceholderColor()  { return TRgb(0x9A, 0x9A, 0x9A); }
    // Store accent: the same blue as the active tab icon
    const TRgb KButtonTopColor()    { return TRgb(0x4A, 0x97, 0xF0); }
    const TRgb KButtonBottomColor() { return TRgb(0x10, 0x56, 0xB4); }
    const TRgb KButtonTextColor()   { return KRgbWhite; }
    const TRgb KPageColor()         { return KRgbWhite; }
    const TRgb KHintColor()         { return TRgb(0x5A, 0x5A, 0x5A); }
    }

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
    iHintLines = new (ELeave) CArrayFixFlat<TPtrC>(4);
    }

CWunderlandSearchPage::~CWunderlandSearchPage()
    {
    delete iHintLines;
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

const CFont* CWunderlandSearchPage::FieldFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);
    }

const CFont* CWunderlandSearchPage::ButtonFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);   // small bold
    }

// Tall enough for either font; both depend on the layout (portrait,
// landscape, touch), so the header grows with them.
TInt CWunderlandSearchPage::FieldHeight() const
    {
    TInt textH = FieldFont()->HeightInPixels();
    const TInt buttonH = ButtonFont()->HeightInPixels();
    if (buttonH > textH)
        {
        textH = buttonH;
        }
    return (textH + 2 * KFieldVPad) * 13 / 10;   // roomier than a plain text line
    }

TRect CWunderlandSearchPage::HeaderRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl, TSize(rect.Width(), FieldHeight() + 2 * KHeaderMargin));
    }

TRect CWunderlandSearchPage::GoRect() const
    {
    _LIT(KGo, "Go");
    const TRect header(HeaderRect());
    const TInt h = FieldHeight();
    TInt w = ButtonFont()->TextWidthInPixels(KGo) + 2 * KButtonHPad;
    if (w < h)
        {
        w = h;
        }
    w = w * 9 / 10;   // a bit narrower than that
    const TInt right = header.iBr.iX - KHeaderMargin;
    const TInt top = header.iTl.iY + KHeaderMargin;
    return TRect(right - w, top, right, top + h);
    }

TRect CWunderlandSearchPage::FieldRect() const
    {
    const TRect header(HeaderRect());
    const TRect go(GoRect());
    return TRect(header.iTl.iX + KHeaderMargin, go.iTl.iY, go.iTl.iX - KFieldGap, go.iBr.iY);
    }

void CWunderlandSearchPage::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    gc.SetClippingRect(rect);

    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KPageColor());
    gc.DrawRect(rect);

    const TRect header(HeaderRect());
    gc.SetBrushColor(KHeaderColor());
    gc.DrawRect(header);
    gc.SetBrushColor(KHeaderShadeColor());
    gc.DrawRect(TRect(header.iTl.iX, header.iBr.iY - 1, header.iBr.iX, header.iBr.iY));

    DrawField(gc);
    DrawGoButton(gc);
    DrawHint(gc);
    gc.CancelClippingRect();
    }

// The fonts follow the layout, which always comes with a SetRect(), so the
// hint is wrapped again here. On failure it's simply not shown.
void CWunderlandSearchPage::SizeChanged()
    {
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
    // The lines point into KHint, a literal, so they stay valid.
    AknTextUtils::WrapToArrayL(KHint, width, *HintFont(), *iHintLines);
    }

// Centred horizontally on the page and vertically in the space below the header.
void CWunderlandSearchPage::DrawHint(CWindowGc& aGc) const
    {
    const TInt count = iHintLines->Count();
    if (count == 0)
        {
        return;
        }
    const CFont* font = HintFont();
    const TInt lineH = font->HeightInPixels() + KHintLineGap;
    const TRect rect(Rect());
    const TRect body(rect.iTl.iX, HeaderRect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    const TInt blockH = count * lineH - KHintLineGap;
    TInt top = (body.Height() - blockH) / 2;
    if (top < 0)
        {
        top = 0;   // too short: keep the first lines under the header
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

void CWunderlandSearchPage::DrawField(CWindowGc& aGc) const
    {
    const TRect field(FieldRect());
    if (field.Width() <= 2 * KFieldHPad)
        {
        return;
        }
    aGc.SetPenStyle(CGraphicsContext::ENullPen);
    aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    aGc.SetBrushColor(KFieldColor());
    aGc.DrawRect(field);
    aGc.SetBrushColor(KFieldShadeColor());   // a little depth: the field sits in the header
    aGc.DrawRect(TRect(field.iTl, TSize(field.Width(), 1)));

    // TODO: an editor on top of the field once search is wired up
    _LIT(KPlaceholder, "Type here...");
    const CFont* font = FieldFont();
    TBuf<32> text(KPlaceholder);
    AknTextUtils::ClipToFit(text, *font, field.Width() - 2 * KFieldHPad);
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KPlaceholderColor());
    const TInt baseline = (field.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    aGc.DrawText(text, field, baseline, CGraphicsContext::ELeft, KFieldHPad);
    aGc.DiscardFont();
    }

void CWunderlandSearchPage::DrawGoButton(CWindowGc& aGc) const
    {
    _LIT(KGo, "Go");
    const TRect go(GoRect());
    WunderlandDraw::FillGradient(aGc, go, KButtonTopColor(), KButtonBottomColor());

    const CFont* font = ButtonFont();
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KButtonTextColor());
    const TInt baseline = (go.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    aGc.DrawText(KGo, go, baseline, CGraphicsContext::ECenter);
    aGc.DiscardFont();
    }
