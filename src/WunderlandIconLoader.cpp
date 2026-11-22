#include <coemain.h>
#include <fbs.h>
#include <imageconversion.h>
#include "WunderlandApiClient.h"
#include "WunderlandAppList.h"
#include "WunderlandIconLoader.h"

namespace
    {
    const TInt KMaxReduction = 3;          // ICL decoders reduce by 1/2, 1/4, 1/8
    const TInt KMaxDecodedSide = 1024;     // bigger (after reduction): not an icon
    const TInt KRgbBytes = 3;              // EColor16M: B, G, R
    const TUint KOpaque = 255;

    // Leaves aBitmap on the cleanup stack.
    CFbsBitmap* CreateBitmapLC(const TSize& aSize, TDisplayMode aMode)
        {
        CFbsBitmap* bitmap = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(bitmap);
        User::LeaveIfError(bitmap->Create(aSize, aMode));
        return bitmap;
        }

    CFbsBitmap* DuplicateLC(const CFbsBitmap& aBitmap)
        {
        CFbsBitmap* bitmap = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(bitmap);
        User::LeaveIfError(bitmap->Duplicate(aBitmap.Handle()));
        return bitmap;
        }

    // Scales aSource (any mode; with its mask, if any) to aSize as an
    // EColor16M bitmap and an EGray256 mask (NULL without a source mask).
    // Box filter: each target pixel averages the source pixels it covers
    // (the nearest one when enlarging), colours weighted by their alpha so
    // transparent pixels don't darken the edges. Both are left on the
    // cleanup stack: the bitmap, then the mask (also when NULL).
    void ScaleLC(const CFbsBitmap& aSource, const CFbsBitmap* aSourceMask, const TSize& aSize,
                 CFbsBitmap*& aBitmap, CFbsBitmap*& aMask)
        {
        const TSize from(aSource.SizeInPixels());
        const TInt sw = from.iWidth;
        const TInt sh = from.iHeight;
        const TInt dw = aSize.iWidth;
        const TInt dh = aSize.iHeight;
        if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
            {
            User::Leave(KErrArgument);
            }

        RBuf8 srcRgb;
        srcRgb.CreateL(CFbsBitmap::ScanLineLength(sw, EColor16M));
        CleanupClosePushL(srcRgb);
        RBuf8 srcAlpha;
        srcAlpha.CreateL(CFbsBitmap::ScanLineLength(sw, EGray256));
        CleanupClosePushL(srcAlpha);
        RBuf8 dstRgb;
        dstRgb.CreateL(CFbsBitmap::ScanLineLength(dw, EColor16M));
        CleanupClosePushL(dstRgb);
        RBuf8 dstAlpha;
        dstAlpha.CreateL(CFbsBitmap::ScanLineLength(dw, EGray256));
        CleanupClosePushL(dstAlpha);
        // Per target column: alpha-weighted B, G, R sums, alpha sum, pixel count
        const TInt KSums = 5;
        TUint32* sums = static_cast<TUint32*>(User::AllocL(dw * KSums * sizeof(TUint32)));
        CleanupStack::PushL(sums);

        CFbsBitmap* bitmap = CreateBitmapLC(aSize, EColor16M);
        CFbsBitmap* mask = NULL;
        if (aSourceMask)
            {
            mask = CreateBitmapLC(aSize, EGray256);
            }

        dstRgb.SetLength(dstRgb.MaxLength());
        dstAlpha.SetLength(dstAlpha.MaxLength());
        dstRgb.FillZ();
        dstAlpha.FillZ();
        for (TInt y = 0; y < dh; ++y)
            {
            const TInt sy0 = y * sh / dh;
            TInt sy1 = (y + 1) * sh / dh;
            if (sy1 <= sy0)
                {
                sy1 = sy0 + 1;
                }
            Mem::FillZ(sums, dw * KSums * sizeof(TUint32));
            for (TInt sy = sy0; sy < sy1; ++sy)
                {
                // GetScanLine converts to the mode asked for, whatever the
                // bitmap's own (EGray2 masks of simple transparency too).
                aSource.GetScanLine(srcRgb, TPoint(0, sy), sw, EColor16M);
                if (aSourceMask)
                    {
                    aSourceMask->GetScanLine(srcAlpha, TPoint(0, sy), sw, EGray256);
                    }
                for (TInt x = 0; x < dw; ++x)
                    {
                    const TInt sx0 = x * sw / dw;
                    TInt sx1 = (x + 1) * sw / dw;
                    if (sx1 <= sx0)
                        {
                        sx1 = sx0 + 1;
                        }
                    TUint32* sum = sums + x * KSums;
                    for (TInt sx = sx0; sx < sx1; ++sx)
                        {
                        const TUint a = aSourceMask ? srcAlpha[sx] : KOpaque;
                        const TInt p = sx * KRgbBytes;
                        sum[0] += srcRgb[p] * a;
                        sum[1] += srcRgb[p + 1] * a;
                        sum[2] += srcRgb[p + 2] * a;
                        sum[3] += a;
                        ++sum[4];
                        }
                    }
                }
            for (TInt x = 0; x < dw; ++x)
                {
                const TUint32* sum = sums + x * KSums;
                const TInt p = x * KRgbBytes;
                if (sum[3] > 0)
                    {
                    dstRgb[p]     = static_cast<TUint8>(sum[0] / sum[3]);
                    dstRgb[p + 1] = static_cast<TUint8>(sum[1] / sum[3]);
                    dstRgb[p + 2] = static_cast<TUint8>(sum[2] / sum[3]);
                    }
                else
                    {
                    dstRgb[p] = dstRgb[p + 1] = dstRgb[p + 2] = 0;
                    }
                dstAlpha[x] = static_cast<TUint8>(sum[4] > 0 ? sum[3] / sum[4] : 0);
                }
            bitmap->SetScanLine(dstRgb, y);
            if (mask)
                {
                mask->SetScanLine(dstAlpha, y);
                }
            }

        // Keep the bitmap and the mask on the stack for the caller; free the rest.
        if (mask)
            {
            CleanupStack::Pop(mask);
            }
        CleanupStack::Pop(bitmap);
        CleanupStack::PopAndDestroy(sums);   // User::Free via the TAny* item
        CleanupStack::PopAndDestroy(4, &srcRgb);   // dstAlpha, dstRgb, srcAlpha, srcRgb
        CleanupStack::PushL(bitmap);
        CleanupStack::PushL(mask);
        aBitmap = bitmap;
        aMask = mask;
        }
    }

