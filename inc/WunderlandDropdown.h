#ifndef __WUNDERLANDDROPDOWN_H__
#define __WUNDERLANDDROPDOWN_H__

#include <coecntrl.h>
#include <badesca.h>

class CFont;

// Drop-down list that floats over the content below the category bar.
// Window-owning child of the container so it stays on top of whatever the
// content area becomes. Hidden until Open(). Keys are fed by the parent;
// pointer events (touch) arrive directly.
// Reports EEventStateChanged to its observer when the user picks an item
// (Highlighted() is then the chosen index). The observer closes it.
class CWunderlandDropdown : public CCoeControl
    {
public:
    static CWunderlandDropdown* NewL(const CCoeControl& aParent);
    ~CWunderlandDropdown();

    void SetItemsL(const MDesCArray& aItems);
    // Shows the list at aMaxRect's top-left, as wide as aMaxRect and only as
    // tall as needed (scrolls when the items don't fit). No-op if empty.
    void Open(const TRect& aMaxRect, TInt aHighlighted);
    void Close();
    TBool IsOpen() const;
    TInt Highlighted() const;
    void ChooseL();   // picks the highlighted item: notifies the observer

public: // from CCoeControl
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;

private:
    CWunderlandDropdown();
    void ConstructL(const CCoeControl& aParent);

    const CFont* Font() const;
    TInt Count() const;
    TInt RowHeight() const;
    TInt VisibleRows() const;
    TBool HasScrollBar() const;
    TRect InnerRect() const;                 // own window coordinates, inside the border
    TRect RowRect(TInt aRow) const;          // aRow = visible row (0 = top)
    TInt ItemAt(const TPoint& aPos) const;   // -1 if none
    void SetTop(TInt aTop);                  // clamps
    void EnsureHighlightVisible();
    void MoveHighlight(TInt aDelta);         // wraps around

private:
    CDesCArrayFlat* iItems;   // owned
    TInt iHighlighted;
    TInt iTop;                // first visible item
    // touch tracking
    TInt iDownItem;
    TInt iDragStartY;
    TInt iDragStartTop;
    TBool iDragged;
    };

#endif // __WUNDERLANDDROPDOWN_H__
