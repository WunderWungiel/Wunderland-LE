#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>
#include <badesca.h>
#include "WunderlandAppList.h"
#include "WunderlandSearchBar.h"

class CWunderlandTabBar;
class CWunderlandTooltip;
class CWunderlandCategoryList;
class CWunderlandSearchPage;
class CWunderlandContentTypePage;
class CWunderlandSearchResultsPage;
class MWunderlandPage;

// Main view: icon tab bar (Home / Categories / Search) on top, the page of
// the active tab below it: the list of entries (sample data for now) on Home,
// the content types on Categories, the search bar on Search. While the tab
// bar has the d-pad focus, a tooltip under the focused tab shows its name for
// a moment.
// Two pages can cover the whole view (no tab bar), with "Store menu" / "Back"
// softkeys: a content type selected on Categories, and the search results
// after a search. Back returns to the tab they were opened from.
// The Home list and the search results load page by page (KPageSize entries)
// as the user scrolls, one page request at a time; for now from the sample
// data, after a pretend network delay.
class CWunderlandContainer : public CCoeControl, public MCoeControlObserver,
                             public MWunderlandSearchObserver,
                             public MWunderlandAppListLoader
    {
public:
    static CWunderlandContainer* NewL(const TRect& aRect);
    ~CWunderlandContainer();

    void HandleBackL();   // closes the full-screen page, if any

public: // from CCoeControl
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void HandleResourceChange(TInt aType);
private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;
private: // from MCoeControlObserver
    void HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);
private: // from MWunderlandSearchObserver
    void HandleSearchL(const TDesC& aQuery);
private: // from MWunderlandAppListLoader
    void HandleLoadMoreL(CWunderlandAppList& aList);
private:
    CWunderlandContainer();
    void ConstructL(const TRect& aRect);
    void AddCategoriesL(const TText* const* aNames, TInt aCount);
    void AddSampleCategoriesL();
    void AddSampleAppsL();
    void AppendAppL(CWunderlandAppList& aList, TInt aIndex) const;   // index into iApps
    void AddContentTypesL();

    MWunderlandPage& Page() const;      // of the active tab
    CCoeControl* PageControl() const;   // the same object as a control
    void UpdatePage();                  // after the active tab changed
    void OpenContentTypeL(TInt aType);  // index into iContentTypes
    void CloseContentTypeL();           // back to the Categories tab
    void OpenSearchResultsL(const TDesC& aQuery);
    void CloseSearchResultsL();         // back to the Search tab
    static void SetSoftkeysL(TInt aResourceId);

    TRect ContentRect() const;    // below the tab bar
    void FocusTabBarL();          // takes the highlight off the list
    void FocusListL();            // first row; no-op on an empty list
    // Text, position, visibility. aRestart shows the tooltip again (focus
    // moved to a tab); otherwise one that already timed out stays hidden.
    void UpdateTooltipL(TBool aRestart = ETrue);
    static TInt HideTooltipCallBack(TAny* aSelf);
    void HideTooltip();

private:
    // One store entry (sample data until the store has an API)
    class TApp
        {
    public:
        TBuf<40> iName;
        TInt iType;          // index into iContentTypes
        TInt iCategory;      // index into iCategories[iType]
        TInt iRating;        // 1..5
        };

    // A list filled page by page: what has been loaded and what it wants.
    // With the store API, a page is GET /content?page=iNextPage+1&per_page=KPageSize
    // (plus search= for the results) and "more" is meta's HasNextPage().
    class TFeed
        {
    public:
        CWunderlandAppList* iList;   // not owned
        TInt iNextPage;              // 0-based
        TBool iWanted;               // the list asked for iNextPage; not started yet
        };

    TFeed* FeedFor(const CWunderlandAppList& aList);   // NULL if not ours
    void ResetFeed(TFeed& aFeed);     // empties the list; reloads from the first page
    void StartNextLoad();             // if idle and a list wants a page
    void StartLoadL(TFeed& aFeed);    // requests aFeed's iNextPage
    static TInt LoadDoneCallBack(TAny* aSelf);
    void DeliverPageL(TFeed& aFeed);
    void FocusFirstRow(TFeed& aFeed); // after a list's first page
    TInt SampleCount(const TFeed& aFeed) const;              // entries in the feed
    TInt SampleIndex(const TFeed& aFeed, TInt aPos) const;   // index into iApps

    CWunderlandTabBar* iTabBar;      // owned
    CWunderlandAppList* iAppList;    // owned
    CWunderlandCategoryList* iCategoryList;   // owned
    CWunderlandSearchPage* iSearchPage;       // owned
    CWunderlandContentTypePage* iContentTypePage;   // owned
    CWunderlandSearchResultsPage* iResultsPage;     // owned
    enum TOverlay
        {
        EOverlayNone,
        EOverlayContentType,
        EOverlaySearchResults
        };
    TOverlay iOverlay;               // full-screen page over the tabs, if any
    TInt iOpenType;                  // content type shown on iContentTypePage; -1: none
    CWunderlandTooltip* iTooltip;    // owned
    CPeriodic* iTooltipTimer;        // owned; hides the tooltip
    CPeriodic* iLoadTimer;           // owned; the pretend network delay of a page
    TFeed iHomeFeed;
    TFeed iResultsFeed;
    TFeed* iLoading;                 // the feed whose page is on the way; NULL: idle
    TBuf<KWunderlandMaxQuery> iSearchQuery;   // of iResultsFeed
    RArray<TApp> iApps;
    CDesCArrayFlat* iContentTypes;   // owned; Apps, Games, Themes
    RPointerArray<CDesCArrayFlat> iCategories;   // owned; one list per content type
    };

#endif // __WUNDERLANDCONTAINER_H__
