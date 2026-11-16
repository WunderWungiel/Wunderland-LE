#ifndef __WUNDERLANDSEARCHPAGE_H__
#define __WUNDERLANDSEARCHPAGE_H__

#include <coecntrl.h>
#include "WunderlandPage.h"

// Search page: blank white for now. Has no rows, so the d-pad focus never
// enters it (Up goes to the tab bar, Left/Right switch tabs).
// Non-window-owning: lives inside the parent control's window.
class CWunderlandSearchPage : public CCoeControl, public MWunderlandPage
    {
public:
    static CWunderlandSearchPage* NewL(const CCoeControl& aParent);
    ~CWunderlandSearchPage();

public: // from MWunderlandPage
    TInt Count() const;
    TInt Highlighted() const;
    void SetHighlighted(TInt aIndex);
    TBool MoveHighlight(TInt aDelta);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;

private:
    CWunderlandSearchPage();
    void ConstructL(const CCoeControl& aParent);
    };

#endif // __WUNDERLANDSEARCHPAGE_H__
