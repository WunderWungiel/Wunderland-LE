#ifndef __WUNDERLANDICONLOADER_H__
#define __WUNDERLANDICONLOADER_H__

#include <e32base.h>

class CFbsBitmap;
class CImageDecoder;
class CWunderlandAppList;
class CWunderlandApiClient;
class CWunderlandApiResponse;

class MWunderlandIconLoaderObserver
    {
public:
    // An icon finished decoding (or failed): the loader can take the API
    // client again, so the owner should call StartNextL() when it's idle.
    // Never called from inside a loader method.
    virtual void HandleIconLoaderReady() = 0;
    };

// Loads the icons (icon_url, PNG) of a CWunderlandAppList's rows, one at a
// time, through the API client the owner shares with its other requests:
// the owner calls StartNextL() whenever the client is idle and nothing more
// important waits, and passes the EWunderlandApiFile response back with
// HandleResponse(). The icon is decoded with the Image Conversion Library,
// scaled to the list's IconSize() (alpha kept as an EGray256 mask) and
// given to the list.
// Rows on screen go first, then the ones below them, then those above.
// An icon URL already loaded (the store's default icon, mostly) is reused
// without a request. A failed icon keeps the placeholder; a transport
// failure (no response at all) pauses the loading until Resume(), so a lost
// connection doesn't turn into a request (and an access point prompt) per row.
class CWunderlandIconLoader : public CActive
    {
public:
    static CWunderlandIconLoader* NewL(CWunderlandAppList& aList, CWunderlandApiClient& aClient,
                                       MWunderlandIconLoaderObserver& aObserver);
    ~CWunderlandIconLoader();

    // Forgets every row and the loaded icons (the list was reset); cancels
    // the client's request if it is ours.
    void Reset();
    // The icon URL of list row aRow; rows are added in increasing order.
    // An empty URL adds nothing.
    void AddL(TInt aRow, const TDesC& aUrl);
    void Resume();   // after a pause; does not start anything by itself

    // Starts the next icon's request on the client, which must be idle.
    // EFalse if there's nothing to do, an icon is being decoded or the
    // loading is paused.
    TBool StartNextL();
    TBool IsFetching() const;   // the client's running request is ours
    // The response to the request StartNextL() started.
    void HandleResponse(CWunderlandApiResponse& aResponse);

private: // from CActive
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);

private:
    enum TEntryState
        {
        EWaiting,
        EFetching,
        EDecoding,
        EDone,
        EFailed
        };

    class CEntry : public CBase
        {
    public:
        ~CEntry();
        TInt iRow;
        HBufC* iUrl;            // owned
        TEntryState iState;
        };

    // A decoded icon, kept for rows with the same URL. The bitmaps are
    // duplicates of the list's (the pixel data is shared).
    class CCachedIcon : public CBase
        {
    public:
        ~CCachedIcon();
        HBufC* iUrl;            // owned
        CFbsBitmap* iBitmap;    // owned
        CFbsBitmap* iMask;      // owned; may be NULL
        };

    CWunderlandIconLoader(CWunderlandAppList& aList, CWunderlandApiClient& aClient,
                          MWunderlandIconLoaderObserver& aObserver);

    TInt NextEntry() const;     // index into iEntries; KErrNotFound if none waits
    void StartDecodeL(HBufC8* aData);   // takes ownership of aData
    void DecodedL();            // scales, gives the icon to the rows, caches it
    void Fail();                // the current entry keeps the placeholder
    void ReleaseDecoder();
    void FinishDecode();        // back to idle; tells the observer
    const CCachedIcon* Cached(const TDesC& aUrl) const;
    // Gives aCached's icon (duplicated) to every waiting row with its URL.
    void ApplyL(const CCachedIcon& aCached);
    void ApplyToRowL(const CCachedIcon& aCached, TInt aRow);

private:
    CWunderlandAppList& iList;
    CWunderlandApiClient& iClient;
    MWunderlandIconLoaderObserver& iObserver;
    RPointerArray<CEntry> iEntries;        // owned; in row order
    RPointerArray<CCachedIcon> iCache;     // owned
    CEntry* iCurrent;            // fetched or decoded now; in iEntries; NULL: none
    TBool iPaused;
    // Decoding
    HBufC8* iData;               // owned; the PNG, alive while decoding
    CImageDecoder* iDecoder;     // owned
    CFbsBitmap* iDecoded;        // owned
    CFbsBitmap* iDecodedMask;    // owned; NULL if the image has no transparency
    };

#endif // __WUNDERLANDICONLOADER_H__
