#include <http.h>
#include <uri8.h>
#include <utf.h>
#include "WunderlandApiClient.h"
#include "WunderlandJson.h"

namespace
    {
    _LIT8(KDefaultBaseUrl, "http://ovi.wunderwungiel.pl/api/v1");
    _LIT8(KUserAgent, "WunderlandLE/1.0 (SymbianOS/9; S60)");
    _LIT8(KAcceptJson, "application/json");
    _LIT8(KContentTypeJson, "application/json");   // UTF-8 is JSON's default
    _LIT8(KAcceptAny, "*/*");

    // Paths
    _LIT8(KPathRoot, "/");
    _LIT8(KPathContentTypes, "/content-types");
    _LIT8(KPathPlatforms, "/platforms");
    _LIT8(KPathCategories, "/categories");
    _LIT8(KPathContent, "/content");
    _LIT8(KPathContentItem, "/content/");
    _LIT8(KPathRelated, "/related");
    _LIT8(KPathRating, "/rating");
    _LIT8(KPathNews, "/news");
    _LIT8(KPathNewsItem, "/news/");
    _LIT8(KPathClients, "/clients");
    _LIT8(KPathLegacy, "/legacy/");
    _LIT8(KPathUpdates, "/updates");
    _LIT8(KSlash, "/");

    // Query parameters
    _LIT8(KParamType, "type");
    _LIT8(KParamCategory, "category");
    _LIT8(KParamPlatform, "platform");
    _LIT8(KParamQuery, "q");
    _LIT8(KParamUid, "uid");
    _LIT8(KParamUpdatedSince, "updated_since");
    _LIT8(KParamSort, "sort");
    _LIT8(KParamOrder, "order");
    _LIT8(KParamPage, "page");
    _LIT8(KParamPerPage, "per_page");
    _LIT8(KParamIds, "ids");
    _LIT8(KParamNonEmpty, "non_empty");
    _LIT8(KParamLimit, "limit");
    _LIT8(KValueFalse, "false");
    _LIT8(KValueAsc, "asc");
    _LIT8(KValueDesc, "desc");
    _LIT8(KSortUpdatedAt, "updated_at");
    _LIT8(KSortCreatedAt, "created_at");
    _LIT8(KSortId, "id");
    _LIT8(KSortTitle, "title");
    _LIT8(KSortRating, "rating");

    // Response envelope
    _LIT8(KKeyData, "data");
    _LIT8(KKeyMeta, "meta");
    _LIT8(KKeyError, "error");
    _LIT8(KKeyCode, "code");
    _LIT8(KKeyDetails, "details");
    _LIT8(KKeyParameter, "parameter");

    // POST /updates body
    _LIT8(KUpdatesBegin, "{\"installed\":[");
    _LIT8(KUpdatesUid, "{\"uid\":\"0x");
    _LIT8(KUpdatesVersion, "\",\"version\":\"");
    _LIT8(KUpdatesItemEnd, "\"}");
    _LIT8(KUpdatesPlatform, "],\"platform\":\"");
    _LIT8(KUpdatesEndWithPlatform, "\"}");
    _LIT8(KUpdatesEnd, "]}");
    _LIT8(KComma, ",");
    _LIT8(KUidFormat, "%08X");
    _LIT8(KEscapeFormat, "\\u%04x");

    const TInt KInitialUrlSize = 256;
    const TInt KInitialBodySize = 4096;
    const TInt KShrinkBodyAbove = 64 * 1024;     // keep a big buffer only while needed
    const TInt KMaxResponseSize = 1024 * 1024;   // a full 50-item page is ~60 KB
    const TInt KMaxFileSize = 256 * 1024;        // icons are a few KB
    const TInt KMaxIds = 50;
    const TInt KMaxInstalled = 100;
    const TInt KMaxVersionLength = 64;

    void GrowL(RBuf8& aBuf, TInt aExtra)
        {
        const TInt needed = aBuf.Length() + aExtra;
        if (needed > aBuf.MaxLength())
            {
            TInt size = aBuf.MaxLength() * 2;
            if (size < needed)
                {
                size = needed;
                }
            aBuf.ReAllocL(size);
            }
        }

    void AppendToL(RBuf8& aBuf, const TDesC8& aText)
        {
        GrowL(aBuf, aText.Length());
        aBuf.Append(aText);
        }

    // RFC 3986 unreserved characters stay as they are; everything else is %XX.
    TBool IsUnreserved(TUint aChar)
        {
        return (aChar >= 'A' && aChar <= 'Z') || (aChar >= 'a' && aChar <= 'z')
            || (aChar >= '0' && aChar <= '9')
            || aChar == '-' || aChar == '_' || aChar == '.' || aChar == '~';
        }

    // Appends aText (UTF-8) as a JSON string body, without the quotes.
    void AppendJsonStringL(RBuf8& aBuf, const TDesC8& aText)
        {
        for (TInt i = 0; i < aText.Length(); ++i)
            {
            TUint c = aText[i];
            GrowL(aBuf, 6);
            if (c == '"' || c == '\\')
                {
                aBuf.Append('\\');
                aBuf.Append(c);
                }
            else if (c < 0x20)
                {
                aBuf.AppendFormat(KEscapeFormat, c);
                }
            else
                {
                aBuf.Append(c);
                }
            }
        }
    }

