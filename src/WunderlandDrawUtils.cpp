#include <aknutils.h>
#include <avkon.hrh>
#include <coemain.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"

RWunderlandFont::RWunderlandFont()
    : iFont(NULL), iBaseFontId(EAknLogicalFontPrimaryFont), iPercent(100), iBold(EFalse)
    {
    }

TInt RWunderlandFont::Update(TInt aBaseFontId, TInt aPercent, TBool aBold)
    {
    const TFontSpec base(AknLayoutUtils::FontFromId(aBaseFontId)->FontSpecInTwips());
    if (iFont && aBaseFontId == iBaseFontId && aPercent == iPercent && aBold == iBold
        && base == iBaseSpec)
        {
        return KErrNone;   // nothing changed
        }
    TFontSpec spec(base);
    spec.iHeight = base.iHeight * aPercent / 100;
    if (aBold)
        {
        spec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
        }
    CWsScreenDevice* device = CCoeEnv::Static()->ScreenDevice();
    CFont* font = NULL;
    const TInt err = device->GetNearestFontInTwips(font, spec);
    if (err != KErrNone)
        {
        return err;
        }
    Close();
    iFont = font;
    iBaseSpec = base;
    iBaseFontId = aBaseFontId;
    iPercent = aPercent;
    iBold = aBold;
    return KErrNone;
    }

void RWunderlandFont::Close()
    {
    if (iFont)
        {
        CCoeEnv::Static()->ScreenDevice()->ReleaseFont(iFont);
        iFont = NULL;
        }
    }

const CFont* RWunderlandFont::Font() const
    {
    return iFont ? iFont : AknLayoutUtils::FontFromId(iBaseFontId);
    }

namespace
    {
    const TInt KRefShortSide = 360;   // the screen the pixel sizes are designed for
    TInt gShortSide = 0;              // cached by UpdateScale(); 0 = not yet
    }

void WunderlandDraw::UpdateScale()
    {
    TRect screen;
    if (!AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EScreen, screen))
        {
        screen = TRect(CCoeEnv::Static()->ScreenDevice()->SizeInPixels());
        }
    const TSize size(screen.Size());
    gShortSide = size.iWidth < size.iHeight ? size.iWidth : size.iHeight;
    if (gShortSide <= 0)
        {
        gShortSide = KRefShortSide;
        }
    }

TInt WunderlandDraw::Px(TInt aRefPixels)
    {
    if (gShortSide == 0)
        {
        UpdateScale();
        }
    if (aRefPixels == 0)
        {
        return 0;
        }
    const TInt px = (aRefPixels * gShortSide + KRefShortSide / 2) / KRefShortSide;
    return px != 0 ? px : (aRefPixels > 0 ? 1 : -1);
    }

namespace
    {
    // Below nHD some elements keep their original (pre-nHD) look.
    TBool IsSmallScreen()
        {
        if (gShortSide == 0)
            {
            WunderlandDraw::UpdateScale();
            }
        return gShortSide < KRefShortSide;
        }
    }

const CFont* WunderlandDraw::HeadingFont()
    {
    return AknLayoutUtils::FontFromId(!IsSmallScreen()
                                      ? EAknLogicalFontTitleFont          // large bold
                                      : EAknLogicalFontPrimarySmallFont); // small bold
    }

TRgb WunderlandDraw::Blend(const TRgb& aFrom, const TRgb& aTo, TInt aNum, TInt aDen)
    {
    if (aDen <= 0)
        {
        return aFrom;
        }
    return TRgb(aFrom.Red()   + (aTo.Red()   - aFrom.Red())   * aNum / aDen,
                aFrom.Green() + (aTo.Green() - aFrom.Green()) * aNum / aDen,
                aFrom.Blue()  + (aTo.Blue()  - aFrom.Blue())  * aNum / aDen);
    }

void WunderlandDraw::FillGradient(CWindowGc& aGc, const TRect& aRect, const TRgb& aTop, const TRgb& aBottom)
    {
    const TInt h = aRect.Height();
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenSize(TSize(1, 1));
    for (TInt row = 0; row < h; ++row)
        {
        const TInt y = aRect.iTl.iY + row;
        aGc.SetPenColor(Blend(aTop, aBottom, row, h > 1 ? h - 1 : 1));
        aGc.DrawLine(TPoint(aRect.iTl.iX, y), TPoint(aRect.iBr.iX, y));   // end point excluded
        }
    }

