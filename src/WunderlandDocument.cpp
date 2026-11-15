#include "WunderlandDocument.h"
#include "WunderlandAppUi.h"

CWunderlandDocument* CWunderlandDocument::NewL(CEikApplication& aApp)
    {
    return new (ELeave) CWunderlandDocument(aApp);   // nothing that can leave to construct
    }

CWunderlandDocument::CWunderlandDocument(CEikApplication& aApp)
    : CAknDocument(aApp)
    {
    }

CEikAppUi* CWunderlandDocument::CreateAppUiL()
    {
    return new (ELeave) CWunderlandAppUi;   // the framework calls ConstructL()
    }
