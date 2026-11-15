#ifndef __WUNDERLANDAPPUI_H__
#define __WUNDERLANDAPPUI_H__

#include <aknappui.h>

class CWunderlandContainer;

class CWunderlandAppUi : public CAknAppUi
    {
public:
    void ConstructL();
    ~CWunderlandAppUi();
private: // from CEikAppUi
    void HandleCommandL(TInt aCommand);
    void HandleResourceChangeL(TInt aType);
private:
    CWunderlandContainer* iContainer;   // owned
    };

#endif // __WUNDERLANDAPPUI_H__