// ---------------------------------------------------------------------------
// TWunderlandContentQuery
// ---------------------------------------------------------------------------

TWunderlandContentQuery::TWunderlandContentQuery()
    : iCategory(0), iSort(ESortDefault), iOrder(EOrderDefault), iPage(0), iPerPage(0)
    {
    }

// ---------------------------------------------------------------------------
// CWunderlandApiResponse
// ---------------------------------------------------------------------------

CWunderlandApiResponse::CWunderlandApiResponse(TWunderlandApiRequest aRequest)
    : iRequest(aRequest)
    {
    }

CWunderlandApiResponse::~CWunderlandApiResponse()
    {
    delete iRoot;
    delete iBody;
    }

const TDesC8& CWunderlandApiResponse::Body() const
    {
    return iBody ? static_cast<const TDesC8&>(*iBody) : KNullDesC8();
    }

HBufC8* CWunderlandApiResponse::TakeBody()
    {
    HBufC8* body = iBody;
    iBody = NULL;
    return body;
    }

TWunderlandApiRequest CWunderlandApiResponse::Request() const
    {
    return iRequest;
    }

TInt CWunderlandApiResponse::Error() const
    {
    return iError;
    }

TInt CWunderlandApiResponse::HttpStatus() const
    {
    return iHttpStatus;
    }

const TDesC& CWunderlandApiResponse::ErrorCode() const
    {
    const CWunderlandJsonValue* error = iRoot ? iRoot->Member(KKeyError) : NULL;
    return error ? error->MemberString(KKeyCode) : KNullDesC();
    }

const TDesC& CWunderlandApiResponse::ErrorParameter() const
    {
    const CWunderlandJsonValue* error = iRoot ? iRoot->Member(KKeyError) : NULL;
    const CWunderlandJsonValue* details = error ? error->Member(KKeyDetails) : NULL;
    return details ? details->MemberString(KKeyParameter) : KNullDesC();
    }

const CWunderlandJsonValue* CWunderlandApiResponse::Data() const
    {
    return (iError == KErrNone && iRoot) ? iRoot->Member(KKeyData) : NULL;
    }

TInt CWunderlandApiResponse::Count() const
    {
    const CWunderlandJsonValue* data = Data();
    return (data && data->Type() == CWunderlandJsonValue::EArray) ? data->Count() : 0;
    }

const CWunderlandJsonValue* CWunderlandApiResponse::ItemAt(TInt aIndex) const
    {
    return (aIndex >= 0 && aIndex < Count()) ? &Data()->At(aIndex) : NULL;
    }

TWunderlandPageMeta CWunderlandApiResponse::Meta() const
    {
    return TWunderlandPageMeta(iRoot ? iRoot->Member(KKeyMeta) : NULL);
    }

