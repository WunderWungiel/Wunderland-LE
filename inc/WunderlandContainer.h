#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>

class CWunderlandTabBar;

// Main view: category tab bar on top, content (placeholder for now) below.
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
private:
    CWunderlandTabBar* iTabBar;   // owned
    };

#endif // __WUNDERLANDCONTAINER_H__
