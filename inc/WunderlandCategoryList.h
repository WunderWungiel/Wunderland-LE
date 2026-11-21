#ifndef __WUNDERLANDCATEGORYLIST_H__
#define __WUNDERLANDCATEGORYLIST_H__

#include <coecntrl.h>
#include "WunderlandPage.h"

class CFont;
class CDesC16ArrayFlat;

// Categories page in the Ovi Store style: a grey bold "Categories" heading,
// then one thick row per entry (content types for now). Rows are rounded
// boxes inset from the edges, left-aligned with the heading: dark grey bold
// name on the left, "»" on the right. The highlighted row turns dark grey
// like the focused tab. The whole page scrolls when it doesn't fit.
// Non-window-owning. Keys are fed by the parent (MoveHighlight); pointer
// events arrive through the parent. Reports EEventStateChanged to its
// observer when a tap highlights a row.
// A grey status line ("Loading...", "Loading failed...") can follow the
// rows; a tap on a retry status goes to the retry observer.
class CWunderlandCategoryList : public CCoeControl, public MWunderlandPage
    {
public:
    static CWunderlandCategoryList* NewL(const CCoeControl& aParent);
    ~CWunderlandCategoryList();

    void Reset();   // removes all rows, highlight and scroll (not the status)
    void AppendL(const TDesC& aName);

    // KNullDesC hides the line. aRetry: a tap on it calls the retry observer.
    void SetStatusL(const TDesC& aText, TBool aRetry = EFalse);
    TBool CanRetry() const;   // a retry status is shown
    void SetRetryObserver(MWunderlandRetryObserver* aObserver);   // not owned

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
    CWunderlandCategoryList();
    void ConstructL(const CCoeControl& aParent);

    const CFont* HeadingFont() const;
    const CFont* RowFont() const;
    // Content coordinates: y from the top of the unscrolled page
    TInt HeadingHeight() const;
    TInt RowHeight() const;
    TInt RowTop(TInt aIndex) const;
    TInt ContentHeight() const;
    TBool HasScrollBar() const;
    TInt ItemAt(const TPoint& aPos) const;   // -1 if none
    TBool IsStatusAt(const TPoint& aPos) const;
    void SetScroll(TInt aScrollY);            // clamps
    void EnsureVisible(TInt aIndex);

    void DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow) const;
    void DrawScrollBar(CWindowGc& aGc) const;

private:
    CDesC16ArrayFlat* iItems;   // owned
    HBufC* iStatus;             // owned; NULL: no status line
    TBool iRetry;
    MWunderlandRetryObserver* iRetryObserver;   // not owned
    TInt iHighlighted;
    TInt iScrollY;              // pixels scrolled from the top of the page
    // touch tracking
    TInt iDragStartY;
    TInt iDragStartScroll;
    TBool iDragged;
    };

#endif // __WUNDERLANDCATEGORYLIST_H__
