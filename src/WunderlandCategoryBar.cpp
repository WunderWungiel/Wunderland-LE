#include <aknutils.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandCategoryBar.h"
#include "WunderlandDrawUtils.h"

using WunderlandDraw::FillGradient;

namespace
    {
    const TInt KHPad = 6;              // text inset from the left edge
    const TInt KVPad = 5;              // text padding above/below
    const TInt KMaxDrawnText = 128;    // longer paths are cut before clipping

    // Idle: grey gradient. Highlighted: blue, matching the selected tab.
    const TRgb KTopColor()          { return TRgb(0x96, 0x96, 0x96); }
    const TRgb KBottomColor()       { return TRgb(0x5E, 0x5E, 0x5E); }
    const TRgb KHighlightLine()     { return TRgb(0xBE, 0xBE, 0xBE); }   // top edge
    const TRgb KShadeLine()         { return TRgb(0x3A, 0x3A, 0x3A); }   // bottom edge
    const TRgb KFocusTopColor()     { return TRgb(0x3E, 0x8A, 0xD0); }
    const TRgb KFocusBottomColor()  { return TRgb(0x0A, 0x34, 0x62); }
    const TRgb KFocusBorderColor()  { return TRgb(0xC8, 0xE0, 0xF6); }
    const TRgb KTextColor()         { return KRgbWhite; }
    const TRgb KArrowShadowColor()  { return TRgb(0x2A, 0x2A, 0x2A); }
    }

CWunderlandCategoryBar* CWunderlandCategoryBar::NewL(const CCoeControl& aParent)
    {
    CWunderlandCategoryBar* self = new (ELeave) CWunderlandCategoryBar;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandCategoryBar::CWunderlandCategoryBar()
    {
    }

void CWunderlandCategoryBar::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    }

CWunderlandCategoryBar::~CWunderlandCategoryBar()
    {
    delete iText;
    }

void CWunderlandCategoryBar::SetPathL(const TDesC& aContentType, const TDesC& aCategory)
    {
    _LIT(KSeparator, " > ");
    HBufC* text = HBufC::NewL(aContentType.Length() + KSeparator().Length() + aCategory.Length());
    TPtr ptr(text->Des());
    ptr.Append(aContentType);
    ptr.Append(KSeparator);
    ptr.Append(aCategory);
    delete iText;
    iText = text;
    DrawDeferred();
    }

void CWunderlandCategoryBar::SetHighlighted(TBool aHighlighted)
    {
    if (!aHighlighted != !iHighlighted)
        {
        iHighlighted = aHighlighted;
        DrawDeferred();
        }
    }

TBool CWunderlandCategoryBar::IsHighlighted() const
    {
    return iHighlighted;
    }

const CFont* CWunderlandCategoryBar::Font() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);
    }

TInt CWunderlandCategoryBar::PreferredHeight() const
    {
    return Font()->HeightInPixels() + 2 * KVPad;
    }

void CWunderlandCategoryBar::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());

    if (iHighlighted)
        {
        FillGradient(gc, rect, KFocusTopColor(), KFocusBottomColor());
        gc.SetPenColor(KFocusBorderColor());
        gc.DrawRect(rect);   // brush is null after FillGradient: outline only
        }
    else
        {
        FillGradient(gc, rect, KTopColor(), KBottomColor());
        gc.SetPenColor(KHighlightLine());
        gc.DrawLine(rect.iTl, TPoint(rect.iBr.iX, rect.iTl.iY));
        gc.SetPenColor(KShadeLine());
        gc.DrawLine(TPoint(rect.iTl.iX, rect.iBr.iY - 1), TPoint(rect.iBr.iX, rect.iBr.iY - 1));
        }

    // Drop-down arrow on the right, in a square area as tall as the bar
    const TInt arrowArea = rect.Height();
    const TPoint c(rect.iBr.iX - arrowArea / 2, rect.Center().iY);
    const TInt h = arrowArea / 6;   // triangle height; width is four times that
    TPoint pts[3];
    pts[0] = TPoint(c.iX - 2 * h, c.iY - h);
    pts[1] = TPoint(c.iX + 2 * h, c.iY - h);
    pts[2] = TPoint(c.iX, c.iY + h);
    WunderlandDraw::FillTriangle(gc, pts, KTextColor(), KArrowShadowColor());

    if (!iText)
        {
        return;
        }
    const CFont* font = Font();
    const TRect textRect(rect.iTl.iX + KHPad, rect.iTl.iY, rect.iBr.iX - arrowArea, rect.iBr.iY);
    TBuf<KMaxDrawnText> text(iText->Left(KMaxDrawnText));
    AknTextUtils::ClipToFit(text, *font, textRect.Width());   // ends with "..." when too long
    gc.UseFont(font);
    gc.SetBrushStyle(CGraphicsContext::ENullBrush);
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KTextColor());
    const TInt baseline = (textRect.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    gc.DrawText(text, textRect, baseline, CGraphicsContext::ELeft);
    gc.DiscardFont();
    }
