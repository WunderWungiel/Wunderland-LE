#ifndef __WUNDERLANDCONTAINER_H__
#define __WUNDERLANDCONTAINER_H__

#include <coecntrl.h>

// Main (currently empty) view of the app.
class CWunderlandContainer : public CCoeControl
    {
public:
    static CWunderlandContainer* NewL(const TRect& aRect);
    ~CWunderlandContainer();
private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
private:
    CWunderlandContainer();
    void ConstructL(const TRect& aRect);
    };

#endif // __WUNDERLANDCONTAINER_H__