namespace
    {
    // How far the row aRow (0 = outermost) of a corner of radius aRadius is
    // cut in: aRadius minus the circle's half-width at the row's centre.
    TInt CornerInset(TInt aRadius, TInt aRow)
        {
        if (aRow >= aRadius)
            {
            return 0;
            }
        // Doubled units keep the half-pixel row centre an integer
        const TInt dy = 2 * (aRadius - aRow) - 1;
        const TInt sq = 4 * aRadius * aRadius - dy * dy;
        TInt dx = 0;                      // rounded square root of sq
        while ((dx + 1) * (dx + 1) <= sq)
            {
            ++dx;
            }
        if (sq - dx * dx > dx)
            {
            ++dx;
            }
        return (2 * aRadius - dx + 1) / 2;
        }
    }

void WunderlandDraw::FillRoundGradient(CWindowGc& aGc, const TRect& aRect, const TRgb& aTop,
                                       const TRgb& aBottom, TInt aRadius)
    {
    const TInt h = aRect.Height();
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenSize(TSize(1, 1));
    for (TInt row = 0; row < h; ++row)
        {
        const TInt fromEdge = row < h - 1 - row ? row : h - 1 - row;
        const TInt inset = CornerInset(aRadius, fromEdge);
        const TInt y = aRect.iTl.iY + row;
        aGc.SetPenColor(Blend(aTop, aBottom, row, h > 1 ? h - 1 : 1));
        aGc.DrawLine(TPoint(aRect.iTl.iX + inset, y), TPoint(aRect.iBr.iX - inset, y));
        }
    }

namespace
    {
    // Small screens keep the original bar: fixed padding, smaller inset.
    const TInt KHeaderTextInset = 24;       // lines up with the row text of the pages below
    const TInt KSmallHeaderTextInset = 16;
    const TInt KSmallHeaderVPad = 8;

    TInt HeaderTextInset()
        {
        return IsSmallScreen() ? KSmallHeaderTextInset : KHeaderTextInset;
        }
    const TInt KMaxTitle = 64;

    const TRgb KHeaderTopColor()     { return TRgb(0xF2, 0xF3, 0xF4); }
    const TRgb KHeaderBottomColor()  { return TRgb(0xD6, 0xD9, 0xDC); }
    const TRgb KHeaderShadeColor()   { return TRgb(0xB4, 0xB8, 0xBE); }   // bottom edge
    const TRgb KHeaderTextColor()    { return TRgb(0x3A, 0x3D, 0x40); }

    const CFont* HeaderFont()
        {
        return AknLayoutUtils::FontFromId(EAknLogicalFontPrimaryFont);
        }

    // Above/below the title. On nHD and bigger it grows with the font
    // (the bar is about 2.5x the text height).
    TInt HeaderVPad(const CFont& aFont)
        {
        return IsSmallScreen() ? KSmallHeaderVPad : 10 + aFont.HeightInPixels() / 3;
        }
    }

TInt WunderlandDraw::PageHeaderHeight()
    {
    const CFont* font = HeaderFont();
    return 2 * HeaderVPad(*font) + font->HeightInPixels();
    }

void WunderlandDraw::DrawPageHeader(CWindowGc& aGc, const TRect& aRect, const TDesC& aTitle)
    {
    FillGradient(aGc, TRect(aRect.iTl, TPoint(aRect.iBr.iX, aRect.iBr.iY - 1)),
                 KHeaderTopColor(), KHeaderBottomColor());
    aGc.SetPenColor(KHeaderShadeColor());
    aGc.DrawLine(TPoint(aRect.iTl.iX, aRect.iBr.iY - 1), TPoint(aRect.iBr.iX, aRect.iBr.iY - 1));

    const TInt width = aRect.Width() - 2 * HeaderTextInset();
    if (width <= 0)
        {
        return;
        }
    const CFont* font = HeaderFont();
    TBuf<KMaxTitle> text(aTitle.Left(KMaxTitle));
    AknTextUtils::ClipToFit(text, *font, width);
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KHeaderTextColor());
    aGc.DrawText(text, TPoint(aRect.iTl.iX + HeaderTextInset(),
                              aRect.iTl.iY + HeaderVPad(*font) + font->AscentInPixels()));
    aGc.DiscardFont();
    }

void WunderlandDraw::DrawStatusText(CWindowGc& aGc, const TRect& aRect, const TDesC& aText)
    {
    const TInt KMaxStatus = 64;
    const TInt width = aRect.Width();
    if (width <= 0)
        {
        return;
        }
    const CFont* font = AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);
    TBuf<KMaxStatus> text(aText.Left(KMaxStatus));
    AknTextUtils::ClipToFit(text, *font, width);
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(TRgb(0x80, 0x80, 0x80));   // the lists' "No items" grey
    const TInt baseline = (aRect.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    aGc.DrawText(text, aRect, baseline, CGraphicsContext::ECenter);
    aGc.DiscardFont();
    }
