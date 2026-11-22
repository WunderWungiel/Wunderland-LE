#include <akndef.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <eikbtgpc.h>
#include <eikon.hrh>
#include <e32math.h>
#include <gdi.h>
#include "WunderlandApiTypes.h"
#include "WunderlandAppList.h"
#include "WunderlandCategoryList.h"
#include "WunderlandContainer.h"
#include "WunderlandContentTypePage.h"
#include "WunderlandLE_0xE4F76EAD.rsg"
#include "WunderlandSearchPage.h"
#include "WunderlandSearchResultsPage.h"
#include "WunderlandTabBar.h"
#include "WunderlandTooltip.h"

namespace
    {
    const TInt KCategoryGranularity = 16;
    const TInt KContentTypes = 3;     // apps, games, themes
    const TInt KAppsPerType = 30;     // sample entries per content type
    const TInt KSampleSeed = 0x5EED;  // fixed: the same sample list on every start
    const TInt KTooltipTimeout = 1000000;   // microseconds the tab tooltip stays up
    const TInt KPageSize = 10;              // entries per loaded page (the API's default)
    const TInt KSampleLatency = 600000;     // microseconds a sample page takes to "download"
    const TInt KReferenceMaxAge = 1;        // hours categories are kept (the API's max-age)

    _LIT(KLoadingText, "Loading...");
    _LIT(KLoadFailedText, "Loading failed. Press down or tap to retry");
    _LIT(KNoContentTypes, "No content types");
    _LIT(KNoCategories, "No categories");

    // Pseudo-random number in 0..aRange-1
    TInt Random(TInt64& aSeed, TInt aRange)
        {
        return (Math::Rand(aSeed) & KMaxTInt) % aRange;
        }
    }

