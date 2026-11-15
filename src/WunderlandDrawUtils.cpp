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

void WunderlandDraw::FillTriangle(CWindowGc& aGc, const TPoint aPts[3], const TRgb& aColour, const TRgb& aShadow)
    {
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    if (aShadow != aColour)
        {
        TPoint shadow[3];
        for (TInt i = 0; i < 3; ++i)
            {
            shadow[i] = aPts[i] + TPoint(0, 1);
            }
        aGc.SetPenColor(aShadow);
        aGc.SetBrushColor(aShadow);
        aGc.DrawPolygon(shadow, 3);
        }
    aGc.SetPenColor(aColour);
    aGc.SetBrushColor(aColour);
    aGc.DrawPolygon(aPts, 3);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    }
