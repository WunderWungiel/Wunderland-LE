#ifndef __WUNDERLANDAPICLIENT_H__
#define __WUNDERLANDAPICLIENT_H__

#include <e32base.h>
#include <es_sock.h>
#include <http/rhttpsession.h>
#include <http/rhttptransaction.h>
#include <http/mhttptransactioncallback.h>
#include <http/mhttpdatasupplier.h>
#include "WunderlandApiTypes.h"

class CWunderlandJsonValue;

// What a request was for, so one observer can tell responses apart.
enum TWunderlandApiRequest
    {
    EWunderlandApiServerInfo,     // GET /
    EWunderlandApiContentTypes,   // GET /content-types
    EWunderlandApiPlatforms,      // GET /platforms
    EWunderlandApiCategories,     // GET /categories
    EWunderlandApiContentList,    // GET /content (also ?ids=)
    EWunderlandApiContent,        // GET /content/{id}
    EWunderlandApiRelated,        // GET /content/{id}/related
    EWunderlandApiRating,         // GET /content/{id}/rating
    EWunderlandApiNewsList,       // GET /news
    EWunderlandApiNews,           // GET /news/{id}
    EWunderlandApiClients,        // GET /clients
    EWunderlandApiLegacy,         // GET /legacy/{prefix}/{old_id}
    EWunderlandApiUpdates,        // POST /updates
    EWunderlandApiUrl             // GET of a URL taken from a response (links.self, ...)
    };

// Filters, sorting and paging for GET /content. Empty strings and 0 mean
// "not sent" (server default). The strings are only read while the request
// is being started, so they may point to temporary data.
class TWunderlandContentQuery
    {
public:
    enum TSort { ESortDefault, ESortUpdatedAt, ESortCreatedAt, ESortId, ESortTitle, ESortRating };
    enum TOrder { EOrderDefault, EOrderAscending, EOrderDescending };

    TWunderlandContentQuery();

    TPtrC iType;           // content type id, "apps"
    TInt iCategory;        // category id, 0 = any
    TPtrC iPlatform;       // "s60v3" or "s60v3,s60v5"
    TPtrC iSearch;         // title search, 1-100 characters
    TPtrC iUid;            // "0x2001C3A2"
    TPtrC iUpdatedSince;   // timestamp with a zone, "2026-11-01T12:00:00Z"
    TSort iSort;
    TOrder iOrder;
    TInt iPage;            // 0 = first page
    TInt iPerPage;         // 0 = server default (10), max 50
    };

// A finished request. Owned by the client; valid during the callback only,
// unless the JSON is taken with TakeRoot().
class CWunderlandApiResponse : public CBase
    {
public:
    ~CWunderlandApiResponse();

    TWunderlandApiRequest Request() const;

    // KErrNone: a 2xx response with a JSON body; Data() is set.
    // Otherwise a Symbian error code:
    //   transport errors as reported by the HTTP stack (no network,
    //   DNS failure, KErrCancel, ...), KErrCorrupt for a body that isn't
    //   the expected JSON, KErrOverflow for an oversized body, and for
    //   HTTP errors KErrNotFound (404), KErrArgument (400, 413),
    //   KErrServerBusy (503) or KErrGeneral (anything else).
    TInt Error() const;
    TInt HttpStatus() const;           // 0 if no response arrived

    // From the error envelope: error.code ("not_found", "invalid_parameter",
    // ...) and error.details.parameter. KNullDesC when the server didn't send
    // a JSON error (a proxy page, a transport error). Switch on the code,
    // never on the message.
    const TDesC& ErrorCode() const;
    const TDesC& ErrorParameter() const;

    // The response's "data": an object or an array depending on the request.
    // NULL on error. List items can be read with the views of
    // WunderlandApiTypes.h, e.g. TWunderlandContent(&Data()->At(i)).
    const CWunderlandJsonValue* Data() const;
    TInt Count() const;                         // items in a "data" array, else 0
    const CWunderlandJsonValue* ItemAt(TInt aIndex) const;   // NULL if out of range
    TWunderlandPageMeta Meta() const;           // /content, /news, /related

    // Takes the whole parsed body (envelope included); Data() and every view
    // made from it then belong to the caller's copy. NULL if there is none.
    CWunderlandJsonValue* TakeRoot();

private:
    friend class CWunderlandApiClient;
    CWunderlandApiResponse(TWunderlandApiRequest aRequest);

    TWunderlandApiRequest iRequest;
    TInt iError;
    TInt iHttpStatus;
    CWunderlandJsonValue* iRoot;   // owned
    };

class MWunderlandApiObserver
    {
public:
    // Called once per started request, including failed ones, but not after
    // CancelRequest(). Always asynchronous: never from inside a StartXxxL().
    // The observer may start the next request or delete the client from
    // here. A leave is ignored.
    virtual void HandleApiResponseL(CWunderlandApiResponse& aResponse) = 0;
    };