CWunderlandContainer* CWunderlandContainer::NewL(const TRect& aRect)
    {
    CWunderlandContainer* self = new (ELeave) CWunderlandContainer;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandContainer::CWunderlandContainer()
    : iOverlay(EOverlayNone), iOpenType(-1), iCategoriesWanted(-1), iCategoriesLoading(-1)
    {
    }

CWunderlandContainer::CContentType* CWunderlandContainer::CContentType::NewLC(
    const TDesC& aId, const TDesC& aName, TBool aShowCategories)
    {
    CContentType* self = new (ELeave) CContentType;
    CleanupStack::PushL(self);
    self->ConstructL(aId, aName);
    self->iShowCategories = aShowCategories;
    return self;
    }

void CWunderlandContainer::CContentType::ConstructL(const TDesC& aId, const TDesC& aName)
    {
    iId = aId.AllocL();
    iName = aName.AllocL();
    }

CWunderlandContainer::CContentType::~CContentType()
    {
    iCategoryIds.Close();
    delete iCategoryNames;
    delete iName;
    delete iId;
    }

void CWunderlandContainer::ConstructL(const TRect& aRect)
    {
    CreateWindowL();

    iTabBar = CWunderlandTabBar::NewL(*this);
    iTabBar->SetObserver(this);
    iAppList = CWunderlandAppList::NewL(*this);
    iAppList->SetObserver(this);
    _LIT(KHomeTitle, "Recommended");
    iAppList->SetTitle(KHomeTitle);
    iCategoryList = CWunderlandCategoryList::NewL(*this);
    iCategoryList->SetObserver(this);
    iCategoryList->SetRetryObserver(this);
    iSearchPage = CWunderlandSearchPage::NewL(*this, *this);
    iSearchPage->SetObserver(this);
    iContentTypePage = CWunderlandContentTypePage::NewL(*this);
    iContentTypePage->SetObserver(this);
    iContentTypePage->SetRetryObserver(this);
    iContentTypePage->MakeVisible(EFalse);
    iResultsPage = CWunderlandSearchResultsPage::NewL(*this, *this);
    iResultsPage->MakeVisible(EFalse);
    iTooltip = CWunderlandTooltip::NewL(*this);
    iTooltip->MakeVisible(EFalse);
    iTooltipTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    iLoadTimer = CPeriodic::NewL(CActive::EPriorityStandard);
    iApi = CWunderlandApiClient::NewL(*this);
    iResultsIcons = CWunderlandIconLoader::NewL(iResultsPage->List(), *iApi, *this);
    EnableDragEvents();   // touch scrolling of the list

    iHomeFeed.iList = iAppList;
    iResultsFeed.iList = &iResultsPage->List();
    iAppList->SetLoader(this);
    iResultsPage->List().SetLoader(this);

    AddSampleCategoriesL();
    AddSampleAppsL();
    UpdatePage();

    SetRect(aRect);   // lays out the children via SizeChanged()
    // The first page loads once the app is up; its first entry then gets the
    // highlight, like Ovi Store (FocusFirstRow).
    ResetFeed(iHomeFeed);
    LoadContentTypes();   // the Categories tab; the first request may ask for an access point
    ActivateL();
    // ActivateL() only reaches the page that is a component control right
    // now; a page that isn't activated ignores its own DrawDeferred() calls.
    iAppList->ActivateL();
    iCategoryList->ActivateL();
    iSearchPage->ActivateL();
    iContentTypePage->ActivateL();
    iResultsPage->ActivateL();
    }

CWunderlandContainer::~CWunderlandContainer()
    {
    delete iResultsIcons;   // uses iApi and the results list
    delete iApi;   // cancels a request, so no callback reaches a half-deleted container
    iContentTypes.ResetAndDestroy();
    if (iLoadTimer)
        {
        iLoadTimer->Cancel();
        }
    delete iLoadTimer;
    iApps.Close();
    iSampleCategories.ResetAndDestroy();
    if (iTooltipTimer)
        {
        iTooltipTimer->Cancel();
        }
    delete iTooltipTimer;
    delete iTooltip;
    delete iResultsPage;
    delete iContentTypePage;
    delete iSearchPage;
    delete iCategoryList;
    delete iAppList;
    delete iTabBar;
    }

// Category names for the sample entries of Home:
// one list each for apps, games and themes. The Categories tab doesn't use
// them; its content types and categories come from the API.
void CWunderlandContainer::AddSampleCategoriesL()
    {
    const TText* const apps[] =
        {
        _S("Camera, photos, videos"), _S("Emulator"), _S("Business"), _S("Communication"),
        _S("Entertainment"), _S("Maps & navigation"), _S("Music & audio"), _S("News & weather"),
        _S("Productivity"), _S("Social networks"), _S("Tools & utilities"), _S("Travel")
        };
    const TText* const games[] =
        {
        _S("Action"), _S("Adventure"), _S("Arcade"), _S("Board"), _S("Card & casino"),
        _S("Educational"), _S("Puzzle"), _S("Racing"), _S("Sports"), _S("Strategy")
        };
    const TText* const themes[] =
        {
        _S("Abstract"), _S("Animals"), _S("Cartoons"), _S("Holidays"), _S("Nature"),
        _S("Sports"), _S("Technology")
        };
    #define WUNDERLAND_COUNT(a) (static_cast<TInt>(sizeof(a) / sizeof((a)[0])))
    AddCategoriesL(apps, WUNDERLAND_COUNT(apps));
    AddCategoriesL(games, WUNDERLAND_COUNT(games));
    AddCategoriesL(themes, WUNDERLAND_COUNT(themes));
    #undef WUNDERLAND_COUNT
    }

void CWunderlandContainer::AddCategoriesL(const TText* const* aNames, TInt aCount)
    {
    CDesCArrayFlat* list = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    CleanupStack::PushL(list);
    for (TInt i = 0; i < aCount; ++i)
        {
        list->AppendL(TPtrC(aNames[i]));
        }
    iSampleCategories.AppendL(list);
    CleanupStack::Pop(list);
    }

// Random names, categories and ratings, generated from a fixed seed.
void CWunderlandContainer::AddSampleAppsL()
    {
    const TText* const appFirst[] =
        {
        _S("Smart"), _S("Pocket"), _S("Quick"), _S("Easy"), _S("Mobile"), _S("Tiny"),
        _S("Handy"), _S("Simple"), _S("Night"), _S("Open"), _S("Super"), _S("Daily")
        };
    const TText* const appSecond[] =
        {
        _S("Notes"), _S("Radio"), _S("Camera"), _S("Tracker"), _S("Browser"), _S("Weather"),
        _S("Tasks"), _S("Player"), _S("Maps"), _S("Chat"), _S("Clock"), _S("Reader")
        };
    const TText* const gameFirst[] =
        {
        _S("Space"), _S("Pixel"), _S("Turbo"), _S("Crazy"), _S("Jungle"), _S("Galaxy"),
        _S("Mini"), _S("Retro"), _S("Ninja"), _S("Castle")
        };
    const TText* const gameSecond[] =
        {
        _S("Racer"), _S("Golf"), _S("Quest"), _S("Blocks"), _S("Pinball"), _S("Chess"),
        _S("Rally"), _S("Puzzle"), _S("Defender"), _S("Bowling")
        };
    const TText* const themeFirst[] =
        {
        _S("Blue"), _S("Dark"), _S("Neon"), _S("Autumn"), _S("Ocean"), _S("Carbon"),
        _S("Spring"), _S("Sunset")
        };
    const TText* const themeSecond[] =
        {
        _S("Glass"), _S("Waves"), _S("Lines"), _S("Dreams"), _S("Leaves"), _S("Shine"),
        _S("Bubbles"), _S("Metal")
        };
    #define WUNDERLAND_COUNT(a) (static_cast<TInt>(sizeof(a) / sizeof((a)[0])))

    TInt64 seed = KSampleSeed;
    for (TInt type = 0; type < KContentTypes; ++type)
        {
        const TInt categories = iSampleCategories[type]->MdcaCount();
        const TText* const* first;
        const TText* const* second;
        TInt firstCount;
        TInt secondCount;
        switch (type)
            {
            case 0:   // apps
                first = appFirst;     firstCount = WUNDERLAND_COUNT(appFirst);
                second = appSecond;   secondCount = WUNDERLAND_COUNT(appSecond);
                break;
            case 1:   // games
                first = gameFirst;    firstCount = WUNDERLAND_COUNT(gameFirst);
                second = gameSecond;  secondCount = WUNDERLAND_COUNT(gameSecond);
                break;
            default:  // themes
                first = themeFirst;   firstCount = WUNDERLAND_COUNT(themeFirst);
                second = themeSecond; secondCount = WUNDERLAND_COUNT(themeSecond);
                break;
            }
        for (TInt i = 0; i < KAppsPerType; ++i)
            {
            TApp app;
            const TPtrC firstWord(first[Random(seed, firstCount)]);
            const TPtrC secondWord(second[Random(seed, secondCount)]);
            app.iName.Copy(firstWord.Left(app.iName.MaxLength()));
            if (app.iName.Length() + 1 + secondWord.Length() <= app.iName.MaxLength())
                {
                app.iName.Append(' ');
                app.iName.Append(secondWord);
                }
            app.iType = type;
            app.iCategory = categories > 0 ? Random(seed, categories) : 0;
            app.iRating = 1 + Random(seed, 5);
            iApps.AppendL(app);
            }
        }
    #undef WUNDERLAND_COUNT
    }

// One row: the entry's name, its category under it, its rating.
void CWunderlandContainer::AppendAppL(CWunderlandAppList& aList, TInt aIndex) const
    {
    const TApp& app = iApps[aIndex];
    TPtrC subtitle(KNullDesC);
    if (app.iType >= 0 && app.iType < iSampleCategories.Count()
        && app.iCategory >= 0 && app.iCategory < iSampleCategories[app.iType]->MdcaCount())
        {
        subtitle.Set(iSampleCategories[app.iType]->MdcaPoint(app.iCategory));
        }
    aList.AppendL(app.iName, subtitle, app.iRating);
    }

// ---------------------------------------------------------------------------
// Paged loading

CWunderlandContainer::TFeed* CWunderlandContainer::FeedFor(const CWunderlandAppList& aList)
    {
    if (&aList == iHomeFeed.iList)
        {
        return &iHomeFeed;
        }
    if (&aList == iResultsFeed.iList)
        {
        return &iResultsFeed;
        }
    return NULL;
    }

void CWunderlandContainer::ResetFeed(TFeed& aFeed)
    {
    if (&aFeed == &iResultsFeed)
        {
        iResultsIcons->Reset();   // cancels an icon request; the rows go below
        }
    if (iLoading == &aFeed)
        {
        if (&aFeed == &iResultsFeed)
            {
            iApi->CancelRequest();   // the running request is this feed's page
            }
        else
            {
            iLoadTimer->Cancel();
            }
        iLoading = NULL;
        }
    aFeed.iNextPage = 0;
    aFeed.iWanted = EFalse;
    aFeed.iList->Reset();
    aFeed.iList->SetMoreAvailable(ETrue);   // the list asks for the first page
    StartNextRequest();   // something may have been waiting for the cancelled page
    }

// The list calls this when the user nears its end. If the page can't start,
// StartNextLoad() shows the retry row.
void CWunderlandContainer::HandleLoadMoreL(CWunderlandAppList& aList)
    {
    TFeed* feed = FeedFor(aList);
    if (!feed)
        {
        User::Leave(KErrNotFound);   // the list shows the "loading failed" row
        }
    feed->iWanted = ETrue;
    StartNextRequest();
    }

// One page at a time, the page on screen first. A results page needs the
// API client; while it runs another request the page waits for
// StartNextRequest().
void CWunderlandContainer::StartNextLoad()
    {
    if (iLoading)
        {
        return;   // StartNextLoad() runs again when it arrives
        }
    const TBool resultsReady = iResultsFeed.iWanted && !iApi->IsBusy();
    TFeed* feed = NULL;
    if (resultsReady && iOverlay == EOverlaySearchResults)
        {
        feed = &iResultsFeed;
        }
    else if (iHomeFeed.iWanted)
        {
        feed = &iHomeFeed;
        }
    else if (resultsReady)
        {
        feed = &iResultsFeed;
        }
    if (!feed)
        {
        return;
        }
    feed->iWanted = EFalse;
    TRAPD(err, StartLoadL(*feed));
    if (err == KErrNone)
        {
        iLoading = feed;
        }
    else
        {
        feed->iList->SetLoadFailed();   // offers a retry
        }
    }

// Search results: GET /content?q=&page=&per_page=, answered in
// ApiResponseL(). Home: sample data after a pretend delay.
// TODO: Home from the API too (the same request without q).
void CWunderlandContainer::StartLoadL(TFeed& aFeed)
    {
    if (&aFeed == &iResultsFeed)
        {
        TWunderlandContentQuery query;
        query.iSearch.Set(iSearchQuery);
        query.iPage = aFeed.iNextPage + 1;   // the API counts pages from 1
        query.iPerPage = KPageSize;
        iApi->StartContentListL(query);
        return;
        }
    iLoadTimer->Cancel();
    iLoadTimer->Start(KSampleLatency, KSampleLatency,
                      TCallBack(CWunderlandContainer::LoadDoneCallBack, this));
    }

TInt CWunderlandContainer::LoadDoneCallBack(TAny* aSelf)
    {
    CWunderlandContainer* self = static_cast<CWunderlandContainer*>(aSelf);
    self->iLoadTimer->Cancel();   // CPeriodic repeats; one tick is enough
    TFeed* feed = self->iLoading;
    self->iLoading = NULL;
    if (feed)
        {
        TRAPD(err, self->DeliverPageL(*feed));
        if (err != KErrNone)
            {
            feed->iList->SetLoadFailed();
            }
        }
    self->StartNextRequest();
    return EFalse;
    }

// Appends page iNextPage of the sample entries (Home only).
void CWunderlandContainer::DeliverPageL(TFeed& aFeed)
    {
    CWunderlandAppList& list = *aFeed.iList;
    const TBool first = (list.Count() == 0);
    const TInt total = iApps.Count();
    const TInt start = aFeed.iNextPage * KPageSize;
    for (TInt pos = start; pos < start + KPageSize && pos < total; ++pos)
        {
        AppendAppL(list, pos);
        }
    ++aFeed.iNextPage;
    list.SetMoreAvailable(start + KPageSize < total);
    if (first)
        {
        FocusFirstRow(aFeed);
        }
    }

// One page of GET /content: a row per item (title, category, whole stars).
// A leave keeps iNextPage, so a retry asks for the same page again.
void CWunderlandContainer::ContentPageLoadedL(TFeed& aFeed, const CWunderlandApiResponse& aResponse)
    {
    User::LeaveIfError(aResponse.Error());
    CWunderlandAppList& list = *aFeed.iList;
    const TBool first = (list.Count() == 0);
    const TInt count = aResponse.Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TWunderlandContent item(aResponse.ItemAt(i));
        if (!item.IsValid())
            {
            continue;   // not a content object
            }
        list.AppendL(item.Title(), item.CategoryName(), item.Rating().Rounded());
        if (&aFeed == &iResultsFeed)
            {
            // Optional: without it the row keeps its placeholder
            TRAP_IGNORE(iResultsIcons->AddL(list.Count() - 1, item.IconUrl()));
            }
        }
    if (&aFeed == &iResultsFeed)
        {
        iResultsIcons->Resume();   // the connection works again
        }
    ++aFeed.iNextPage;
    // An empty page is past the end, whatever meta says
    list.SetMoreAvailable(count > 0 && aResponse.Meta().HasNextPage());
    if (first)
        {
        FocusFirstRow(aFeed);
        }
    }

