#include <avkon.hrh>
#include <eikspane.h>
#include "WunderlandAppUi.h"
#include "WunderlandContainer.h"
#include "Wunderland.hrh"

void CWunderlandAppUi::ConstructL()
    {
    BaseConstructL(EAknEnableSkin);
    // Full screen except the softkeys (like Ovi Store): hide the status pane
    // before creating the container so ClientRect() already excludes it.
    CEikStatusPane* statusPane = StatusPane();
    if (statusPane)
        {
        statusPane->MakeVisible(EFalse);
        }
    iContainer = CWunderlandContainer::NewL(ClientRect());
    AddToStackL(iContainer);   // receive key events
    }

CWunderlandAppUi::~CWunderlandAppUi()
    {
    if (iContainer)
        {
        RemoveFromStack(iContainer);
        delete iContainer;
        }
    }

void CWunderlandAppUi::HandleCommandL(TInt aCommand)
    {
    switch (aCommand)
        {
        case EEikCmdExit:
        case EAknCmdExit:
        case EAknSoftkeyExit:
            Exit();
            break;
        case EAknSoftkeyBack:   // content type page, search results
            iContainer->HandleBackL();
            break;
        default:
            break;
        }
    }

void CWunderlandAppUi::HandleResourceChangeL(TInt aType)
    {
    CAknAppUi::HandleResourceChangeL(aType);
    if (aType == KEikDynamicLayoutVariantSwitch && iContainer)   // rotation / resolution change
        {
        iContainer->SetRect(ClientRect());
        }
    }
