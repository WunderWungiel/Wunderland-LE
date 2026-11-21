#ifndef __WUNDERLANDPAGE_H__
#define __WUNDERLANDPAGE_H__

#include <e32def.h>

// A page under the tab bar whose rows take the d-pad highlight. Lets the
// container move the focus without knowing which page is shown.
class MWunderlandPage
    {
public:
    virtual TInt Count() const = 0;
    virtual TInt Highlighted() const = 0;           // -1 if no row is highlighted
    virtual void SetHighlighted(TInt aIndex) = 0;   // -1 removes it; scrolls the row into view
    // Moves the highlight by aDelta rows without wrapping. EFalse if it would
    // leave the list (the parent then moves the focus elsewhere).
    virtual TBool MoveHighlight(TInt aDelta) = 0;
    };

class CCoeControl;

// Told when the user taps a page's "loading failed" line to load it again.
class MWunderlandRetryObserver
    {
public:
    virtual void HandleRetryL(CCoeControl& aPage) = 0;
    };

#endif // __WUNDERLANDPAGE_H__