// ---------------------------------------------------------------------------

CWunderlandIconLoader::CEntry::~CEntry()
    {
    delete iUrl;
    }

CWunderlandIconLoader::CCachedIcon::~CCachedIcon()
    {
    delete iUrl;
    delete iBitmap;
    delete iMask;
    }

CWunderlandIconLoader* CWunderlandIconLoader::NewL(CWunderlandAppList& aList,
                                                   CWunderlandApiClient& aClient,
                                                   MWunderlandIconLoaderObserver& aObserver)
    {
    return new (ELeave) CWunderlandIconLoader(aList, aClient, aObserver);
    }

CWunderlandIconLoader::CWunderlandIconLoader(CWunderlandAppList& aList,
                                             CWunderlandApiClient& aClient,
                                             MWunderlandIconLoaderObserver& aObserver)
    : CActive(EPriorityIdle), iList(aList), iClient(aClient), iObserver(aObserver)
    {
    CActiveScheduler::Add(this);
    }

CWunderlandIconLoader::~CWunderlandIconLoader()
    {
    Cancel();
    ReleaseDecoder();
    iEntries.ResetAndDestroy();
    iCache.ResetAndDestroy();
    }

void CWunderlandIconLoader::Reset()
    {
    if (IsFetching())
        {
        iClient.CancelRequest();   // no response comes for it
        }
    Cancel();                      // a decode in progress
    ReleaseDecoder();
    iCurrent = NULL;
    iEntries.ResetAndDestroy();
    iCache.ResetAndDestroy();
    iPaused = EFalse;
    }

