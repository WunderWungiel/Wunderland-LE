#ifndef __WUNDERLANDDOCUMENT_H__
#define __WUNDERLANDDOCUMENT_H__

#include <akndoc.h>

class CWunderlandDocument : public CAknDocument
    {
public:
    static CWunderlandDocument* NewL(CEikApplication& aApp);
    CEikAppUi* CreateAppUiL();
private:
    CWunderlandDocument(CEikApplication& aApp);
    };

#endif // __WUNDERLANDDOCUMENT_H__