// The first entry takes the highlight when the first page arrives, unless
// the user has put the focus elsewhere in the meantime.
void CWunderlandContainer::FocusFirstRow(TFeed& aFeed)
    {
    if (&aFeed == &iHomeFeed)
        {
        if (iOverlay == EOverlayNone && iTabBar->Active() == CWunderlandTabBar::ETabHome
            && !iTabBar->IsBarFocused() && iAppList->Highlighted() < 0 && iAppList->Count() > 0)
            {
            iAppList->SetHighlighted(0);
            }
        }
    else if (iOverlay == EOverlaySearchResults && iResultsPage->Highlighted() < 0)
        {
        // First result, or the search bar when there are none
        iResultsPage->SetHighlighted(iResultsPage->Count() > 1 ? 1 : 0);
        }
    }

// ---------------------------------------------------------------------------
// Store API: content types and categories
//
// The client runs one request at a time, shared with the search results
// pages. StartNextRequest() starts whatever waits, in this order: the
// categories of the open content type page, the content types, then a
// results page. A running request isn't cancelled for another one, except
// categories of a type whose page was left for another type's page.
// No platform filter yet: the user can't pick one, so the counts and the
// non-empty categories are over all platforms.

void CWunderlandContainer::LoadContentTypes()
    {
    TRAPD(err, LoadContentTypesL());
    if (err != KErrNone)
        {
        iContentTypesFailed = ETrue;
        TRAP_IGNORE(iCategoryList->SetStatusL(KLoadFailedText, ETrue));
        }
    }

