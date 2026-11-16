#ifndef __WUNDERLANDAPPLIST_H__
#define __WUNDERLANDAPPLIST_H__

#include <coecntrl.h>

class CFont;
class CFbsBitmap;

// Scrollable list of store entries in the Ovi Store style. Every row is as
// tall as its square (placeholder) icon: icon on the left, bold app name
// with an optional grey subtitle (the category) next to it, a 1-5 star
// rating stuck to the right. Rows alternate #E8E8E8 / white; the highlighted
// row turns dark grey.
// Non-window-owning. Keys are fed by the parent (MoveHighlight); pointer
// events arrive through the parent. Reports EEventStateChanged to its
// observer when a tap highlights a row.
class CWunderlandAppList : public CCoeControl
    {
public:
    static CWunderlandAppList* NewL(const CCoeControl& aParent);
    ~CWunderlandAppList();

    void Reset();   // removes all rows, highlight and scroll
    // aSubtitle may be empty: the name is then centred on its own.
    // aRating is clamped to 0..5.
    void AppendL(const TDesC& aName, const TDesC& aSubtitle, TInt aRating);
    TInt Count() const;

    TInt Highlighted() const;           // -1 if no row is highlighted
    void SetHighlighted(TInt aIndex);   // -1 removes it; scrolls the row into view
    // Moves the highlight by aDelta rows without wrapping. EFalse if it would
    // leave the list (the parent then moves the focus elsewhere).
    TBool MoveHighlight(TInt aDelta);

public: // from CCoeControl
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();

private:
    class TItem
        {
    public:
        TBuf<40> iName;
        TBuf<40> iSubtitle;
        TInt iRating;
        };

    CWunderlandAppList();
    void ConstructL(const CCoeControl& aParent);

    const CFont* NameFont() const;
    const CFont* SubtitleFont() const;
    TInt RowHeight() const;
    TInt ContentHeight() const;
    TBool HasScrollBar() const;
    TInt ItemAt(const TPoint& aPos) const;   // -1 if none
    void SetScroll(TInt aScrollY);            // clamps
    void EnsureVisible(TInt aIndex);

    void DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow) const;
    void DrawIcon(CWindowGc& aGc, const TItem& aItem, const TRect& aRect) const;
    TInt StarsWidth() const;
    void DrawStars(CWindowGc& aGc, TInt aRating, const TRect& aRow, TInt aRight) const;
    void DrawScrollBar(CWindowGc& aGc) const;

private:
    // Star bitmaps from the web store (star_p / star_g), 8-bit alpha masks
    CFbsBitmap* iStarOn;        // owned
    CFbsBitmap* iStarOnMask;    // owned
    CFbsBitmap* iStarOff;       // owned
    CFbsBitmap* iStarOffMask;   // owned
    RArray<TItem> iItems;
    TInt iHighlighted;
    TInt iScrollY;          // pixels scrolled from the top of the first row
    // touch tracking
    TInt iDragStartY;
    TInt iDragStartScroll;
    TBool iDragged;
    };

#endif // __WUNDERLANDAPPLIST_H__
