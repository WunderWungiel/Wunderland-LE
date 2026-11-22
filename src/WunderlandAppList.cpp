#include <aknutils.h>
#include <e32base.h>
#include <fbs.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandAppList.h"
#include "WunderlandDrawUtils.h"
#include <wunderlandle_0xe4f76ead_stars.mbg>

using WunderlandDraw::FillGradient;

namespace
    {
    const TInt KItemGranularity = 16;
    const TInt KMinRowHeight = 52;     // rows double as touch targets
    const TInt KMargin = 10;           // around the box; the scroll bar sits in the right one
    const TInt KTitleGap = 6;          // between the title and the box
    const TInt KBorder = 1;            // the box's outline
    const TInt KIconPad = 8;           // around the icon inside its row
    const TInt KRowVPad = 6;           // space above/below the text block
    const TInt KLineGap = 2;           // between the name and the subtitle
    const TInt KStarsGap = 3;          // between the subtitle and the stars
    const TInt KTextHPad = 10;         // left of the text (after the icon) and right of it
    const TInt KStarGap = 1;
    const TInt KSubtitlePercent = 85;  // of the secondary font's height
    const TInt KMaxStars = 5;
    const TInt KScrollBarWidth = 4;
    const TInt KMinThumbHeight = 8;
    const TInt KDragThreshold = 8;     // pixels before a tap becomes a drag
    const TInt KMaxDrawnText = 40;     // matches TItem's buffers
    // Ask for the next page when its loading row is this many rows (or
    // fewer) below the bottom of the screen, so it's usually there before
    // the user gets to it.
    const TInt KPrefetchRows = 3;

    _LIT(KLoadingText, "Loading...");
    _LIT(KLoadFailedText, "Loading failed. Press down or tap to retry");
    _LIT(KEmptyText, "No items");

    const TRgb KBackgroundColor()   { return TRgb(0xEF, 0xEF, 0xEF); }   // around the box
    const TRgb KBoxColor()          { return KRgbWhite; }
    const TRgb KBorderColor()       { return TRgb(0xD4, 0xD4, 0xD4); }
    const TRgb KSeparatorColor()    { return TRgb(0xE6, 0xE6, 0xE6); }   // between rows
    const TRgb KTitleColor()        { return TRgb(0x5A, 0x5A, 0x5A); }
    // Highlight: dark grey gradient
    const TRgb KHiTopColor()        { return TRgb(0x96, 0x96, 0x96); }
    const TRgb KHiBottomColor()     { return TRgb(0x5E, 0x5E, 0x5E); }
    const TRgb KNameColor()         { return KRgbBlack; }
    const TRgb KSubtitleColor()     { return TRgb(0x80, 0x80, 0x80); }
    const TRgb KHiNameColor()       { return KRgbWhite; }
    const TRgb KHiSubtitleColor()   { return TRgb(0xE0, 0xE0, 0xE0); }
    const TRgb KIconTextColor()     { return KRgbWhite; }
    const TRgb KTrackColor()        { return TRgb(0xDC, 0xDC, 0xDC); }
    const TRgb KThumbColor()        { return TRgb(0x8A, 0x8A, 0x8A); }

    // Placeholder icon colours, picked from the app name
    TRgb IconColor(const TDesC& aName)
        {
        TUint hash = 0;
        for (TInt i = 0; i < aName.Length(); ++i)
            {
            hash = hash * 31 + aName[i];
            }
        switch (hash % 6)
            {
            case 0:  return TRgb(0x2E, 0x7D, 0xC8);   // blue
            case 1:  return TRgb(0x3C, 0xA0, 0x46);   // green
            case 2:  return TRgb(0xE0, 0x7A, 0x1E);   // orange
            case 3:  return TRgb(0x8E, 0x44, 0xB4);   // purple
            case 4:  return TRgb(0xC8, 0x32, 0x32);   // red
            default: return TRgb(0x1E, 0x9A, 0x9A);   // teal
            }
        }

    _LIT(KStarsFile, "\\resource\\apps\\WunderlandLE_0xE4F76EAD_stars.mbm");

    CFbsBitmap* LoadBitmapL(const TDesC& aFile, TInt aId)
        {
        CFbsBitmap* bitmap = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(bitmap);
        User::LeaveIfError(bitmap->Load(aFile, aId));
        CleanupStack::Pop(bitmap);
        return bitmap;
        }
    }

