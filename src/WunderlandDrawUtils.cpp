#include <w32std.h>
#include "WunderlandDrawUtils.h"

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
