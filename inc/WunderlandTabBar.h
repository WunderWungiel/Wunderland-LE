#ifndef __WUNDERLANDTABBAR_H__
#define __WUNDERLANDTABBAR_H__

#include <coecntrl.h>
#include <badesca.h>

class CFont;

// Horizontal, scrollable category bar ("Home", "Apps", ...) drawn in the
// Ovi Store style: arrows on both ends, the selected tab kept centred.
// Non-window-owning: lives inside the parent control's window.
// Reports EEventStateChanged to its observer when the selection changes.
class CWunderlandTabBar : public CCoeControl
    {
public:
    static CWunderlandTabBar* NewL(const CCoeControl& aParent);
    ~CWunderlandTabBar();

    void AppendTabL(const TDesC& aLabel);
    TInt Count() const;
    TInt Selected() const;
    TPtrC SelectedLabel() const;
    void SetSelected(TInt aIndex);   // clamps; redraws; does not notify the observer

    TInt PreferredHeight() const;    // depends on the current layout font

public: // from CCoeControl
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();

private:
    CWunderlandTabBar();
    void ConstructL(const CCoeControl& aParent);

    const CFont* Font() const;
    TInt ArrowWidth() const;
    TRect LeftArrowRect() const;
    TRect RightArrowRect() const;
    TRect Viewport() const;                      // area between the arrows
    TInt TabWidth(TInt aIndex) const;
    TRect TabRect(TInt aIndex) const;            // in control coordinates, scrolled
    TInt TotalTabsWidth() const;
    void UpdateScroll();                         // centre the selected tab
    void MoveSelectionL(TInt aDelta);            // select + notify observer
    void DrawArrow(CWindowGc& aGc, const TRect& aRect, TBool aLeft, TBool aEnabled) const;

private:
    CDesCArrayFlat* iLabels;   // owned
    TInt iSelected;
    TInt iScrollX;             // pixels the tab strip is shifted left
    };

#endif // __WUNDERLANDTABBAR_H__