CWunderlandAppList* CWunderlandAppList::NewL(const CCoeControl& aParent)
    {
    CWunderlandAppList* self = new (ELeave) CWunderlandAppList;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandAppList::CWunderlandAppList()
    : iItems(KItemGranularity), iHighlighted(-1), iLoadState(ELoadDone)
    {
    }

void CWunderlandAppList::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iLoadCallBack = new (ELeave) CAsyncCallBack(
        TCallBack(CWunderlandAppList::LoadMoreCallBack, this), CActive::EPriorityStandard);

    TFileName file(KStarsFile);
    User::LeaveIfError(CompleteWithAppPath(file));   // drive the app is installed on
    iStarOn      = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_p_s);
    iStarOnMask  = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_p_s_mask);
    iStarOff     = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_g_s);
    iStarOffMask = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_g_s_mask);
    UpdateFonts();
    }

void CWunderlandAppList::UpdateFonts()
    {
    // Errors leave the previous (or the plain layout) font in use.
    iNameFont.Update(EAknLogicalFontPrimaryFont, 100, ETrue);
    iSubtitleFont.Update(EAknLogicalFontSecondaryFont, KSubtitlePercent, EFalse);
    }

CWunderlandAppList::~CWunderlandAppList()
    {
    if (iLoadCallBack)
        {
        iLoadCallBack->Cancel();
        }
    delete iLoadCallBack;
    delete iStarOn;
    delete iStarOnMask;
    delete iStarOff;
    delete iStarOffMask;
    iNameFont.Close();
    iSubtitleFont.Close();
    iItems.Close();
    iIcons.ResetAndDestroy();
    iIconMasks.ResetAndDestroy();
    }

void CWunderlandAppList::Reset()
    {
    iItems.Reset();
    iIcons.ResetAndDestroy();
    iIconMasks.ResetAndDestroy();
    iHighlighted = -1;
    iScrollY = 0;
    iDragged = EFalse;
    iLoadState = ELoadDone;
    iLoadCallBack->Cancel();
    DrawDeferred();
    }

void CWunderlandAppList::AppendL(const TDesC& aName, const TDesC& aSubtitle, TInt aRating)
    {
    TItem item;
    item.iName = aName.Left(item.iName.MaxLength());
    item.iSubtitle = aSubtitle.Left(item.iSubtitle.MaxLength());
    item.iRating = aRating < 0 ? 0 : (aRating > KMaxStars ? KMaxStars : aRating);
    // Room in all three arrays first, so they stay the same length
    iIcons.ReserveL(iItems.Count() + 1);
    iIconMasks.ReserveL(iItems.Count() + 1);
    iItems.AppendL(item);
    iIcons.Append(NULL);       // can't fail after ReserveL
    iIconMasks.Append(NULL);
    DrawDeferred();
    }

void CWunderlandAppList::SetIcon(TInt aIndex, CFbsBitmap* aBitmap, CFbsBitmap* aMask)
    {
    if (aIndex < 0 || aIndex >= iIcons.Count() || !aBitmap)
        {
        delete aBitmap;
        delete aMask;
        return;
        }
    delete iIcons[aIndex];
    iIcons[aIndex] = aBitmap;
    delete iIconMasks[aIndex];
    iIconMasks[aIndex] = aMask;
    DrawDeferred();
    }

TSize CWunderlandAppList::IconSize() const
    {
    const TInt size = RowHeight() - 2 * KIconPad;
    return TSize(size, size);
    }

TInt CWunderlandAppList::FirstVisible() const
    {
    const TInt first = (iScrollY - RowsTop()) / RowHeight();
    return first > 0 ? first : 0;
    }

void CWunderlandAppList::SetTitle(const TDesC& aTitle)
    {
    iTitle = aTitle.Left(iTitle.MaxLength());
    SetScroll(iScrollY);   // the content height changed
    DrawDeferred();
    }

TInt CWunderlandAppList::Count() const
    {
    return iItems.Count();
    }

// ---------------------------------------------------------------------------
// Paged loading

void CWunderlandAppList::SetLoader(MWunderlandAppListLoader* aLoader)
    {
    iLoader = aLoader;
    CheckLoadMore();
    }

void CWunderlandAppList::SetMoreAvailable(TBool aMore)
    {
    iLoadState = aMore ? ELoadPending : ELoadDone;
    if (!aMore)
        {
        iLoadCallBack->Cancel();
        SetScroll(iScrollY);   // the loading row is gone: re-clamp
        }
    DrawDeferred();
    CheckLoadMore();   // the new page may not fill the screen
    }

