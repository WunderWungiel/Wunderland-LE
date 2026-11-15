#ifndef __WUNDERLANDAPPLICATION_H__
#define __WUNDERLANDAPPLICATION_H__

#include <aknapp.h>

const TUid KUidWunderlandApp = { 0xE4F76EAD };   // must equal UID3 in the MMP

class CWunderlandApplication : public CAknApplication
    {
public: // from CApaApplication
    TUid AppDllUid() const;
protected: // from CApaApplication
    CApaDocument* CreateDocumentL();
    };

#endif // __WUNDERLANDAPPLICATION_H__
