#ifndef __WUNDERLANDDRAWUTILS_H__
#define __WUNDERLANDDRAWUTILS_H__

#include <gdi.h>

class CWindowGc;

// Small drawing helpers shared by the custom controls.
// TRgb(TUint32) is 0x00BBGGRR, so always build colours with the (r, g, b) form.
namespace WunderlandDraw
    {
    // Linear blend: aNum/aDen of the way from aFrom to aTo.
    TRgb Blend(const TRgb& aFrom, const TRgb& aTo, TInt aNum, TInt aDen);

    // Fills aRect row by row with a vertical gradient. Leaves the pen solid, brush null.
    void FillGradient(CWindowGc& aGc, const TRect& aRect, const TRgb& aTop, const TRgb& aBottom);

    // Filled triangle with an optional 1 px drop shadow (aShadow == aColour disables it).
    void FillTriangle(CWindowGc& aGc, const TPoint aPts[3], const TRgb& aColour, const TRgb& aShadow);
    }

#endif // __WUNDERLANDDRAWUTILS_H__
