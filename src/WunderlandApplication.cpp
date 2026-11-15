#include "WunderlandApplication.h"
#include "WunderlandDocument.h"

TUid CWunderlandApplication::AppDllUid() const
    {
    return KUidWunderlandApp;
    }

CApaDocument* CWunderlandApplication::CreateDocumentL()
    {
    return CWunderlandDocument::NewL(*this);
    }
