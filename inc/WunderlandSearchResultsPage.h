#ifndef __WUNDERLANDSEARCHRESULTSPAGE_H__
#define __WUNDERLANDSEARCHRESULTSPAGE_H__

#include <coecntrl.h>
#include "WunderlandPage.h"
#include "WunderlandSearchBar.h"

class CWunderlandAppList;

// Full-screen search results, shown instead of the tab bar and its pages:
// the "Search results" header (like a content type page), the search bar
// holding the query (a new search can be started from here), then the
// results in the same list as the Home tab (a CWunderlandAppList; the owner
// fills it through List()).
// Rows for the d-pad: 0 is the search bar, 1.. are the results. Up from the
// first result goes to the bar.
// Non-window-owning. Searches go to its MWunderlandSearchObserver.
class CWunderlandSearchResultsPage : public CCoeControl, public MWunderlandPage,
                                     public MWunderlandSearchBarObserver,
                                     public MCoeControlObserver
    {
public:
    static CWunderlandSearchResultsPage* NewL(const CCoeControl& aParent,
                                              MWunderlandSearchObserver& aObserver);
    ~CWunderlandSearchResultsPage();

    CWunderlandAppList& List() const;
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

private: // from MCoeControlObserver
    void HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);   // a tapped result

private:
    CWunderlandSearchResultsPage(MWunderlandSearchObserver& aObserver);
    void ConstructL(const CCoeControl& aParent);

    TRect HeaderRect() const;

private:
    MWunderlandSearchObserver& iObserver;
    CWunderlandSearchBar* iBar;    // owned
    CWunderlandAppList* iList;     // owned
    };

#endif // __WUNDERLANDSEARCHRESULTSPAGE_H__