void CWunderlandContainer::LoadContentTypesL()
    {
    if (iContentTypesWanted || iContentTypesLoading)
        {
        return;   // already on the way
        }
    iContentTypesFailed = EFalse;
    iCategoryList->SetStatusL(KLoadingText);
    iContentTypesWanted = ETrue;
    StartNextRequest();
    }

// Starts the waiting request with the highest priority if the client is
// idle. A failure to start shows on the page it was for.
void CWunderlandContainer::StartNextRequest()
    {
    if (iCategoriesWanted >= iContentTypes.Count())
        {
        iCategoriesWanted = -1;   // the content types were reloaded meanwhile
        }
    if (!iApi->IsBusy() && iCategoriesWanted >= 0)
        {
        const TInt type = iCategoriesWanted;
        iCategoriesWanted = -1;
        TRAPD(err, iApi->StartCategoriesL(*iContentTypes[type]->iId, KNullDesC));
        if (err == KErrNone)
            {
            iCategoriesLoading = type;
            }
        else if (iOpenType == type)
            {
            TRAP_IGNORE(ShowCategoriesFailedL());
            }
        }
    if (!iApi->IsBusy() && iContentTypesWanted)
        {
        iContentTypesWanted = EFalse;
        TRAPD(err, iApi->StartContentTypesL());
        if (err == KErrNone)
            {
            iContentTypesLoading = ETrue;
            }
        else
            {
            iContentTypesFailed = ETrue;
            TRAP_IGNORE(iCategoryList->SetStatusL(KLoadFailedText, ETrue));
            }
        }
    StartNextLoad();   // a results page if the client is still idle; Home anyway
    if (!iApi->IsBusy() && iOverlay == EOverlaySearchResults)
        {
        TRAP_IGNORE(iResultsIcons->StartNextL());   // last: icons wait for everything else
        }
    }

void CWunderlandContainer::HandleIconLoaderReady()
    {
    StartNextRequest();   // the next icon, if the client is idle
    }

void CWunderlandContainer::HandleApiResponseL(CWunderlandApiResponse& aResponse)
    {
    TRAP_IGNORE(ApiResponseL(aResponse));
    StartNextRequest();   // the client is idle again
    }

