#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>
#include <badesca.h>

class CWunderlandTabBar;
class CWunderlandTooltip;
class CWunderlandAppList;
class CWunderlandCategoryList;
class CWunderlandSearchPage;
class MWunderlandPage;

// Main view: icon tab bar (Home / Categories / Search) on top, the page of
// the active tab below it: the list of entries (sample data for now) on Home,
// the content types on Categories, a blank page on Search (for now). While the tab bar has the d-pad
// focus, a tooltip under the focused tab shows its name for a moment.
class CWunderlandContainer : public CCoeControl, public MCoeControlObserver
    {
public:
    static CWunderlandContainer* NewL(const TRect& aRect);
    ~CWunderlandContainer();

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
private:
    CWunderlandContainer();
    void ConstructL(const TRect& aRect);
    void AddSampleCategoriesL();
    void AddSampleAppsL();
    void UpdateAppListL();
    void AddContentTypesL();

    MWunderlandPage& Page() const;      // of the active tab
    CCoeControl* PageControl() const;   // the same object as a control
    void UpdatePage();                  // after the active tab changed

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
        TInt iCategory;      // index into iCategories
        TInt iRating;        // 1..5
        };

    CWunderlandTabBar* iTabBar;      // owned
    CWunderlandAppList* iAppList;    // owned
    CWunderlandCategoryList* iCategoryList;   // owned
    CWunderlandSearchPage* iSearchPage;       // owned
    CWunderlandTooltip* iTooltip;    // owned
    CPeriodic* iTooltipTimer;        // owned; hides the tooltip
    RArray<TApp> iApps;
    CDesCArrayFlat* iCategories;     // owned
    };

#endif // __WUNDERLANDCONTAINER_H__