void CWunderlandAppList::SetLoadFailed()
    {
    iLoadState = ELoadFailed;
    iLoadCallBack->Cancel();
    DrawDeferred();
    }

TBool CWunderlandAppList::IsLoading() const
    {
    return iLoadState == ELoadRequested;
    }

TBool CWunderlandAppList::RetryLoad()
    {
    if (iLoadState != ELoadFailed)
        {
        return EFalse;
        }
    iLoadState = ELoadPending;
    DrawDeferred();
    CheckLoadMore();
    return ETrue;
    }

TBool CWunderlandAppList::NeedsMore() const
    {
    if (iLoadState != ELoadPending || !iLoader)
        {
        return EFalse;
        }
    // Pixels between the bottom of the screen and the top of the loading row
    const TInt below = RowsTop() + Count() * RowHeight() - (iScrollY + Rect().Height());
    if (below <= KPrefetchRows * RowHeight())
        {
        return ETrue;
        }
    return iHighlighted >= Count() - KPrefetchRows;
    }

void CWunderlandAppList::CheckLoadMore()
    {
    if (iLoadCallBack && NeedsMore() && !iLoadCallBack->IsActive())
        {
        iLoadCallBack->CallBack();
        }
    }

TInt CWunderlandAppList::LoadMoreCallBack(TAny* aSelf)
    {
    static_cast<CWunderlandAppList*>(aSelf)->DoLoadMore();
    return KErrNone;
    }

void CWunderlandAppList::DoLoadMore()
    {
    if (!NeedsMore())   // scrolled back up, reset, ... in the meantime
        {
        return;
        }
    iLoadState = ELoadRequested;
    TRAPD(err, iLoader->HandleLoadMoreL(*this));
    if (err != KErrNone && iLoadState == ELoadRequested)
        {
        SetLoadFailed();
        }
    }

TInt CWunderlandAppList::Highlighted() const
    {
    return iHighlighted;
    }

void CWunderlandAppList::SetHighlighted(TInt aIndex)
    {
    if (aIndex < -1 || aIndex >= Count())
        {
        aIndex = -1;
        }
    if (aIndex != iHighlighted)
        {
        iHighlighted = aIndex;
        DrawDeferred();
        }
    if (aIndex >= 0)
        {
        EnsureVisible(aIndex);
        }
    }

TBool CWunderlandAppList::MoveHighlight(TInt aDelta)
    {
    const TInt target = iHighlighted + aDelta;
    if (iHighlighted >= 0 && target == Count() && RetryLoad())
        {
        EnsureVisible(iHighlighted);   // brings the loading row into view
        return ETrue;
        }
    if (iHighlighted < 0 || target < 0 || target >= Count())
        {
        return EFalse;
        }
    SetHighlighted(target);
    return ETrue;
    }

// ---------------------------------------------------------------------------
// Geometry (control coordinates; rows are scrolled by iScrollY)

const CFont* CWunderlandAppList::NameFont() const
    {
    return iNameFont.Font();
    }

const CFont* CWunderlandAppList::SubtitleFont() const
    {
    return iSubtitleFont.Font();
    }

const CFont* CWunderlandAppList::TitleFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontTitleFont);   // large bold
    }

TInt CWunderlandAppList::RowHeight() const
    {
    // Room for all three lines whether or not this list shows subtitles,
    // so every list has the same row (and icon) size.
    const TInt textHeight = NameFont()->HeightInPixels() + KLineGap
                          + SubtitleFont()->HeightInPixels() + KStarsGap
                          + StarsHeight() + 2 * KRowVPad;
    return textHeight > KMinRowHeight ? textHeight : KMinRowHeight;
    }

TInt CWunderlandAppList::RowCount() const
    {
    return Count() + (iLoadState != ELoadDone ? 1 : 0);
    }

TInt CWunderlandAppList::BoxRows() const
    {
    const TInt rows = RowCount();
    return rows > 0 ? rows : 1;
    }

TInt CWunderlandAppList::RowsTop() const
    {
    TInt top = KMargin + KBorder;
    if (iTitle.Length() > 0)
        {
        top += TitleFont()->HeightInPixels() + KTitleGap;
        }
    return top;
    }

TInt CWunderlandAppList::ContentHeight() const
    {
    return RowsTop() + BoxRows() * RowHeight() + KBorder + KMargin;
    }

