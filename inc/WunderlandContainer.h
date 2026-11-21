#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>
#include <badesca.h>
#include "WunderlandApiClient.h"
#include "WunderlandAppList.h"
#include "WunderlandPage.h"
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
// as the user scrolls, one page request at a time: the search results from
// the store API (GET /content?q=), Home for now from sample data after a
// pretend network delay.
// The content types (Categories tab) and each type's categories come from
// the store API: the types once at start-up, a type's categories when its
// page opens (kept for an hour, as the API allows). A failed load shows a
// retry line: Down or a tap retries.
class CWunderlandContainer : public CCoeControl, public MCoeControlObserver,
                             public MWunderlandSearchObserver,
                             public MWunderlandAppListLoader,
                             public MWunderlandApiObserver,
                             public MWunderlandRetryObserver
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
private: // from MWunderlandApiObserver
    void HandleApiResponseL(CWunderlandApiResponse& aResponse);
private: // from MWunderlandRetryObserver
    void HandleRetryL(CCoeControl& aPage);
private:
    CWunderlandContainer();
    void ConstructL(const TRect& aRect);
    void AddCategoriesL(const TText* const* aNames, TInt aCount);
    void AddSampleCategoriesL();
    void AddSampleAppsL();
    void AppendAppL(CWunderlandAppList& aList, TInt aIndex) const;   // index into iApps

    // Store API: content types and categories
    void LoadContentTypes();          // GET /content-types; failure shows the retry line
    void LoadContentTypesL();
    void ContentTypesLoadedL(const CWunderlandApiResponse& aResponse);
    void LoadCategories(TInt aType);  // GET /categories?type=; for the open page
    void LoadCategoriesL(TInt aType);
    void CategoriesLoadedL(const CWunderlandApiResponse& aResponse);
    void ShowCategoriesFailedL();     // on the open content type page
    void StartNextRequest();          // if the client is idle and a request waits
    void ApiResponseL(CWunderlandApiResponse& aResponse);
    TBool HasFreshCategories(TInt aType) const;
    TBool RetryOpenPageL();           // EFalse if the open page has nothing to retry

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
    // A results page is GET /content?q=&page=iNextPage+1&per_page=KPageSize;
    // "more" is meta's HasNextPage().
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
    void DeliverPageL(TFeed& aFeed);  // Home's sample entries
    void ContentPageLoadedL(TFeed& aFeed, const CWunderlandApiResponse& aResponse);
    void FocusFirstRow(TFeed& aFeed); // after a list's first page

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
    RPointerArray<CDesCArrayFlat> iSampleCategories;   // owned; category names of the sample entries

    // One content type from the API and its categories once loaded
    class CContentType : public CBase
        {
    public:
        static CContentType* NewLC(const TDesC& aId, const TDesC& aName, TBool aShowCategories);
        ~CContentType();
    private:
        void ConstructL(const TDesC& aId, const TDesC& aName);
    public:
        HBufC* iId;                    // owned; "apps"
        HBufC* iName;                  // owned; "Applications"
        TBool iShowCategories;         // EFalse: no content type page; browse the entries directly
        CDesCArrayFlat* iCategoryNames;   // owned; NULL until loaded
        RArray<TInt> iCategoryIds;     // parallel to iCategoryNames
        TTime iLoadedAt;               // universal time of the categories
        };

    CWunderlandApiClient* iApi;      // owned
    RPointerArray<CContentType> iContentTypes;   // owned; in the server's order
    TBool iContentTypesFailed;       // the Categories tab shows the retry line
    TBool iContentTypesWanted;       // to start once the client is idle
    TBool iContentTypesLoading;      // the running request
    TInt iCategoriesWanted;          // type whose categories wait for the client; -1: none
    TInt iCategoriesLoading;         // type whose categories are on the way; -1: none
    };

#endif // __WUNDERLANDCONTAINER_H__
