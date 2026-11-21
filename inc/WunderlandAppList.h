#ifndef __WUNDERLANDAPPLIST_H__
#define __WUNDERLANDAPPLIST_H__

#include <coecntrl.h>
#include "WunderlandPage.h"

class CFont;
class CFbsBitmap;
class CAsyncCallBack;
class CWunderlandAppList;

// Supplies a list's rows page by page. The list asks for the next page when
// the user scrolls near its end (see CWunderlandAppList::SetMoreAvailable).
class MWunderlandAppListLoader
    {
public:
    // Start loading the next page of aList: append its rows with AppendL(),
    // then call SetMoreAvailable() (or SetLoadFailed()). Called once per
    // page, asynchronously, never from inside a list method. A leave shows
    // the "loading failed" row.
    virtual void HandleLoadMoreL(CWunderlandAppList& aList) = 0;
    };

// Scrollable list of store entries in the Ovi Store style. Every row is as
// tall as its square (placeholder) icon: icon on the left, bold app name
// with an optional grey subtitle (the category) next to it, a 1-5 star
// rating stuck to the right. Rows alternate #E8E8E8 / white; the highlighted
// row turns dark grey.
// Non-window-owning. Keys are fed by the parent (MoveHighlight); pointer
// events arrive through the parent. Reports EEventStateChanged to its
// observer when a tap highlights a row.
// Paged loading: while more rows are available, a "Loading..." row follows
// the last one and the loader is asked for the next page as soon as that row
// comes within a few rows of the screen. If a page fails, the row offers a
// retry (Down on the last row, or a tap on it).
class CWunderlandAppList : public CCoeControl, public MWunderlandPage
    {
public:
    static CWunderlandAppList* NewL(const CCoeControl& aParent);
    ~CWunderlandAppList();

    // Removes all rows, highlight and scroll; no more rows are expected
    // until SetMoreAvailable(ETrue).
    void Reset();
    // aSubtitle may be empty: the name is then centred on its own.
    // aRating is clamped to 0..5.
    void AppendL(const TDesC& aName, const TDesC& aSubtitle, TInt aRating);

    void SetLoader(MWunderlandAppListLoader* aLoader);   // not owned; NULL: none
    // After each page (and once to start an empty list): whether the loader
    // has another page. ETrue shows the "Loading..." row.
    void SetMoreAvailable(TBool aMore);
    void SetLoadFailed();          // the requested page didn't arrive
    TBool IsLoading() const;       // a page was asked for and hasn't arrived
    // Asks for the failed page again. EFalse if no page had failed.
    TBool RetryLoad();

public: // from MWunderlandPage
    TInt Count() const;
    TInt Highlighted() const;
    void SetHighlighted(TInt aIndex);
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
    TInt RowCount() const;        // Count() plus the loading row, if shown
    TInt ContentHeight() const;
    TBool HasScrollBar() const;
    TInt ItemAt(const TPoint& aPos) const;   // -1 if none
    void SetScroll(TInt aScrollY);            // clamps
    void EnsureVisible(TInt aIndex);
    TBool IsLoadingRowAt(const TPoint& aPos) const;
    // Asks the loader for the next page (asynchronously) if one is
    // expected, none was asked for yet and its row is close to the screen.
    void CheckLoadMore();
    TBool NeedsMore() const;
    static TInt LoadMoreCallBack(TAny* aSelf);
    void DoLoadMore();

    void DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow) const;
    void DrawIcon(CWindowGc& aGc, const TItem& aItem, const TRect& aRect) const;
    TInt StarsWidth() const;
    void DrawStars(CWindowGc& aGc, TInt aRating, const TRect& aRow, TInt aRight) const;
    void DrawLoadingRow(CWindowGc& aGc, const TRect& aRow) const;
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
    // paged loading
    enum TLoadState
        {
        ELoadDone,        // no more pages: no loading row
        ELoadPending,     // another page exists; not asked for yet
        ELoadRequested,   // the loader is fetching it
        ELoadFailed       // the last request failed; waits for a retry
        };
    TLoadState iLoadState;
    MWunderlandAppListLoader* iLoader;   // not owned
    CAsyncCallBack* iLoadCallBack;       // owned; calls the loader outside list methods
    };

#endif // __WUNDERLANDAPPLIST_H__
