#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>
#include <badesca.h>

class CWunderlandTabBar;
class CWunderlandCategoryBar;
class CWunderlandDropdown;

// Main view: content-type tab bar on top; on content-type tabs (not Home) a
// "<Type> > <Category>" bar below it that opens a category drop-down;
// content (placeholder for now) below that.
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

    TBool HasCategoryBar() const;            // not on Home
    void UpdateCategoryBarL();               // text, visibility, drop-down items
    void OpenDropdownL();
    void CloseDropdownL();                   // restores the softkeys
    void SetSoftkeysL(TInt aResourceId);

private:
    CWunderlandTabBar* iTabBar;              // owned
    CWunderlandCategoryBar* iCategoryBar;    // owned
    CWunderlandDropdown* iDropdown;          // owned
    CDesCArrayFlat* iCategories;             // owned; shared by all content types for now
    RArray<TInt> iSelectedCategory;          // per tab; 0 = "All <type>"
    TBool iSwallowPointer;                   // eat the rest of a tap that closed the drop-down
    };

#endif // __WUNDERLANDCONTAINER_H__
