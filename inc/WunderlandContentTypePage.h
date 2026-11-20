#ifndef __WUNDERLANDCONTENTTYPEPAGE_H__
#define __WUNDERLANDCONTENTTYPEPAGE_H__

#include <coecntrl.h>
#include <bamdesca.h>
#include "WunderlandPage.h"

class CFont;
class CDesC16ArrayFlat;

// Page of one content type (Apps / Games / Themes), shown instead of the tab
// bar and its pages: a light grey header bar with the type's name, then a
// "See all <type>" row (a rounded box like the Categories rows), then the
// type's categories as one block of rows with no gap between them, each with
// "»" on the right. The header stays put; the rows scroll under it.
// Non-window-owning. Keys are fed by the parent (MoveHighlight); pointer
// events arrive through the parent. Reports EEventStateChanged to its
// observer when a tap highlights a row.
class CWunderlandContentTypePage : public CCoeControl, public MWunderlandPage
    {
public:
    static CWunderlandContentTypePage* NewL(const CCoeControl& aParent);
    ~CWunderlandContentTypePage();

    // Row 0 is "See all <aTitle in lower case>", rows 1.. are aCategories.
    // Removes the highlight and scrolls to the top.
    void SetContentL(const TDesC& aTitle, const MDesCArray& aCategories);

public: // from MWunderlandPage
    TInt Count() const;
    TInt Highlighted() const;
    void SetHighlighted(TInt aIndex);
    TBool MoveHighlight(TInt aDelta);

public: // from CCoeControl
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();

private:
    CWunderlandContentTypePage();
    void ConstructL(const CCoeControl& aParent);

    const CFont* HeaderFont() const;
    const CFont* RowFont() const;
    TRect HeaderRect() const;
    TRect ListRect() const;          // below the header
    // List coordinates: y from the top of the unscrolled list
    TInt RowHeight() const;
    TInt RowTop(TInt aIndex) const;
    TInt ContentHeight() const;
    TBool HasScrollBar() const;
    TInt ItemAt(const TPoint& aPos) const;   // -1 if none
    void SetScroll(TInt aScrollY);            // clamps
    void EnsureVisible(TInt aIndex);

    void DrawHeader(CWindowGc& aGc) const;
    void DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow, TInt aRadius) const;
    void DrawScrollBar(CWindowGc& aGc) const;

private:
    TBuf<32> iTitle;
    CDesC16ArrayFlat* iItems;   // owned
    TInt iHighlighted;
    TInt iScrollY;              // pixels scrolled from the top of the list
    // touch tracking
    TInt iDragStartY;
    TInt iDragStartScroll;
    TBool iDragged;
    };

#endif // __WUNDERLANDCONTENTTYPEPAGE_H__