// Client for the Wunderland Store JSON API v1. One request at a time: a
// StartXxxL() call while another request runs leaves with KErrInUse.
//
// The first request opens a network connection (the system may ask the user
// for an access point); it's kept and reused until the client is deleted.
//
// Responses aren't gzipped: the Symbian HTTP stack doesn't decompress, so no
// Accept-Encoding is sent.
//
// Needs the NetworkServices capability. The HTTP stack uses ECom: the
// application calls REComSession::FinalClose() once at exit (the AppUi does).
class CWunderlandApiClient : public CActive,
                             public MHTTPTransactionCallback,
                             public MHTTPDataSupplier
    {
public:
    static CWunderlandApiClient* NewL(MWunderlandApiObserver& aObserver);
    static CWunderlandApiClient* NewLC(MWunderlandApiObserver& aObserver);
    ~CWunderlandApiClient();

    // Default: http://ovi.wunderwungiel.pl/api/v1 (no trailing slash).
    void SetBaseUrlL(const TDesC8& aBaseUrl);

    TBool IsBusy() const;
    void CancelRequest();   // stops the running request; the observer isn't called

    // Endpoints. aPlatform is a platform id list ("s60v3,s60v5") or empty
    // for no filter; other empty strings and 0 mean "not sent".
    void StartServerInfoL();
    void StartContentTypesL();
    void StartPlatformsL();
    void StartCategoriesL(const TDesC& aType, const TDesC& aPlatform, TBool aNonEmpty = ETrue);
    void StartContentListL(const TWunderlandContentQuery& aQuery);
    void StartContentByIdsL(const RArray<TInt>& aIds);   // 1 to 50 ids
    void StartContentL(TInt aId);
    void StartRelatedL(TInt aId, const TDesC& aPlatform, TInt aLimit = 0);
    void StartRatingL(TInt aId);
    void StartNewsListL(TInt aPage = 0, TInt aPerPage = 0);
    void StartNewsL(TInt aId);
    void StartClientsL();
    void StartLegacyL(const TDesC& aPrefix, TInt aOldId);
    // Up to 100 apps per request, each version 1-64 characters (else
    // KErrArgument); aPlatform is one platform id or empty.
    void StartUpdatesL(const TDesC& aPlatform, const RArray<TWunderlandInstalledApp>& aInstalled);
    // GET any absolute API URL from a response (links.self, links.related,
    // ...). aRequest is passed back in the response.
    void StartUrlL(const TDesC& aUrl, TWunderlandApiRequest aRequest = EWunderlandApiUrl);

private: // from CActive
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);

private: // from MHTTPTransactionCallback
    void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent);
    TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent& aEvent);

private: // from MHTTPDataSupplier (the POST body)
    TBool GetNextDataPart(TPtrC8& aDataPart);
    void ReleaseData();
    TInt OverallDataSize();
    TInt Reset();

private:
    enum TState
        {
        EIdle,
        EConnecting,     // RConnection::Start pending
        ETransaction,    // HTTP transaction running
        ECompleting      // finished; RunL reports to the observer
        };

    CWunderlandApiClient(MWunderlandApiObserver& aObserver);
    void ConstructL();

    // Request building: iUrl gets the base URL plus aPath, then parameters.
    void BeginRequestL(TWunderlandApiRequest aRequest, const TDesC8& aPath);
    void PrepareResponseL(TWunderlandApiRequest aRequest);   // allocates iResponse
    void AppendL(const TDesC8& aText);
    void AppendIntL(TInt aValue);
    void AppendParamL(const TDesC8& aName, const TDesC& aValue);   // skipped if aValue is empty
    void AppendParamL(const TDesC8& aName, const TDesC8& aValue);
    void AppendIntParamL(const TDesC8& aName, TInt aValue);       // skipped if aValue <= 0
    void StartSimpleL(TWunderlandApiRequest aRequest, const TDesC8& aPath);
    void SendL();   // connects if needed, then submits

    void ConnectL();
    void SubmitL();
    void SetHeaderL(RHTTPHeaders aHeaders, TInt aField, const TDesC8& aValue);
    void AppendBodyL(const TDesC8& aData);
    void Finish(TInt aError);       // ends the transaction, reports from RunL
    void CloseTransaction();
    void ResetConnection();         // drops session and connection after a transport error
    void NotifyObserver();          // iState must already be EIdle; never fails

private:
    MWunderlandApiObserver& iObserver;
    TState iState;
    TBool iSessionOpen;
    TBool iConnected;               // iConnection started and given to iSession
    TBool iTransactionOpen;
    TBool iResetPending;            // ResetConnection() once out of the HTTP callback
    RSocketServ iSocketServ;
    RConnection iConnection;
    RHTTPSession iSession;
    RHTTPTransaction iTransaction;

    HBufC8* iBaseUrl;               // owned
    RBuf8 iUrl;                     // current request URL
    RBuf8 iRequestBody;             // POST body, empty for GET
    RBuf8 iResponseBody;
    TBool iPost;
    // Made when a request starts, so reporting it later can't run out of
    // memory and lose the callback.
    CWunderlandApiResponse* iResponse;   // owned
    TBool iResponseComplete;        // EResponseComplete seen: the body is whole
    TInt iHttpStatus;
    TInt iError;                    // result of the transaction
    };

#endif // __WUNDERLANDAPICLIENT_H__
