#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>
#include <badesca.h>

class CWunderlandTabBar;
class CWunderlandTooltip;
class CWunderlandAppList;

// Main view: icon tab bar (Home / Categories / Search) on top, the list of
// entries (sample data for now) below it. While the tab bar has the d-pad
// focus, a tooltip under the focused tab shows its name.
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

    TRect ContentRect() const;    // below the tab bar
    void FocusTabBarL();          // takes the highlight off the list
    void FocusListL();            // first row; no-op on an empty list
    void UpdateTooltipL();        // text, position, visibility

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
    CWunderlandTooltip* iTooltip;    // owned
    RArray<TApp> iApps;
    CDesCArrayFlat* iCategories;     // owned
    };

#endif // __WUNDERLANDCONTAINER_H__