void CWunderlandIconLoader::AddL(TInt aRow, const TDesC& aUrl)
    {
    if (aUrl.Length() == 0)
        {
        return;
        }
    const CCachedIcon* cached = Cached(aUrl);
    if (cached)
        {
        ApplyToRowL(*cached, aRow);
        return;
        }
    CEntry* entry = new (ELeave) CEntry;
    CleanupStack::PushL(entry);
    entry->iRow = aRow;
    entry->iUrl = aUrl.AllocL();
    entry->iState = EWaiting;
    iEntries.AppendL(entry);
    CleanupStack::Pop(entry);
    }

void CWunderlandIconLoader::Resume()
    {
    iPaused = EFalse;
    }

TBool CWunderlandIconLoader::IsFetching() const
    {
    return iCurrent && iCurrent->iState == EFetching;
    }

// The first waiting row from the top of the screen down, else from the top
// of the list.
TInt CWunderlandIconLoader::NextEntry() const
    {
    const TInt first = iList.FirstVisible();
    const TInt count = iEntries.Count();
    TInt fallback = KErrNotFound;
    for (TInt i = 0; i < count; ++i)
        {
        const CEntry& entry = *iEntries[i];
        if (entry.iState != EWaiting)
            {
            continue;
            }
        if (entry.iRow >= first)
            {
            return i;
            }
        if (fallback == KErrNotFound)
            {
            fallback = i;
            }
        }
    return fallback;
    }

TBool CWunderlandIconLoader::StartNextL()
    {
    if (iCurrent || iPaused)
        {
        return EFalse;
        }
    for (;;)
        {
        const TInt index = NextEntry();
        if (index == KErrNotFound)
            {
            return EFalse;
            }
        CEntry* entry = iEntries[index];
        const CCachedIcon* cached = Cached(*entry->iUrl);
        if (cached)
            {
            ApplyL(*cached);   // loaded meanwhile for another row; marks this one done
            continue;
            }
        TRAPD(err, iClient.StartFileL(*entry->iUrl));
        if (err == KErrArgument)
            {
            entry->iState = EFailed;   // not a usable URL: skip it for good
            continue;
            }
        User::LeaveIfError(err);
        entry->iState = EFetching;
        iCurrent = entry;
        return ETrue;
        }
    }

void CWunderlandIconLoader::HandleResponse(CWunderlandApiResponse& aResponse)
    {
    if (!IsFetching())
        {
        return;   // not ours (or reset in the meantime)
        }
    const TInt error = aResponse.Error();
    if (error != KErrNone)
        {
        if (aResponse.HttpStatus() == 0 && error != KErrNoMemory && error != KErrOverflow)
            {
            // No response at all: likely no connection. Try this row again
            // once the owner resumes us.
            iCurrent->iState = EWaiting;
            iCurrent = NULL;
            iPaused = ETrue;
            return;
            }
        Fail();
        return;
        }
    HBufC8* data = aResponse.TakeBody();
    iCurrent->iState = EDecoding;
    TRAPD(err, StartDecodeL(data));
    if (err != KErrNone)
        {
        ReleaseDecoder();
        Fail();
        }
    }

void CWunderlandIconLoader::StartDecodeL(HBufC8* aData)
    {
    ReleaseDecoder();
    iData = aData;
    if (!iData)
        {
        User::Leave(KErrCorrupt);
        }
    // The decoder reads iData while it runs; it's freed in ReleaseDecoder().
    iDecoder = CImageDecoder::DataNewL(CCoeEnv::Static()->FsSession(), *iData,
                                       CImageDecoder::EOptionNone);
    const TFrameInfo& info = iDecoder->FrameInfo(0);
    const TSize full(info.iOverallSizeInPixels);
    const TSize target(iList.IconSize());

    // The strongest reduction that still gives at least the icon's size:
    // less to decode and to scale.
    TSize size(full);
    for (TInt factor = 1; factor <= KMaxReduction; ++factor)
        {
        TSize reduced;
        if (iDecoder->ReducedSize(full, factor, reduced) != KErrNone
            || reduced.iWidth < target.iWidth || reduced.iHeight < target.iHeight)
            {
            break;
            }
        size = reduced;
        }
    if (size.iWidth <= 0 || size.iHeight <= 0)
        {
        User::Leave(KErrCorrupt);
        }
    if (size.iWidth > KMaxDecodedSide || size.iHeight > KMaxDecodedSide)
        {
        User::Leave(KErrTooBig);
        }

    iDecoded = CreateBitmapLC(size, EColor16M);
    CleanupStack::Pop(iDecoded);
    if (info.iFlags & TFrameInfo::ETransparencyPossible)
        {
        // An alpha channel needs an 8-bit mask; simple transparency a 1-bit one
        const TDisplayMode maskMode = (info.iFlags & TFrameInfo::EAlphaChannel) ? EGray256 : EGray2;
        iDecodedMask = CreateBitmapLC(size, maskMode);
        CleanupStack::Pop(iDecodedMask);
        iDecoder->Convert(&iStatus, *iDecoded, *iDecodedMask);
        }
    else
        {
        iDecoder->Convert(&iStatus, *iDecoded);
        }
    SetActive();
    }