TRect CWunderlandAppList::RowRect(TInt aRow) const
    {
    const TRect rect(Rect());
    const TInt rowH = RowHeight();
    const TInt top = rect.iTl.iY - iScrollY + RowsTop() + aRow * rowH;
    return TRect(rect.iTl.iX + KMargin + KBorder, top, rect.iBr.iX - KMargin - KBorder, top + rowH);
    }

TBool CWunderlandAppList::HasScrollBar() const
    {
    return ContentHeight() > Rect().Height();
    }

TInt CWunderlandAppList::RowAt(const TPoint& aPos) const
    {
    if (!Rect().Contains(aPos))
        {
        return -1;
        }
    const TInt y = aPos.iY - Rect().iTl.iY + iScrollY - RowsTop();
    if (y < 0)
        {
        return -1;   // the title or the margin above the box
        }
    const TInt row = y / RowHeight();
    return row < RowCount() ? row : -1;
    }

TInt CWunderlandAppList::ItemAt(const TPoint& aPos) const
    {
    const TInt row = RowAt(aPos);
    return row < Count() ? row : -1;
    }

TBool CWunderlandAppList::IsLoadingRowAt(const TPoint& aPos) const
    {
    return iLoadState != ELoadDone && RowAt(aPos) == Count();
    }

void CWunderlandAppList::SetScroll(TInt aScrollY)
    {
    TInt maxScroll = ContentHeight() - Rect().Height();
    if (maxScroll < 0)
        {
        maxScroll = 0;
        }
    if (aScrollY > maxScroll)
        {
        aScrollY = maxScroll;
        }
    if (aScrollY < 0)
        {
        aScrollY = 0;
        }
    if (aScrollY != iScrollY)
        {
        iScrollY = aScrollY;
        DrawDeferred();
        }
    CheckLoadMore();
    }

void CWunderlandAppList::EnsureVisible(TInt aIndex)
    {
    const TInt rowH = RowHeight();
    // The first row brings the title back; the last one the margin under the box.
    const TInt top = aIndex == 0 ? 0 : RowsTop() + aIndex * rowH;
    // On the last row, also show the loading row under it
    const TInt shown = aIndex + (aIndex == Count() - 1 && iLoadState != ELoadDone ? 2 : 1);
    const TInt bottom = shown >= RowCount() ? ContentHeight() : RowsTop() + shown * rowH;
    if (top < iScrollY)
        {
        SetScroll(top);
        }
    else if (bottom > iScrollY + Rect().Height())
        {
        TInt scroll = bottom - Rect().Height();
        if (scroll > top)
            {
            scroll = top;   // screen shorter than the rows: the row itself wins
            }
        SetScroll(scroll);
        }
    else
        {
        CheckLoadMore();   // SetScroll() does it on the other paths
        }
    }

void CWunderlandAppList::SizeChanged()
    {
    UpdateFonts();         // the layout fonts follow the screen layout
    SetScroll(iScrollY);   // re-clamp after rotation / layout change
    if (iHighlighted >= 0)
        {
        EnsureVisible(iHighlighted);
        }
    }

// ---------------------------------------------------------------------------
// Input

void CWunderlandAppList::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
    // Only delivered on touch devices (S60 5th Edition). Dragging scrolls by
    // pixels; a tap without a drag highlights the row.
    const TPoint pos(aPointerEvent.iPosition);
    switch (aPointerEvent.iType)
        {
        case TPointerEvent::EButton1Down:
            iDragStartY = pos.iY;
            iDragStartScroll = iScrollY;
            iDragged = EFalse;
            break;
        case TPointerEvent::EDrag:
            {
            const TInt dy = iDragStartY - pos.iY;
            if (!iDragged && Abs(dy) >= KDragThreshold)
                {
                iDragged = ETrue;
                }
            if (iDragged)
                {
                SetScroll(iDragStartScroll + dy);
                }
            break;
            }
        case TPointerEvent::EButton1Up:
            if (!iDragged)
                {
                const TInt item = ItemAt(pos);
                if (item >= 0)
                    {
                    SetHighlighted(item);
                    ReportEventL(MCoeControlObserver::EEventStateChanged);
                    }
                else if (IsLoadingRowAt(pos))
                    {
                    RetryLoad();   // no-op unless the page had failed
                    }
                }
            iDragged = EFalse;
            break;
        default:
            break;
        }
    }

// ---------------------------------------------------------------------------
// Drawing

