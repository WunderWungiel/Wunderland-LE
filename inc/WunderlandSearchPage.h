#ifndef __WUNDERLANDSEARCHPAGE_H__
#define __WUNDERLANDSEARCHPAGE_H__

#include <coecntrl.h>
#include <e32base.h>
#include "WunderlandPage.h"
#include "WunderlandSearchBar.h"

class CFont;
class CWindowGc;

// Search tab: the search bar (field + Go) on top; white below, with a hint
// text centred in that space. The bar is the page's only row: highlighting
// it focuses the field. Up from it goes to the tab bar; Left at the start of
// the field and Right on Go aren't used by the page, so they switch tabs.
// Reports EEventStateChanged to its observer when a tap focuses the bar, and
// searches to its MWunderlandSearchObserver.
// Non-window-owning: lives inside the parent control's window.
class CWunderlandSearchPage : public CCoeControl, public MWunderlandPage,
                              public MWunderlandSearchBarObserver
    {
public:
    static CWunderlandSearchPage* NewL(const CCoeControl& aParent, MWunderlandSearchObserver& aObserver);
    ~CWunderlandSearchPage();

    void SetQueryL(const TDesC& aQuery);

public: // from MWunderlandPage
    TInt Count() const;
    TInt Highlighted() const;
    void SetHighlighted(TInt aIndex);
    TBool MoveHighlight(TInt aDelta);

public: // from CCoeControl
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;

private: // from MWunderlandSearchBarObserver
    void HandleSearchBarTappedL(CWunderlandSearchBar& aBar);
    void HandleSearchRequestL(CWunderlandSearchBar& aBar);

private:
    CWunderlandSearchPage(MWunderlandSearchObserver& aObserver);
    void ConstructL(const CCoeControl& aParent);

    TRect BodyRect() const;   // below the bar
    const CFont* HintFont() const;
    TInt HintLineWidth() const;
    void WrapHintL();
    void DrawHint(CWindowGc& aGc) const;

private:
    MWunderlandSearchObserver& iObserver;
    CWunderlandSearchBar* iBar;         // owned
    HBufC* iHint;                       // owned; writable copy of the hint (see WrapHintL)
    CArrayFixFlat<TPtrC>* iHintLines;   // owned; the hint wrapped to the page width, into iHint
    };

#endif // __WUNDERLANDSEARCHPAGE_H__