void CWunderlandContainer::ApiResponseL(CWunderlandApiResponse& aResponse)
    {
    switch (aResponse.Request())
        {
        case EWunderlandApiContentList:
            {
            if (iLoading == &iResultsFeed)
                {
                iLoading = NULL;
                TRAPD(err, ContentPageLoadedL(iResultsFeed, aResponse));
                if (err != KErrNone)
                    {
                    iResultsFeed.iList->SetLoadFailed();   // offers a retry
                    }
                }
            break;
            }
        case EWunderlandApiContentTypes:
            {
            iContentTypesLoading = EFalse;
            TRAPD(err, ContentTypesLoadedL(aResponse));
            if (err != KErrNone)
                {
                iContentTypes.ResetAndDestroy();
                iCategoryList->Reset();
                iContentTypesFailed = ETrue;
                iCategoryList->SetStatusL(KLoadFailedText, ETrue);
                }
            break;
            }
        case EWunderlandApiCategories:
            {
            const TInt type = iCategoriesLoading;
            TRAPD(err, CategoriesLoadedL(aResponse));
            iCategoriesLoading = -1;
            if (err != KErrNone && iOpenType == type)
                {
                ShowCategoriesFailedL();
                }
            break;
            }
        case EWunderlandApiFile:
            iResultsIcons->HandleResponse(aResponse);
            break;
        default:
            break;
        }
    }

// Replaces the rows of the Categories tab. A leave leaves a partial list;
// the caller then empties it and shows the retry line.
void CWunderlandContainer::ContentTypesLoadedL(const CWunderlandApiResponse& aResponse)
    {
    User::LeaveIfError(aResponse.Error());
    iContentTypes.ResetAndDestroy();
    iCategoryList->Reset();
    const TInt count = aResponse.Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TWunderlandContentType type(aResponse.ItemAt(i));
        if (type.Id().Length() == 0)
            {
            continue;   // not a content type object
            }
        CContentType* entry = CContentType::NewLC(type.Id(), type.Name(), type.ShowCategories());
        iContentTypes.AppendL(entry);
        CleanupStack::Pop(entry);
        iCategoryList->AppendL(type.Name());
        }
    iCategoryList->SetStatusL(iContentTypes.Count() > 0 ? KNullDesC() : KNoContentTypes());
    }

// Fills the categories of iCategoriesLoading and, if its page is open, the
// page. On a leave the type has no categories (iCategoryNames NULL).
void CWunderlandContainer::CategoriesLoadedL(const CWunderlandApiResponse& aResponse)
    {
    const TInt typeIndex = iCategoriesLoading;
    if (typeIndex < 0 || typeIndex >= iContentTypes.Count())
        {
        return;
        }
    CContentType& type = *iContentTypes[typeIndex];
    delete type.iCategoryNames;
    type.iCategoryNames = NULL;
    type.iCategoryIds.Reset();
    User::LeaveIfError(aResponse.Error());

    CDesCArrayFlat* names = new (ELeave) CDesCArrayFlat(KCategoryGranularity);
    CleanupStack::PushL(names);
    const TInt count = aResponse.Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TWunderlandCategory category(aResponse.ItemAt(i));
        if (category.Name().Length() == 0)
            {
            continue;   // not a category object
            }
        names->AppendL(category.Name());
        type.iCategoryIds.AppendL(category.Id());
        }
    CleanupStack::Pop(names);
    type.iCategoryNames = names;
    type.iLoadedAt.UniversalTime();

    if (iOverlay == EOverlayContentType && iOpenType == typeIndex)
        {
        iContentTypePage->SetCategoriesL(*names);
        iContentTypePage->SetStatusL(names->MdcaCount() > 0 ? KNullDesC() : KNoCategories());
        }
    }

void CWunderlandContainer::LoadCategories(TInt aType)
    {
    TRAPD(err, LoadCategoriesL(aType));
    if (err != KErrNone)
        {
        TRAP_IGNORE(ShowCategoriesFailedL());
        }
    }

void CWunderlandContainer::LoadCategoriesL(TInt aType)
    {
    if (aType < 0 || aType >= iContentTypes.Count())
        {
        return;
        }
    iContentTypePage->SetStatusL(KLoadingText);
    if (iCategoriesLoading == aType)
        {
        return;   // already on the way
        }
    if (iCategoriesLoading >= 0)
        {
        iApi->CancelRequest();   // another type's categories: its page was closed
        iCategoriesLoading = -1;
        }
    iCategoriesWanted = aType;   // replaces another type's that hadn't started
    StartNextRequest();
    }

// On the open content type page, unless its categories are on the way.
void CWunderlandContainer::ShowCategoriesFailedL()
    {
    if (iOverlay == EOverlayContentType && iOpenType != iCategoriesLoading)
        {
        iContentTypePage->SetStatusL(KLoadFailedText, ETrue);
        }
    }

TBool CWunderlandContainer::HasFreshCategories(TInt aType) const
    {
    const CContentType& type = *iContentTypes[aType];
    if (!type.iCategoryNames)
        {
        return EFalse;
        }
    TTime now;
    now.UniversalTime();
    // A clock set back counts as stale too
    return now >= type.iLoadedAt && now < type.iLoadedAt + TTimeIntervalHours(KReferenceMaxAge);
    }

TBool CWunderlandContainer::RetryOpenPageL()
    {
    if (iOverlay != EOverlayContentType || !iContentTypePage->CanRetry())
        {
        return EFalse;
        }
    LoadCategories(iOpenType);
    return ETrue;
    }