void CWunderlandAppList::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    gc.SetClippingRect(rect);

    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KBackgroundColor());
    gc.DrawRect(rect);

    const TInt top = rect.iTl.iY - iScrollY;   // of the content
    if (iTitle.Length() > 0)
        {
        const CFont* font = TitleFont();
        const TInt width = rect.Width() - 2 * KMargin;
        if (width > 0)
            {
            TBuf<KMaxDrawnText> text(iTitle);
            AknTextUtils::ClipToFit(text, *font, width);
            gc.UseFont(font);
            gc.SetBrushStyle(CGraphicsContext::ENullBrush);
            gc.SetPenStyle(CGraphicsContext::ESolidPen);
            gc.SetPenColor(KTitleColor());
            gc.DrawText(text, TPoint(rect.iTl.iX + KMargin, top + KMargin + font->AscentInPixels()));
            gc.DiscardFont();
            }
        }

    // The white box with its outline; the rows go inside
    const TInt rowH = RowHeight();
    const TInt boxRows = BoxRows();
    const TRect box(rect.iTl.iX + KMargin, top + RowsTop() - KBorder,
                    rect.iBr.iX - KMargin, top + RowsTop() + boxRows * rowH + KBorder);
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KBorderColor());
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KBoxColor());
    gc.DrawRect(box);

    const TInt count = Count();
    TInt first = (iScrollY - RowsTop()) / rowH;
    if (first < 0)
        {
        first = 0;
        }
    for (TInt i = first; i < boxRows; ++i)
        {
        const TRect row(RowRect(i));
        if (row.iTl.iY >= rect.iBr.iY)
            {
            break;
            }
        if (i < count)
            {
            DrawRow(gc, i, row);
            }
        else
            {
            // The loading row, or "No items" in an empty list
            DrawRowBackground(gc, i, row, EFalse);
            const TDesC& text = iLoadState == ELoadDone ? KEmptyText()
                              : (iLoadState == ELoadFailed ? KLoadFailedText() : KLoadingText());
            WunderlandDraw::DrawStatusText(gc, TRect(row.iTl.iX + KTextHPad, row.iTl.iY,
                                                     row.iBr.iX - KTextHPad, row.iBr.iY), text);
            }
        }

    if (HasScrollBar())
        {
        DrawScrollBar(gc);
        }
    gc.CancelClippingRect();
    }

// The box is already white: only the highlight and the line under the row
// (not under the last one, the box outline is there) need drawing.
void CWunderlandAppList::DrawRowBackground(CWindowGc& aGc, TInt aRow, const TRect& aRect,
                                           TBool aHighlighted) const
    {
    if (aHighlighted)
        {
        FillGradient(aGc, aRect, KHiTopColor(), KHiBottomColor());
        return;
        }
    if (aRow < BoxRows() - 1)
        {
        aGc.SetPenStyle(CGraphicsContext::ESolidPen);
        aGc.SetPenColor(KSeparatorColor());
        const TInt y = aRect.iBr.iY - 1;
        aGc.DrawLine(TPoint(aRect.iTl.iX, y), TPoint(aRect.iBr.iX, y));   // end point excluded
        }
    }

void CWunderlandAppList::DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow) const
    {
    const TItem& item = iItems[aIndex];
    const TBool highlighted = (aIndex == iHighlighted);
    DrawRowBackground(aGc, aIndex, aRow, highlighted);

    // Square icon, inset from the row's edges
    const TInt iconSize = aRow.Height() - 2 * KIconPad;
    const TRect iconRect(TPoint(aRow.iTl.iX + KIconPad, aRow.iTl.iY + KIconPad),
                         TSize(iconSize, iconSize));
    DrawIcon(aGc, aIndex, iconRect);

    // Name, subtitle and stars under each other, centred vertically as a block
    const TInt textLeft = iconRect.iBr.iX + KTextHPad;
    const TInt textWidth = aRow.iBr.iX - KTextHPad - textLeft;
    if (textWidth <= 0)
        {
        return;
        }
    const CFont* nameFont = NameFont();
    const CFont* subFont = SubtitleFont();
    const TBool hasSubtitle = item.iSubtitle.Length() > 0;
    TInt blockH = nameFont->HeightInPixels() + KStarsGap + StarsHeight();
    if (hasSubtitle)
        {
        blockH += KLineGap + subFont->HeightInPixels();
        }
    TInt y = aRow.iTl.iY + (aRow.Height() - blockH) / 2;

    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);

    TBuf<KMaxDrawnText> text(item.iName);
    AknTextUtils::ClipToFit(text, *nameFont, textWidth);   // "..." when too long
    aGc.UseFont(nameFont);
    aGc.SetPenColor(highlighted ? KHiNameColor() : KNameColor());
    aGc.DrawText(text, TPoint(textLeft, y + nameFont->AscentInPixels()));
    aGc.DiscardFont();
    y += nameFont->HeightInPixels();

    if (hasSubtitle)
        {
        y += KLineGap;
        text = item.iSubtitle;
        AknTextUtils::ClipToFit(text, *subFont, textWidth);
        aGc.UseFont(subFont);
        aGc.SetPenColor(highlighted ? KHiSubtitleColor() : KSubtitleColor());
        aGc.DrawText(text, TPoint(textLeft, y + subFont->AscentInPixels()));
        aGc.DiscardFont();
        y += subFont->HeightInPixels();
        }

    if (StarsWidth() <= textWidth)
        {
        DrawStars(aGc, item.iRating, TPoint(textLeft, y + KStarsGap));
        }
    }

