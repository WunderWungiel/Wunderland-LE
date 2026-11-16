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