CWunderlandJsonValue* CWunderlandApiResponse::TakeRoot()
    {
    CWunderlandJsonValue* root = iRoot;
    iRoot = NULL;
    return root;
    }

// ---------------------------------------------------------------------------
// CWunderlandApiClient: construction
// ---------------------------------------------------------------------------

CWunderlandApiClient* CWunderlandApiClient::NewL(MWunderlandApiObserver& aObserver)
    {
    CWunderlandApiClient* self = NewLC(aObserver);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandApiClient* CWunderlandApiClient::NewLC(MWunderlandApiObserver& aObserver)
    {
    CWunderlandApiClient* self = new (ELeave) CWunderlandApiClient(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

CWunderlandApiClient::CWunderlandApiClient(MWunderlandApiObserver& aObserver)
    : CActive(EPriorityStandard), iObserver(aObserver), iState(EIdle)
    {
    CActiveScheduler::Add(this);
    }

void CWunderlandApiClient::ConstructL()
    {
    iBaseUrl = KDefaultBaseUrl().AllocL();
    iUrl.CreateL(KInitialUrlSize);
    iRequestBody.CreateL(KInitialUrlSize);
    iResponseBody.CreateL(KInitialBodySize);
    iSession.OpenL();
    iSessionOpen = ETrue;
    }

CWunderlandApiClient::~CWunderlandApiClient()
    {
    Cancel();
    CloseTransaction();
    if (iSessionOpen)
        {
        iSession.Close();
        }
    iConnection.Close();
    iSocketServ.Close();
    delete iBaseUrl;
    iUrl.Close();
    iRequestBody.Close();
    iResponseBody.Close();
    delete iResponse;
    }

void CWunderlandApiClient::SetBaseUrlL(const TDesC8& aBaseUrl)
    {
    HBufC8* url = aBaseUrl.AllocL();
    delete iBaseUrl;
    iBaseUrl = url;
    }

TBool CWunderlandApiClient::IsBusy() const
    {
    return iState != EIdle;
    }

void CWunderlandApiClient::CancelRequest()
    {
    switch (iState)
        {
        case EConnecting:
        case ECompleting:
            Cancel();
            break;
        case ETransaction:
            CloseTransaction();   // closing a transaction cancels it
            break;
        default:
            break;
        }
    iState = EIdle;
    if (iResetPending)
        {
        ResetConnection();
        }
    }

// ---------------------------------------------------------------------------
// Endpoints
// ---------------------------------------------------------------------------

void CWunderlandApiClient::StartServerInfoL()
    {
    StartSimpleL(EWunderlandApiServerInfo, KPathRoot);
    }

void CWunderlandApiClient::StartContentTypesL()
    {
    StartSimpleL(EWunderlandApiContentTypes, KPathContentTypes);
    }

void CWunderlandApiClient::StartPlatformsL()
    {
    StartSimpleL(EWunderlandApiPlatforms, KPathPlatforms);
    }

void CWunderlandApiClient::StartCategoriesL(const TDesC& aType, const TDesC& aPlatform, TBool aNonEmpty)
    {
    BeginRequestL(EWunderlandApiCategories, KPathCategories);
    AppendParamL(KParamType, aType);
    AppendParamL(KParamPlatform, aPlatform);
    if (!aNonEmpty)
        {
        AppendParamL(KParamNonEmpty, KValueFalse);
        }
    SendL();
    }

void CWunderlandApiClient::StartContentListL(const TWunderlandContentQuery& aQuery)
    {
    BeginRequestL(EWunderlandApiContentList, KPathContent);
    AppendParamL(KParamType, aQuery.iType);
    AppendIntParamL(KParamCategory, aQuery.iCategory);
    AppendParamL(KParamPlatform, aQuery.iPlatform);
    AppendParamL(KParamQuery, aQuery.iSearch);
    AppendParamL(KParamUid, aQuery.iUid);
    AppendParamL(KParamUpdatedSince, aQuery.iUpdatedSince);
    switch (aQuery.iSort)
        {
        case TWunderlandContentQuery::ESortUpdatedAt: AppendParamL(KParamSort, KSortUpdatedAt); break;
        case TWunderlandContentQuery::ESortCreatedAt: AppendParamL(KParamSort, KSortCreatedAt); break;
        case TWunderlandContentQuery::ESortId:        AppendParamL(KParamSort, KSortId);        break;
        case TWunderlandContentQuery::ESortTitle:     AppendParamL(KParamSort, KSortTitle);     break;
        case TWunderlandContentQuery::ESortRating:    AppendParamL(KParamSort, KSortRating);    break;
        default: break;
        }
    switch (aQuery.iOrder)
        {
        case TWunderlandContentQuery::EOrderAscending:  AppendParamL(KParamOrder, KValueAsc);  break;
        case TWunderlandContentQuery::EOrderDescending: AppendParamL(KParamOrder, KValueDesc); break;
        default: break;
        }
    AppendIntParamL(KParamPage, aQuery.iPage);
    AppendIntParamL(KParamPerPage, aQuery.iPerPage);
    SendL();
    }

void CWunderlandApiClient::StartContentByIdsL(const RArray<TInt>& aIds)
    {
    const TInt count = aIds.Count();
    if (count < 1 || count > KMaxIds)
        {
        User::Leave(KErrArgument);
        }
    BeginRequestL(EWunderlandApiContentList, KPathContent);
    AppendParamL(KParamIds, KNullDesC8);   // "?ids="
    for (TInt i = 0; i < count; ++i)
        {
        if (i > 0)
            {
            AppendL(KComma);   // a literal comma is allowed in a query
            }
        AppendIntL(aIds[i]);
        }
    SendL();
    }

void CWunderlandApiClient::StartContentL(TInt aId)
    {
    BeginRequestL(EWunderlandApiContent, KPathContentItem);
    AppendIntL(aId);
    SendL();
    }

void CWunderlandApiClient::StartRelatedL(TInt aId, const TDesC& aPlatform, TInt aLimit)
    {
    BeginRequestL(EWunderlandApiRelated, KPathContentItem);
    AppendIntL(aId);
    AppendL(KPathRelated);
    AppendParamL(KParamPlatform, aPlatform);
    AppendIntParamL(KParamLimit, aLimit);
    SendL();
    }

void CWunderlandApiClient::StartRatingL(TInt aId)
    {
    BeginRequestL(EWunderlandApiRating, KPathContentItem);
    AppendIntL(aId);
    AppendL(KPathRating);
    SendL();
    }

void CWunderlandApiClient::StartNewsListL(TInt aPage, TInt aPerPage)
    {
    BeginRequestL(EWunderlandApiNewsList, KPathNews);
    AppendIntParamL(KParamPage, aPage);
    AppendIntParamL(KParamPerPage, aPerPage);
    SendL();
    }

void CWunderlandApiClient::StartNewsL(TInt aId)
    {
    BeginRequestL(EWunderlandApiNews, KPathNewsItem);
    AppendIntL(aId);
    SendL();
    }

void CWunderlandApiClient::StartClientsL()
    {
    StartSimpleL(EWunderlandApiClients, KPathClients);
    }

void CWunderlandApiClient::StartLegacyL(const TDesC& aPrefix, TInt aOldId)
    {
    if (aPrefix.Length() == 0)
        {
        User::Leave(KErrArgument);
        }
    BeginRequestL(EWunderlandApiLegacy, KPathLegacy);
    HBufC8* prefix = CnvUtfConverter::ConvertFromUnicodeToUtf8L(aPrefix);
    CleanupStack::PushL(prefix);
    for (TInt i = 0; i < prefix->Length(); ++i)
        {
        if (!IsUnreserved((*prefix)[i]))
            {
            User::Leave(KErrArgument);   // prefixes are plain words ("applications")
            }
        }
    AppendL(*prefix);
    CleanupStack::PopAndDestroy(prefix);
    AppendL(KSlash);
    AppendIntL(aOldId);
    SendL();
    }

void CWunderlandApiClient::StartUpdatesL(const TDesC& aPlatform,
                                         const RArray<TWunderlandInstalledApp>& aInstalled)
    {
    const TInt count = aInstalled.Count();
    if (count > KMaxInstalled)
        {
        User::Leave(KErrArgument);
        }
    for (TInt i = 0; i < count; ++i)
        {
        const TInt length = aInstalled[i].iVersion.Length();
        if (length < 1 || length > KMaxVersionLength)
            {
            User::Leave(KErrArgument);   // the server would answer 400 installed[i].version
            }
        }
    BeginRequestL(EWunderlandApiUpdates, KPathUpdates);
    iPost = ETrue;

    // {"installed":[{"uid":"0x2001C3A2","version":"2.09(0)"},...],"platform":"s60v3"}
    AppendToL(iRequestBody, KUpdatesBegin);
    TBuf8<8> uid;
    for (TInt i = 0; i < count; ++i)
        {
        const TWunderlandInstalledApp& app = aInstalled[i];
        if (i > 0)
            {
            AppendToL(iRequestBody, KComma);
            }
        AppendToL(iRequestBody, KUpdatesUid);
        uid.Format(KUidFormat, app.iUid);
        AppendToL(iRequestBody, uid);
        AppendToL(iRequestBody, KUpdatesVersion);
        HBufC8* version = CnvUtfConverter::ConvertFromUnicodeToUtf8L(app.iVersion);
        CleanupStack::PushL(version);
        AppendJsonStringL(iRequestBody, *version);
        CleanupStack::PopAndDestroy(version);
        AppendToL(iRequestBody, KUpdatesItemEnd);
        }
    if (aPlatform.Length() > 0)
        {
        AppendToL(iRequestBody, KUpdatesPlatform);
        HBufC8* platform = CnvUtfConverter::ConvertFromUnicodeToUtf8L(aPlatform);
        CleanupStack::PushL(platform);
        AppendJsonStringL(iRequestBody, *platform);
        CleanupStack::PopAndDestroy(platform);
        AppendToL(iRequestBody, KUpdatesEndWithPlatform);
        }
    else
        {
        AppendToL(iRequestBody, KUpdatesEnd);
        }
    SendL();
    }

void CWunderlandApiClient::StartFileL(const TDesC& aUrl)
    {
    StartUrlL(aUrl, EWunderlandApiFile);
    }

void CWunderlandApiClient::StartUrlL(const TDesC& aUrl, TWunderlandApiRequest aRequest)
    {
    if (IsBusy())
        {
        User::Leave(KErrInUse);
        }
    if (aUrl.Length() == 0)
        {
        User::Leave(KErrArgument);
        }
    PrepareResponseL(aRequest);
    iPost = EFalse;
    iFile = (aRequest == EWunderlandApiFile);
    iUrl.Zero();
    iRequestBody.Zero();
    // The API's URLs are ASCII; UTF-8 keeps anything else intact.
    HBufC8* url = CnvUtfConverter::ConvertFromUnicodeToUtf8L(aUrl);
    CleanupStack::PushL(url);
    AppendL(*url);
    CleanupStack::PopAndDestroy(url);
    SendL();
    }

// ---------------------------------------------------------------------------
// Request building
// ---------------------------------------------------------------------------

void CWunderlandApiClient::StartSimpleL(TWunderlandApiRequest aRequest, const TDesC8& aPath)
    {
    BeginRequestL(aRequest, aPath);
    SendL();
    }

void CWunderlandApiClient::BeginRequestL(TWunderlandApiRequest aRequest, const TDesC8& aPath)
    {
    if (IsBusy())
        {
        User::Leave(KErrInUse);
        }
    PrepareResponseL(aRequest);
    iPost = EFalse;
    iFile = EFalse;
    iUrl.Zero();
    iRequestBody.Zero();
    AppendL(*iBaseUrl);
    AppendL(aPath);
    }

void CWunderlandApiClient::PrepareResponseL(TWunderlandApiRequest aRequest)
    {
    CWunderlandApiResponse* response = new (ELeave) CWunderlandApiResponse(aRequest);
    delete iResponse;
    iResponse = response;
    }

void CWunderlandApiClient::AppendL(const TDesC8& aText)
    {
    AppendToL(iUrl, aText);
    }

void CWunderlandApiClient::AppendIntL(TInt aValue)
    {
    TBuf8<12> number;   // "-2147483648"
    number.Num(aValue);
    AppendL(number);
    }

void CWunderlandApiClient::AppendParamL(const TDesC8& aName, const TDesC& aValue)
    {
    if (aValue.Length() == 0)
        {
        return;
        }
    HBufC8* value = CnvUtfConverter::ConvertFromUnicodeToUtf8L(aValue);
    CleanupStack::PushL(value);
    AppendParamL(aName, *value);
    CleanupStack::PopAndDestroy(value);
    }

void CWunderlandApiClient::AppendParamL(const TDesC8& aName, const TDesC8& aValue)
    {
    _LIT8(KPercentFormat, "%%%02X");
    TBuf8<1> separator;
    separator.Append(iUrl.Locate('?') == KErrNotFound ? '?' : '&');
    AppendL(separator);
    AppendL(aName);
    GrowL(iUrl, 1 + aValue.Length() * 3);
    iUrl.Append('=');
    for (TInt i = 0; i < aValue.Length(); ++i)
        {
        TUint c = aValue[i];
        if (IsUnreserved(c))
            {
            iUrl.Append(c);
            }
        else
            {
            iUrl.AppendFormat(KPercentFormat, c);
            }
        }
    }

void CWunderlandApiClient::AppendIntParamL(const TDesC8& aName, TInt aValue)
    {
    if (aValue <= 0)
        {
        return;
        }
    TBuf8<12> number;
    number.Num(aValue);
    AppendParamL(aName, number);
    }

// ---------------------------------------------------------------------------
// Connection and transaction
// ---------------------------------------------------------------------------

void CWunderlandApiClient::SendL()
    {
    if (!iSessionOpen)
        {
        iSession.OpenL();
        iSessionOpen = ETrue;
        }
    if (!iConnected)
        {
        ConnectL();   // RunL submits once the connection is up
        return;
        }
    TRAPD(err, SubmitL());
    if (err != KErrNone)
        {
        CloseTransaction();
        iState = EIdle;
        User::Leave(err);
        }
    }

void CWunderlandApiClient::ConnectL()
    {
    // An own connection, given to the HTTP session, so the access point is
    // asked for (at most) once and then reused for every request.
    iConnection.Close();
    iSocketServ.Close();
    User::LeaveIfError(iSocketServ.Connect());
    TInt err = iConnection.Open(iSocketServ);
    if (err != KErrNone)
        {
        iSocketServ.Close();
        User::Leave(err);
        }
    iConnection.Start(iStatus);   // default preferences: the system's AP choice or prompt
    SetActive();
    iState = EConnecting;
    }

void CWunderlandApiClient::SubmitL()
    {
    TUriParser8 uri;
    User::LeaveIfError(uri.Parse(iUrl));

    RStringPool pool = iSession.StringPool();
    RStringF method = pool.StringF(iPost ? HTTP::EPOST : HTTP::EGET, RHTTPSession::GetTable());
    iTransaction = iSession.OpenTransactionL(uri, *this, method);
    iTransactionOpen = ETrue;

    RHTTPHeaders headers = iTransaction.Request().GetHeaderCollection();
    SetHeaderL(headers, HTTP::EUserAgent, KUserAgent);
    SetHeaderL(headers, HTTP::EAccept, iFile ? KAcceptAny() : KAcceptJson());
    if (iPost)
        {
        SetHeaderL(headers, HTTP::EContentType, KContentTypeJson);
        iTransaction.Request().SetBody(*this);
        }

    iResponseBody.Zero();
    iResponseComplete = EFalse;
    iHttpStatus = 0;
    iError = KErrNone;
    iTransaction.SubmitL();
    iState = ETransaction;
    }

void CWunderlandApiClient::SetHeaderL(RHTTPHeaders aHeaders, TInt aField, const TDesC8& aValue)
    {
    RStringPool pool = iSession.StringPool();
    RStringF value = pool.OpenFStringL(aValue);
    CleanupClosePushL(value);
    aHeaders.SetFieldL(pool.StringF(aField, RHTTPSession::GetTable()), THTTPHdrVal(value));
    CleanupStack::PopAndDestroy(&value);
    }

void CWunderlandApiClient::AppendBodyL(const TDesC8& aData)
    {
    AppendToL(iResponseBody, aData);
    }

void CWunderlandApiClient::CloseTransaction()
    {
    if (iTransactionOpen)
        {
        iTransaction.Close();
        iTransactionOpen = EFalse;
        }
    }

void CWunderlandApiClient::Finish(TInt aError)
    {
    if (iState != ETransaction)
        {
        return;   // already finished
        }
    CloseTransaction();
    iError = aError;
    // A transport failure (no complete response arrived) may mean the
    // connection is gone: start afresh next time. Don't close the session
    // from inside its own callback, though.
    if (aError != KErrNone && !iResponseComplete
        && aError != KErrOverflow && aError != KErrNoMemory)
        {
        iResetPending = ETrue;
        }
    // Report from RunL, outside the HTTP callback, so the observer may start
    // a new request or delete us.
    iState = ECompleting;
    iStatus = KRequestPending;
    SetActive();
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
    }

void CWunderlandApiClient::ResetConnection()
    {
    iResetPending = EFalse;
    CloseTransaction();
    if (iSessionOpen)
        {
        iSession.Close();
        iSessionOpen = EFalse;   // reopened by the next SendL()
        }
    iConnection.Close();
    iSocketServ.Close();
    iConnected = EFalse;
    }

// ---------------------------------------------------------------------------
// CActive
// ---------------------------------------------------------------------------

void CWunderlandApiClient::RunL()
    {
    switch (iState)
        {
        case EConnecting:
            {
            if (iStatus.Int() != KErrNone)
                {
                iError = iStatus.Int();
                iHttpStatus = 0;
                ResetConnection();
                iState = EIdle;
                NotifyObserver();
                return;   // the observer may have deleted us
                }
            RStringPool pool = iSession.StringPool();
            RHTTPConnectionInfo info = iSession.ConnectionInfo();
            info.SetPropertyL(pool.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()),
                              THTTPHdrVal(iSocketServ.Handle()));
            info.SetPropertyL(pool.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()),
                              THTTPHdrVal(reinterpret_cast<TInt>(&iConnection)));
            iConnected = ETrue;
            SubmitL();   // a leave goes to RunError
            break;
            }
        case ECompleting:
            if (iResetPending)
                {
                ResetConnection();
                }
            iState = EIdle;
            NotifyObserver();
            return;   // the observer may have deleted us
        default:
            break;
        }
    }

void CWunderlandApiClient::DoCancel()
    {
    if (iState == EConnecting)
        {
        // RConnection has no CancelStart(); closing it completes the request.
        iConnection.Close();
        iSocketServ.Close();
        iConnected = EFalse;
        }
    // ECompleting: the request is already complete; nothing to cancel.
    }

TInt CWunderlandApiClient::RunError(TInt aError)
    {
    // Only the EConnecting branch of RunL can leave: setting up the session
    // or submitting the first transaction failed.
    CloseTransaction();
    if (!iConnected)
        {
        ResetConnection();
        }
    iError = aError;
    iHttpStatus = 0;
    iState = EIdle;
    NotifyObserver();
    return KErrNone;
    }

void CWunderlandApiClient::NotifyObserver()
    {
    // Nothing here may fail: every started request must reach the observer.
    // The response object was made when the request started.
    CWunderlandApiResponse* response = iResponse;
    if (!response)
        {
        return;   // not reached: every Start method makes one first
        }
    iResponse = NULL;
    response->iHttpStatus = iHttpStatus;

    TInt error = iError;
    const TBool success = (iHttpStatus >= 200 && iHttpStatus < 300);
    if (iFile && success && error == KErrNone)
        {
        response->iBody = iResponseBody.Alloc();   // NULL on failure
        if (!response->iBody)
            {
            error = KErrNoMemory;
            }
        }
    else if (iHttpStatus > 0 && iResponseBody.Length() > 0)
        {
        // Error bodies are parsed too, for ErrorCode(). A body that isn't
        // JSON (a proxy page) leaves iRoot NULL.
        TRAPD(parseError, response->iRoot = CWunderlandJsonValue::ParseL(iResponseBody));
        if (parseError == KErrNoMemory && error == KErrNone)
            {
            error = KErrNoMemory;
            }
        }
    if (error == KErrNone)
        {
        if (success)
            {
            if (!iFile && (!response->iRoot || !response->iRoot->Member(KKeyData)))
                {
                error = KErrCorrupt;
                }
            }
        else if (iHttpStatus == 404)
            {
            error = KErrNotFound;
            }
        else if (iHttpStatus == 400 || iHttpStatus == 413)
            {
            error = KErrArgument;
            }
        else if (iHttpStatus == 503)
            {
            error = KErrServerBusy;
            }
        else
            {
            error = KErrGeneral;
            }
        }
    response->iError = error;

    iResponseBody.Zero();
    if (iResponseBody.MaxLength() > KShrinkBodyAbove)
        {
        TRAP_IGNORE(iResponseBody.ReAllocL(KInitialBodySize));   // keeps the old buffer on failure
        }

    // Nothing of ours may be touched after this call: it may delete us.
    // The response is a local, so it can still be freed.
    TRAP_IGNORE(iObserver.HandleApiResponseL(*response));
    delete response;
    }

// ---------------------------------------------------------------------------
// MHTTPTransactionCallback
// ---------------------------------------------------------------------------

void CWunderlandApiClient::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
    {
    switch (aEvent.iStatus)
        {
        case THTTPEvent::EGotResponseHeaders:
            iHttpStatus = aTransaction.Response().StatusCode();
            break;

        case THTTPEvent::EGotResponseBodyData:
            {
            MHTTPDataSupplier* body = aTransaction.Response().Body();
            TPtrC8 part;
            body->GetNextDataPart(part);
            TInt err = KErrNone;
            if (iResponseBody.Length() + part.Length() > (iFile ? KMaxFileSize : KMaxResponseSize))
                {
                err = KErrOverflow;
                }
            else
                {
                TRAP(err, AppendBodyL(part));
                }
            body->ReleaseData();
            if (err != KErrNone)
                {
                Finish(err);
                }
            break;
            }

        case THTTPEvent::EResponseComplete:
            iResponseComplete = ETrue;
            break;

        case THTTPEvent::ESucceeded:
            Finish(KErrNone);
            break;

        case THTTPEvent::EFailed:
            // After a complete response, judge by the HTTP status (an error
            // page may end as EFailed). A missing or cut-off response is a
            // transport failure, even after a 2xx status line.
            Finish(iResponseComplete ? KErrNone : KErrCouldNotConnect);
            break;

        default:
            if (aEvent.iStatus < 0)
                {
                Finish(aEvent.iStatus);   // an error code from the stack
                }
            break;   // redirects and other progress events
        }
    }

TInt CWunderlandApiClient::MHFRunError(TInt aError, RHTTPTransaction /*aTransaction*/,
                                       const THTTPEvent& /*aEvent*/)
    {
    Finish(aError);
    return KErrNone;
    }

// ---------------------------------------------------------------------------
// MHTTPDataSupplier: the whole POST body in one part
// ---------------------------------------------------------------------------

TBool CWunderlandApiClient::GetNextDataPart(TPtrC8& aDataPart)
    {
    aDataPart.Set(iRequestBody);
    return ETrue;   // last part
    }

void CWunderlandApiClient::ReleaseData()
    {
    }

TInt CWunderlandApiClient::OverallDataSize()
    {
    return iRequestBody.Length();
    }

TInt CWunderlandApiClient::Reset()
    {
    return KErrNone;   // the body is in memory: it can always start over
    }
