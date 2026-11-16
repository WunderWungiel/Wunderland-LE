#ifndef __WUNDERLANDTABBAR_H__
#define __WUNDERLANDTABBAR_H__

#include <coecntrl.h>

class CFbsBitmap;

// Ovi Store style top bar: three equal-width icon tabs (Home, Categories,
// Search) on a white-to-light-grey gradient. The active tab looks pressed in:
// lighter background, blue engraved icon. Icons are SVGs from
// WunderlandLE_0xE4F76EAD_tabs.mif, one per tab and state.
// While the bar has the d-pad focus, the focused tab is drawn dark grey
// (left/right move it, Select activates it); its name is shown by the
// parent in a tooltip under the tab (see FocusedLabel / TabRect).
// Non-window-owning: lives inside the parent control's window.
// Reports EEventStateChanged to its observer when the focus or active tab changes.
class CWunderlandTabBar : public CCoeControl
    {
public:
    enum TTab
        {
        ETabHome,
        ETabCategories,
        ETabSearch,
        ETabCount
        };

    static CWunderlandTabBar* NewL(const CCoeControl& aParent);
    ~CWunderlandTabBar();

    TInt Active() const;
    TInt Focused() const;               // tab under the d-pad focus
    TBool IsBarFocused() const;
    void SetBarFocused(TBool aFocused); // redraws if changed; does not notify the observer
    TPtrC FocusedLabel() const;         // "Home", "Categories", "Search"
    TRect TabRect(TInt aIndex) const;   // in parent (window) coordinates

    // Height for a bar in a client area of aArea: tall, like Ovi Store.
    TInt PreferredHeight(const TSize& aArea) const;

public: // from CCoeControl
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();

private:
    CWunderlandTabBar();
    void ConstructL(const CCoeControl& aParent);

    void MoveFocusL(TInt aDelta);   // no wrapping; notifies the observer
    void ActivateFocusedL();        // notifies the observer
    TSize IconSize() const;

private:
    enum TIconState
        {
        EIconNormal,
        EIconActive,
        EIconFocused,
        EIconFocusedActive,
        EIconStateCount
        };

    CFbsBitmap* iIcons[ETabCount][EIconStateCount];       // owned
    CFbsBitmap* iIconMasks[ETabCount][EIconStateCount];   // owned
    TInt iActive;
    TInt iFocused;
    TBool iBarFocused;
    };

#endif // __WUNDERLANDTABBAR_H__