void CWunderlandContainer::HandleRetryL(CCoeControl& aPage)
    {
    if (&aPage == iCategoryList)
        {
        if (iContentTypesFailed)
            {
            LoadContentTypes();
            }
        }
    else if (&aPage == iContentTypePage)
        {
        RetryOpenPageL();
        }
    }

void CWunderlandContainer::HandleBackL()
    {
    switch (iOverlay)
        {
        case EOverlayContentType:
            CloseContentTypeL();
            break;
        case EOverlaySearchResults:
            CloseSearchResultsL();
            break;
        default:
            break;
        }
    }

// Covers the tab bar and the tab pages; the Categories tab stays active
// underneath with the opened row highlighted, ready for Back.
// A type without categories (show_categories false) has no page: it opens
// nothing for now.
// Categories loaded within the last hour are shown at once; otherwise they
// load under a "Loading..." line.
void CWunderlandContainer::OpenContentTypeL(TInt aType)
    {
    if (aType < 0 || aType >= iContentTypes.Count())
        {
        return;
        }
    const CContentType& type = *iContentTypes[aType];
    if (!type.iShowCategories)
        {
        // TODO: open the type's entries (GET /content?type=) once there is
        // a page for them.
        return;
        }
    iContentTypePage->SetContentL(*type.iName);
    TBool load = EFalse;
    if (HasFreshCategories(aType))
        {
        iContentTypePage->SetCategoriesL(*type.iCategoryNames);
        if (type.iCategoryNames->MdcaCount() == 0)
            {
            iContentTypePage->SetStatusL(KNoCategories);
            }
        }
    else
        {
        load = ETrue;
        }
    SetSoftkeysL(R_WUNDERLAND_SOFTKEYS_STOREMENU_BACK);
    iOverlay = EOverlayContentType;
    iOpenType = aType;
    HideTooltip();
    iContentTypePage->MakeVisible(ETrue);
    iContentTypePage->SetHighlighted(0);
    if (load)
        {
        LoadCategories(aType);   // after iOpenType: a failure shows on this page
        }
    DrawDeferred();
    }

void CWunderlandContainer::CloseContentTypeL()
    {
    if (iOverlay != EOverlayContentType)
        {
        return;
        }
    SetSoftkeysL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    iContentTypePage->SetHighlighted(-1);
    iContentTypePage->MakeVisible(EFalse);
    iTabBar->SetBarFocused(EFalse);
    iCategoryList->SetHighlighted(iOpenType);
    iOverlay = EOverlayNone;
    iOpenType = -1;
    DrawDeferred();
    }

// From the Search tab or from the results page itself (a new search).
// Empty queries are ignored.
void CWunderlandContainer::HandleSearchL(const TDesC& aQuery)
    {
    TBuf<KWunderlandMaxQuery> query(aQuery.Left(KWunderlandMaxQuery));
    query.Trim();
    if (query.Length() == 0)
        {
        return;
        }
    OpenSearchResultsL(query);
    }

// Covers the tab bar and the tab pages; the Search tab stays active
// underneath, ready for Back. Both search bars keep the last query.
void CWunderlandContainer::OpenSearchResultsL(const TDesC& aQuery)
    {
    iSearchPage->SetQueryL(aQuery);
    iResultsPage->SetQueryL(aQuery);
    iSearchQuery = aQuery.Left(iSearchQuery.MaxLength());
    if (iOverlay != EOverlaySearchResults)
        {
        iSearchPage->SetHighlighted(-1);   // only one editor may have the focus
        SetSoftkeysL(R_WUNDERLAND_SOFTKEYS_STOREMENU_BACK);
        iOverlay = EOverlaySearchResults;
        HideTooltip();
        iResultsPage->MakeVisible(ETrue);
        }
    // No highlight until the first page arrives (FocusFirstRow)
    iResultsPage->SetHighlighted(-1);
    ResetFeed(iResultsFeed);   // drops the old results, loads the new ones
    DrawDeferred();
    }

void CWunderlandContainer::CloseSearchResultsL()
    {
    if (iOverlay != EOverlaySearchResults)
        {
        return;
        }
    SetSoftkeysL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    iResultsPage->SetHighlighted(-1);
    iResultsPage->MakeVisible(EFalse);
    iOverlay = EOverlayNone;
    iTabBar->SetBarFocused(EFalse);
    iSearchPage->SetHighlighted(0);   // back in the field
    DrawDeferred();
    }

void CWunderlandContainer::SetSoftkeysL(TInt aResourceId)
    {
    CEikButtonGroupContainer* cba = CEikButtonGroupContainer::Current();
    if (cba)
        {
        cba->SetCommandSetL(aResourceId);
        cba->DrawDeferred();
        }
    }

MWunderlandPage& CWunderlandContainer::Page() const
    {
    switch (iTabBar->Active())
        {
        case CWunderlandTabBar::ETabCategories:
            return *iCategoryList;
        case CWunderlandTabBar::ETabSearch:
            return *iSearchPage;
        default:
            return *iAppList;
        }
    }

CCoeControl* CWunderlandContainer::PageControl() const
    {
    switch (iTabBar->Active())
        {
        case CWunderlandTabBar::ETabCategories:
            return iCategoryList;
        case CWunderlandTabBar::ETabSearch:
            return iSearchPage;
        default:
            return iAppList;
        }
    }