void CWunderlandAppList::DrawIcon(CWindowGc& aGc, TInt aIndex, const TRect& aRect) const
    {
    const CFbsBitmap* icon = iIcons[aIndex];
    if (icon)
        {
        const CFbsBitmap* mask = iIconMasks[aIndex];
        const TRect source(icon->SizeInPixels());
        if (source.Size() == aRect.Size())
            {
            if (mask)
                {
                aGc.BitBltMasked(aRect.iTl, icon, source, mask, EFalse);
                }
            else
                {
                aGc.BitBlt(aRect.iTl, icon, source);
                }
            }
        else if (mask)   // made before a layout switch: stretched until reloaded
            {
            aGc.DrawBitmapMasked(aRect, icon, source, mask, EFalse);
            }
        else
            {
            aGc.DrawBitmap(aRect, icon, source);
            }
        return;
        }

    // Placeholder until the icon arrives (or if it can't be loaded): a
    // square in a colour derived from the name, with its first letter.
    const TItem& aItem = iItems[aIndex];
    aGc.SetPenStyle(CGraphicsContext::ENullPen);
    aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    aGc.SetBrushColor(IconColor(aItem.iName));
    aGc.DrawRect(aRect);

    if (aItem.iName.Length() == 0)
        {
        return;
        }
    const CFont* font = AknLayoutUtils::FontFromId(EAknLogicalFontTitleFont);
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KIconTextColor());
    const TInt baseline = (aRect.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    aGc.DrawText(aItem.iName.Left(1), aRect, baseline, CGraphicsContext::ECenter);
    aGc.DiscardFont();
    }

TInt CWunderlandAppList::StarsWidth() const
    {
    return KMaxStars * iStarOn->SizeInPixels().iWidth + (KMaxStars - 1) * KStarGap;
    }

TInt CWunderlandAppList::StarsHeight() const
    {
    return iStarOn->SizeInPixels().iHeight;
    }

// The small (9x8) stars at their native size, left to right from aTopLeft.
void CWunderlandAppList::DrawStars(CWindowGc& aGc, TInt aRating, const TPoint& aTopLeft) const
    {
    const TSize size(iStarOn->SizeInPixels());
    const TRect source(size);
    TPoint pos(aTopLeft);
    for (TInt s = 0; s < KMaxStars; ++s)
        {
        const TBool on = (s < aRating);
        aGc.BitBltMasked(pos, on ? iStarOn : iStarOff, source, on ? iStarOnMask : iStarOffMask, EFalse);
        pos.iX += size.iWidth + KStarGap;
        }
    }

// In the right margin, next to the box
void CWunderlandAppList::DrawScrollBar(CWindowGc& aGc) const
    {
    const TRect rect(Rect());
    const TRect track(rect.iBr.iX - 1 - KScrollBarWidth, rect.iTl.iY, rect.iBr.iX - 1, rect.iBr.iY);
    const TInt content = ContentHeight();
    const TInt view = rect.Height();
    TInt thumbH = track.Height() * view / content;
    if (thumbH < KMinThumbHeight)
        {
        thumbH = KMinThumbHeight;
        }
    const TInt maxScroll = content - view;
    const TInt thumbY = track.iTl.iY + (track.Height() - thumbH) * iScrollY / (maxScroll > 0 ? maxScroll : 1);
    aGc.SetPenStyle(CGraphicsContext::ENullPen);
    aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    aGc.SetBrushColor(KTrackColor());
    aGc.DrawRect(track);
    aGc.SetBrushColor(KThumbColor());
    aGc.DrawRect(TRect(track.iTl.iX + 1, thumbY, track.iBr.iX - 1, thumbY + thumbH));
    }