void CWunderlandIconLoader::RunL()
    {
    User::LeaveIfError(iStatus.Int());   // RunError marks the row failed
    DecodedL();
    FinishDecode();
    }

TInt CWunderlandIconLoader::RunError(TInt /*aError*/)
    {
    Fail();
    FinishDecode();
    return KErrNone;
    }

void CWunderlandIconLoader::DoCancel()
    {
    if (iDecoder)
        {
        iDecoder->Cancel();
        }
    }

void CWunderlandIconLoader::DecodedL()
    {
    CFbsBitmap* bitmap = NULL;
    CFbsBitmap* mask = NULL;
    ScaleLC(*iDecoded, iDecodedMask, iList.IconSize(), bitmap, mask);

    CCachedIcon* cached = new (ELeave) CCachedIcon;
    cached->iBitmap = bitmap;
    cached->iMask = mask;
    CleanupStack::Pop(2);   // mask, bitmap: owned by cached now
    CleanupStack::PushL(cached);
    cached->iUrl = iCurrent->iUrl->AllocL();
    iCache.AppendL(cached);
    CleanupStack::Pop(cached);

    iCurrent->iState = EWaiting;   // ApplyL gives it its icon with the others
    ApplyL(*cached);
    }

void CWunderlandIconLoader::ApplyL(const CCachedIcon& aCached)
    {
    for (TInt i = 0; i < iEntries.Count(); ++i)
        {
        CEntry& entry = *iEntries[i];
        if (entry.iState == EWaiting && *entry.iUrl == *aCached.iUrl)
            {
            ApplyToRowL(aCached, entry.iRow);
            entry.iState = EDone;
            }
        }
    }

void CWunderlandIconLoader::ApplyToRowL(const CCachedIcon& aCached, TInt aRow)
    {
    CFbsBitmap* bitmap = DuplicateLC(*aCached.iBitmap);
    CFbsBitmap* mask = NULL;
    if (aCached.iMask)
        {
        mask = DuplicateLC(*aCached.iMask);
        CleanupStack::Pop(mask);
        }
    CleanupStack::Pop(bitmap);
    iList.SetIcon(aRow, bitmap, mask);   // takes both; never fails
    }

const CWunderlandIconLoader::CCachedIcon* CWunderlandIconLoader::Cached(const TDesC& aUrl) const
    {
    for (TInt i = 0; i < iCache.Count(); ++i)
        {
        if (*iCache[i]->iUrl == aUrl)
            {
            return iCache[i];
            }
        }
    return NULL;
    }

void CWunderlandIconLoader::Fail()
    {
    if (iCurrent)
        {
        iCurrent->iState = EFailed;
        iCurrent = NULL;
        }
    }

void CWunderlandIconLoader::ReleaseDecoder()
    {
    delete iDecoder;
    iDecoder = NULL;
    delete iDecoded;
    iDecoded = NULL;
    delete iDecodedMask;
    iDecodedMask = NULL;
    delete iData;
    iData = NULL;
    }

void CWunderlandIconLoader::FinishDecode()
    {
    if (iCurrent && iCurrent->iState == EDecoding)
        {
        iCurrent->iState = EFailed;   // not reached: DecodedL marks it done
        }
    iCurrent = NULL;
    ReleaseDecoder();
    iObserver.HandleIconLoaderReady();   // from RunL: outside our methods
    }
