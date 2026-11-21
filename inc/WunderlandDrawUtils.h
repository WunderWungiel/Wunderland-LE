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

    // Like FillGradient, with corners rounded to aRadius pixels (no anti-aliasing).
    void FillRoundGradient(CWindowGc& aGc, const TRect& aRect, const TRgb& aTop, const TRgb& aBottom,
                           TInt aRadius);

    // Title bar of a full-screen page (content type, search results): light
    // grey gradient, a darker bottom line, the title left-aligned in the
    // primary font. Height follows the font, so it changes with the layout.
    TInt PageHeaderHeight();
    void DrawPageHeader(CWindowGc& aGc, const TRect& aRect, const TDesC& aTitle);

    // Grey status text ("Loading...", "Loading failed...") centred in aRect
    // in the secondary font, clipped with "..." when it doesn't fit.
    void DrawStatusText(CWindowGc& aGc, const TRect& aRect, const TDesC& aText);
    }

#endif // __WUNDERLANDDRAWUTILS_H__