// Only the active page is a component control, so only it is drawn and
// gets pointer events. The others drop their highlight.
void CWunderlandContainer::UpdatePage()
    {
    const TInt active = iTabBar->Active();
    iAppList->MakeVisible(active == CWunderlandTabBar::ETabHome);
    iCategoryList->MakeVisible(active == CWunderlandTabBar::ETabCategories);
    iSearchPage->MakeVisible(active == CWunderlandTabBar::ETabSearch);
    if (active != CWunderlandTabBar::ETabHome)
        {
        iAppList->SetHighlighted(-1);
        }
    if (active != CWunderlandTabBar::ETabCategories)
        {
        iCategoryList->SetHighlighted(-1);
        }
    if (active != CWunderlandTabBar::ETabSearch)
        {
        iSearchPage->SetHighlighted(-1);
        }
    if (active == CWunderlandTabBar::ETabCategories && iContentTypesFailed)
        {
        LoadContentTypes();   // coming back to the tab retries
        }
    DrawDeferred();
    }

TRect CWunderlandContainer::ContentRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl.iX, iTabBar->Rect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    }

void CWunderlandContainer::FocusTabBarL()
    {
    Page().SetHighlighted(-1);
    iTabBar->SetBarFocused(ETrue);
    UpdateTooltipL();
    }

void CWunderlandContainer::FocusListL()
    {
    if (Page().Count() == 0)
        {
        if (iTabBar->Active() == CWunderlandTabBar::ETabCategories && iContentTypesFailed)
            {
            LoadContentTypes();   // Down into the empty tab retries
            }
        return;
        }
    iTabBar->SetBarFocused(EFalse);
    Page().SetHighlighted(0);
    UpdateTooltipL();
    }

// Right under the focused tab and as wide as it, text centred. Wider only if
// the text doesn't fit; then centred on the tab and kept on screen.
void CWunderlandContainer::UpdateTooltipL(TBool aRestart)
    {
    const TBool show = iTabBar->IsBarFocused() && (aRestart || iTooltip->IsVisible());
    if (show)
        {
        iTooltip->SetTextL(iTabBar->FocusedLabel());
        const TRect tab(iTabBar->TabRect(iTabBar->Focused()));
        TSize size(iTooltip->PreferredSize());
        TInt x = tab.iTl.iX;
        if (size.iWidth <= tab.Width())
            {
            size.iWidth = tab.Width();
            }
        else
            {
            x = tab.iTl.iX - (size.iWidth - tab.Width()) / 2;
            if (x + size.iWidth > Rect().iBr.iX)
                {
                x = Rect().iBr.iX - size.iWidth;
                }
            if (x < Rect().iTl.iX)
                {
                x = Rect().iTl.iX;
                }
            }
        iTooltip->SetRect(TRect(TPoint(x, iTabBar->Rect().iBr.iY), size));
        }
    iTooltipTimer->Cancel();
    if (show && aRestart)
        {
        iTooltipTimer->Start(KTooltipTimeout, KTooltipTimeout,
                             TCallBack(CWunderlandContainer::HideTooltipCallBack, this));
        }
    if (show || iTooltip->IsVisible())
        {
        iTooltip->MakeVisible(show);
        DrawDeferred();   // repaints the list where the tooltip was
        }
    }

TInt CWunderlandContainer::HideTooltipCallBack(TAny* aSelf)
    {
    static_cast<CWunderlandContainer*>(aSelf)->HideTooltip();
    return EFalse;
    }

void CWunderlandContainer::HideTooltip()
    {
    iTooltipTimer->Cancel();   // CPeriodic repeats; one tick is enough
    if (iTooltip->IsVisible())
        {
        iTooltip->MakeVisible(EFalse);
        DrawDeferred();
        }
    }

void CWunderlandContainer::SizeChanged()
    {
    if (!iTabBar)
        {
        return;
        }
    const TRect rect(Rect());
    iTabBar->SetRect(TRect(rect.iTl, TSize(rect.Width(), iTabBar->PreferredHeight(rect.Size()))));
    iAppList->SetRect(ContentRect());
    iCategoryList->SetRect(ContentRect());
    iSearchPage->SetRect(ContentRect());
    iContentTypePage->SetRect(rect);   // no tab bar over them
    iResultsPage->SetRect(rect);
    TRAP_IGNORE(UpdateTooltipL(EFalse));   // only leaves on OOM; the old tooltip stays
    }

void CWunderlandContainer::HandleResourceChange(TInt aType)
    {
    CCoeControl::HandleResourceChange(aType);   // forwards to the child controls
    if (aType == KEikDynamicLayoutVariantSwitch)
        {
        SizeChanged();   // bar height and fonts depend on the layout
        DrawDeferred();
        }
    }

TInt CWunderlandContainer::CountComponentControls() const
    {
    if (!iTabBar)
        {
        return 0;
        }
    return iOverlay != EOverlayNone ? 1 : 3;   // a full-screen page covers everything
    }

CCoeControl* CWunderlandContainer::ComponentControl(TInt aIndex) const
    {
    if (iOverlay != EOverlayNone)
        {
        if (aIndex != 0)
            {
            return NULL;
            }
        if (iOverlay == EOverlayContentType)
            {
            return iContentTypePage;
            }
        return iResultsPage;
        }
    switch (aIndex)
        {
        case 0:
            return iTabBar;
        case 1:
            return PageControl();
        case 2:
            return iTooltip;   // last: drawn over the list
        default:
            return NULL;
        }
    }

