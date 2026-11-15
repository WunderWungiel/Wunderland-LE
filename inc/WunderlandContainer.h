#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>
#include <badesca.h>

class CWunderlandTabBar;
class CWunderlandCategoryBar;
class CWunderlandDropdown;
class CWunderlandAppList;

// Main view: content-type tab bar on top; on content-type tabs (not Home) a
// "<Type> > <Category>" bar below it that opens a category drop-down;
// the list of entries (sample data for now) below that.
class CWunderlandContainer : public CCoeControl, public MCoeControlObserver
    {
public:
    static CWunderlandContainer* NewL(const TRect& aRect);
    ~CWunderlandContainer();

    // Softkey commands while the drop-down is open. ETrue if handled.
    TBool HandleCommandL(TInt aCommand);

public: // from CCoeControl
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);
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
    void AddSampleContentTypesL();
    void AddSampleCategoriesL();
    void AddSampleAppsL();

    TRect ContentRect() const;               // below the tab bar / category bar

    TBool HasCategoryBar() const;            // not on Home
    void UpdateCategoryBarL();               // text, visibility, drop-down items
    void UpdateAppListL();                   // entries for the current tab + category
    void OpenDropdownL();
    void CloseDropdownL();                   // restores the softkeys
    void SetSoftkeysL(TInt aResourceId);

private:
    // One store entry (sample data until the store has an API)
    class TApp
        {
    public:
        TBuf<40> iName;
        TBuf<40> iPublisher;
        TInt iContentType;   // tab index
        TInt iCategory;      // index into iCategories
        TInt iRating;        // 1..5
        };

    CWunderlandTabBar* iTabBar;              // owned
    CWunderlandCategoryBar* iCategoryBar;    // owned
    CWunderlandDropdown* iDropdown;          // owned
    CWunderlandAppList* iAppList;            // owned
    RArray<TApp> iApps;                      // all entries of all content types
    CDesCArrayFlat* iCategories;             // owned; shared by all content types for now
    RArray<TInt> iSelectedCategory;          // per tab; 0 = "All <type>"
    TBool iSwallowPointer;                   // eat the rest of a tap that closed the drop-down
    };

#endif // __WUNDERLANDCONTAINER_H__
