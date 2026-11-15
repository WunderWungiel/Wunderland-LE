// Wunderland.cpp - application entry point
#include <eikstart.h>
#include "WunderlandApplication.h"

LOCAL_C CApaApplication* NewApplication()
    {
    return new CWunderlandApplication;   // the framework handles NULL
    }

GLDEF_C TInt E32Main()
    {
    return EikStart::RunApplication(NewApplication);
    }