void CWunderlandContainer::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
    {
    if (aEventType != EEventStateChanged)
        {
        return;
        }
    if (aControl == iTabBar)
        {
        // Focus moved or a tab was activated (key or tap)
        if (!PageControl()->IsVisible())
            {
            UpdatePage();   // the active tab changed
            }
        if (iTabBar->IsBarFocused())
            {
            Page().SetHighlighted(-1);
            }
        UpdateTooltipL();
        }
    else if (aControl == iAppList || aControl == iCategoryList || aControl == iSearchPage)
        {
        // A tapped row takes the focus
        iTabBar->SetBarFocused(EFalse);
        UpdateTooltipL();
        if (aControl == iCategoryList)
            {
            OpenContentTypeL(iCategoryList->Highlighted());   // one tap opens, like Ovi Store
            }
        }
    // A tapped row on iContentTypePage is only highlighted.
    // TODO: open the category's entries once there is a page for them.
    }

void CWunderlandContainer::Draw(const TRect& /*aRect*/) const
    {
    // The child controls cover everything; this only keeps stale pixels
    // from showing through under the list.
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KRgbWhite);
    gc.DrawRect(ContentRect());
    }

TKeyResponse CWunderlandContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    // A focused search bar gets the keys first (text, cursor, Go); it leaves
    // Up/Down and the keys at its edges to the navigation below.
    CCoeControl* searchPage = NULL;
    if (iOverlay == EOverlaySearchResults)
        {
        searchPage = iResultsPage;
        }
    else if (iOverlay == EOverlayNone && iTabBar->Active() == CWunderlandTabBar::ETabSearch
             && !iTabBar->IsBarFocused())
        {
        searchPage = iSearchPage;
        }
    if (searchPage && searchPage->OfferKeyEventL(aKeyEvent, aType) == EKeyWasConsumed)
        {
        return EKeyWasConsumed;
        }
    if (aType != EEventKey)
        {
        return EKeyWasNotConsumed;
        }
    if (iOverlay == EOverlaySearchResults)
        {
        // Up/down through the bar and the results; Back is the right softkey.
        // TODO: OK opens the highlighted entry.
        switch (aKeyEvent.iCode)
            {
            case EKeyDownArrow:
            case EKeyUpArrow:
                if (iResultsPage->Highlighted() < 0)
                    {
                    iResultsPage->SetHighlighted(0);
                    }
                else
                    {
                    // No wrapping: stays on the bar / the last result
                    iResultsPage->MoveHighlight(aKeyEvent.iCode == EKeyDownArrow ? 1 : -1);
                    }
                return EKeyWasConsumed;
            default:
                return EKeyWasNotConsumed;
            }
        }
    if (iOverlay == EOverlayContentType)
        {
        // Content type page: up/down only; Back is the right softkey.
        // TODO: OK opens the highlighted category's entries.
        switch (aKeyEvent.iCode)
            {
            case EKeyDownArrow:
            case EKeyUpArrow:
                if (iContentTypePage->Highlighted() < 0)
                    {
                    iContentTypePage->SetHighlighted(0);
                    }
                // No wrapping: stays on the first / last row. Down on the
                // last row retries failed categories.
                else if (!iContentTypePage->MoveHighlight(aKeyEvent.iCode == EKeyDownArrow ? 1 : -1)
                         && aKeyEvent.iCode == EKeyDownArrow)
                    {
                    RetryOpenPageL();
                    }
                return EKeyWasConsumed;
            default:
                return EKeyWasNotConsumed;
            }
        }
    // Focus runs tab bar -> list rows; up/down move it.
    if (iTabBar->IsBarFocused())
        {
        if (aKeyEvent.iCode == EKeyDownArrow)
            {
            FocusListL();
            return EKeyWasConsumed;
            }
        const TKeyResponse response = iTabBar->OfferKeyEventL(aKeyEvent, aType);   // left/right/select
        if (aKeyEvent.iCode == EKeyOK || aKeyEvent.iCode == EKeyEnter)
            {
            // Selecting a tab (even the open one) goes to its first row.
            // An empty page has none, so the bar keeps the focus.
            FocusListL();
            }
        return response;
        }
    // In the page content, left/right switch tabs (S60 tab convention). Controls
    // that need these keys themselves (the search bar above, a future category
    // drop-down) must be offered the key before this point and get to keep it.
    switch (aKeyEvent.iCode)
        {
        case EKeyOK:
        case EKeyEnter:
            if (iTabBar->Active() == CWunderlandTabBar::ETabCategories && Page().Highlighted() >= 0)
                {
                OpenContentTypeL(Page().Highlighted());
                return EKeyWasConsumed;
                }
            return EKeyWasNotConsumed;
        case EKeyLeftArrow:
        case EKeyRightArrow:
            if (iTabBar->SwitchTabL(aKeyEvent.iCode == EKeyLeftArrow ? -1 : 1))
                {
                FocusListL();   // focus stays in the content: first row of the new page
                }
            return EKeyWasConsumed;
        case EKeyDownArrow:
            if (Page().Highlighted() < 0)
                {
                FocusListL();
                }
            else
                {
                Page().MoveHighlight(1);   // stays on the last row
                }
            return EKeyWasConsumed;
        case EKeyUpArrow:
            if (Page().Highlighted() < 0 || !Page().MoveHighlight(-1))
                {
                FocusTabBarL();   // first row: up into the tab bar
                }
            return EKeyWasConsumed;
        default:
            return EKeyWasNotConsumed;
        }
    }
