#include <w32std.h>
#include "WunderlandAppList.h"
#include "WunderlandDrawUtils.h"
#include "WunderlandSearchResultsPage.h"

namespace
    {
    _LIT(KTitle, "Search results");
    }

CWunderlandSearchResultsPage* CWunderlandSearchResultsPage::NewL(const CCoeControl& aParent,
                                                                 MWunderlandSearchObserver& aObserver)
    {
    CWunderlandSearchResultsPage* self = new (ELeave) CWunderlandSearchResultsPage(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandSearchResultsPage::CWunderlandSearchResultsPage(MWunderlandSearchObserver& aObserver)
    : iObserver(aObserver)
    {
    }

void CWunderlandSearchResultsPage::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iBar = CWunderlandSearchBar::NewL(*this, *this);
    iList = CWunderlandAppList::NewL(*this);
    iList->SetObserver(this);
    }

CWunderlandSearchResultsPage::~CWunderlandSearchResultsPage()
    {
    delete iList;
    delete iBar;
    }

CWunderlandAppList& CWunderlandSearchResultsPage::List() const
    {
    return *iList;
    }

void CWunderlandSearchResultsPage::SetQueryL(const TDesC& aQuery)
    {
    iBar->SetQueryL(aQuery);
    }

TInt CWunderlandSearchResultsPage::Count() const
    {
    return 1 + iList->Count();
    }

TInt CWunderlandSearchResultsPage::Highlighted() const
    {
    if (iBar->FocusSpot() != CWunderlandSearchBar::EFocusNone)
        {
        return 0;
        }
    const TInt row = iList->Highlighted();
    return row >= 0 ? 1 + row : -1;
    }

void CWunderlandSearchResultsPage::SetHighlighted(TInt aIndex)
    {
    if (aIndex == 0)
        {
        iList->SetHighlighted(-1);
        if (iBar->FocusSpot() == CWunderlandSearchBar::EFocusNone)
            {
            iBar->SetFocusSpot(CWunderlandSearchBar::EFocusField);
            }
        }
    else
        {
        iBar->SetFocusSpot(CWunderlandSearchBar::EFocusNone);
        iList->SetHighlighted(aIndex > 0 ? aIndex - 1 : -1);   // -1 also for out of range
        }
    }

TBool CWunderlandSearchResultsPage::MoveHighlight(TInt aDelta)
    {
    const TInt current = Highlighted();
    if (current >= 1 && aDelta > 0)
        {
        return iList->MoveHighlight(aDelta);   // also retries a failed page at the end
        }
    const TInt target = current + aDelta;
    if (current < 0 || target < 0 || target >= Count())
        {
        return EFalse;
        }
    SetHighlighted(target);
    return ETrue;
    }

TKeyResponse CWunderlandSearchResultsPage::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    return iBar->OfferKeyEventL(aKeyEvent, aType);   // not consumed unless the bar has the focus
    }

void CWunderlandSearchResultsPage::HandleSearchBarTappedL(CWunderlandSearchBar& /*aBar*/)
    {
    iList->SetHighlighted(-1);
    }

void CWunderlandSearchResultsPage::HandleSearchRequestL(CWunderlandSearchBar& aBar)
    {
    TBuf<KWunderlandMaxQuery> query;
    aBar.GetQuery(query);
    iObserver.HandleSearchL(query);
    }

void CWunderlandSearchResultsPage::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
    {
    if (aControl == iList && aEventType == EEventStateChanged)
        {
        iBar->SetFocusSpot(CWunderlandSearchBar::EFocusNone);   // the tapped row has the focus
        // TODO: open the entry once there is a page for it.
        }
    }

TRect CWunderlandSearchResultsPage::HeaderRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl, TSize(rect.Width(), WunderlandDraw::PageHeaderHeight()));
    }

void CWunderlandSearchResultsPage::SizeChanged()
    {
    if (!iList)
        {
        return;
        }
    const TRect rect(Rect());
    const TInt barTop = HeaderRect().iBr.iY;
    const TInt barBottom = barTop + iBar->PreferredHeight();
    iBar->SetRect(TRect(rect.iTl.iX, barTop, rect.iBr.iX, barBottom));
    iList->SetRect(TRect(rect.iTl.iX, barBottom, rect.iBr.iX, rect.iBr.iY));
    }

TInt CWunderlandSearchResultsPage::CountComponentControls() const
    {
    return iList ? 2 : 0;
    }

CCoeControl* CWunderlandSearchResultsPage::ComponentControl(TInt aIndex) const
    {
    switch (aIndex)
        {
        case 0:
            return iBar;
        case 1:
            return iList;
        default:
            return NULL;
        }
    }

void CWunderlandSearchResultsPage::Draw(const TRect& /*aRect*/) const
    {
    // The bar and the list cover the rest
    WunderlandDraw::DrawPageHeader(SystemGc(), HeaderRect(), KTitle);
    }
