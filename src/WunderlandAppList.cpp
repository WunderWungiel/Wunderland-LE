#include <aknutils.h>
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
    const TInt KMinRowHeight = 52;     // icon size floor; rows double as touch targets
    const TInt KRowVPad = 8;           // space above/below the two text lines
    const TInt KLineGap = 3;           // space between the name and the subtitle
    const TInt KTextHPad = 8;          // gap between icon, text and stars
    const TInt KRightPad = 4;          // stars inset from the right edge
    const TInt KStarGap = 1;
    const TInt KMaxStars = 5;
    const TInt KScrollBarWidth = 4;
    const TInt KMinThumbHeight = 8;
    const TInt KDragThreshold = 8;     // pixels before a tap becomes a drag
    const TInt KMaxDrawnText = 40;     // matches TItem's buffers

    const TRgb KEvenRowColor()      { return TRgb(0xE8, 0xE8, 0xE8); }   // first row, third, ...
    const TRgb KOddRowColor()       { return KRgbWhite; }
    // Highlight: the category bar's grey gradient
    const TRgb KHiTopColor()        { return TRgb(0x96, 0x96, 0x96); }
    const TRgb KHiBottomColor()     { return TRgb(0x5E, 0x5E, 0x5E); }
    const TRgb KNameColor()         { return KRgbBlack; }
    const TRgb KSubtitleColor()     { return TRgb(0x80, 0x80, 0x80); }
    const TRgb KHiNameColor()       { return KRgbWhite; }
    const TRgb KHiSubtitleColor()   { return TRgb(0xE0, 0xE0, 0xE0); }
    const TRgb KIconTextColor()     { return KRgbWhite; }
    const TRgb KEmptyTextColor()    { return TRgb(0x80, 0x80, 0x80); }
    const TRgb KTrackColor()        { return TRgb(0xE8, 0xE8, 0xE8); }
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
    : iItems(KItemGranularity), iHighlighted(-1)
    {
    }

void CWunderlandAppList::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);

    TFileName file(KStarsFile);
    User::LeaveIfError(CompleteWithAppPath(file));   // drive the app is installed on
    iStarOn      = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_p);
    iStarOnMask  = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_p_mask);
    iStarOff     = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_g);
    iStarOffMask = LoadBitmapL(file, EMbmWunderlandle_0xe4f76ead_starsStar_g_mask);
    }

CWunderlandAppList::~CWunderlandAppList()
    {
    delete iStarOn;
    delete iStarOnMask;
    delete iStarOff;
    delete iStarOffMask;
    iItems.Close();
    }

void CWunderlandAppList::Reset()
    {
    iItems.Reset();
    iHighlighted = -1;
    iScrollY = 0;
    iDragged = EFalse;
    DrawDeferred();
    }

void CWunderlandAppList::AppendL(const TDesC& aName, const TDesC& aSubtitle, TInt aRating)
    {
    TItem item;
    item.iName = aName.Left(item.iName.MaxLength());
    item.iSubtitle = aSubtitle.Left(item.iSubtitle.MaxLength());
    item.iRating = aRating < 0 ? 0 : (aRating > KMaxStars ? KMaxStars : aRating);
    iItems.AppendL(item);
    DrawDeferred();
    }

TInt CWunderlandAppList::Count() const
    {
    return iItems.Count();
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
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);   // small bold
    }

const CFont* CWunderlandAppList::SubtitleFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);
    }

TInt CWunderlandAppList::RowHeight() const
    {
    // Room for both text lines whether or not this list shows subtitles,
    // so every list has the same row (and icon) size.
    const TInt textHeight = NameFont()->HeightInPixels() + KLineGap
                          + SubtitleFont()->HeightInPixels() + 2 * KRowVPad;
    return textHeight > KMinRowHeight ? textHeight : KMinRowHeight;
    }

TInt CWunderlandAppList::ContentHeight() const
    {
    return Count() * RowHeight();
    }

TBool CWunderlandAppList::HasScrollBar() const
    {
    return ContentHeight() > Rect().Height();
    }

TInt CWunderlandAppList::ItemAt(const TPoint& aPos) const
    {
    if (!Rect().Contains(aPos))
        {
        return -1;
        }
    const TInt item = (aPos.iY - Rect().iTl.iY + iScrollY) / RowHeight();
    return item < Count() ? item : -1;
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
    }

void CWunderlandAppList::EnsureVisible(TInt aIndex)
    {
    const TInt rowH = RowHeight();
    const TInt top = aIndex * rowH;
    if (top < iScrollY)
        {
        SetScroll(top);
        }
    else if (top + rowH > iScrollY + Rect().Height())
        {
        SetScroll(top + rowH - Rect().Height());
        }
    }

void CWunderlandAppList::SizeChanged()
    {
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
    gc.SetBrushColor(KOddRowColor());
    gc.DrawRect(rect);

    const TInt count = Count();
    if (count == 0)
        {
        _LIT(KEmpty, "No items");
        const CFont* font = SubtitleFont();
        gc.UseFont(font);
        gc.SetBrushStyle(CGraphicsContext::ENullBrush);
        gc.SetPenStyle(CGraphicsContext::ESolidPen);
        gc.SetPenColor(KEmptyTextColor());
        const TRect line(rect.iTl, TSize(rect.Width(), RowHeight()));
        const TInt baseline = (line.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
        gc.DrawText(KEmpty, line, baseline, CGraphicsContext::ECenter);
        gc.DiscardFont();
        gc.CancelClippingRect();
        return;
        }

    const TInt rowH = RowHeight();
    TInt first = iScrollY / rowH;
    for (TInt i = first; i < count; ++i)
        {
        const TInt top = rect.iTl.iY + i * rowH - iScrollY;
        if (top >= rect.iBr.iY)
            {
            break;
            }
        DrawRow(gc, i, TRect(rect.iTl.iX, top, rect.iBr.iX, top + rowH));
        }

    if (HasScrollBar())
        {
        DrawScrollBar(gc);
        }
    gc.CancelClippingRect();
    }

void CWunderlandAppList::DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow) const
    {
    const TItem& item = iItems[aIndex];
    const TBool highlighted = (aIndex == iHighlighted);

    if (highlighted)
        {
        FillGradient(aGc, aRow, KHiTopColor(), KHiBottomColor());
        }
    else
        {
        aGc.SetPenStyle(CGraphicsContext::ENullPen);
        aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        aGc.SetBrushColor(aIndex % 2 == 0 ? KEvenRowColor() : KOddRowColor());
        aGc.DrawRect(aRow);
        }

    // Square icon filling the row's full height
    const TRect iconRect(aRow.iTl, TSize(aRow.Height(), aRow.Height()));
    DrawIcon(aGc, item, iconRect);

    // Stars stuck to the right (left of the scroll bar when there is one)
    const TInt right = aRow.iBr.iX - KRightPad - (HasScrollBar() ? KScrollBarWidth : 0);
    DrawStars(aGc, item.iRating, aRow, right);

    // Text between the icon and the stars
    const TInt textLeft = iconRect.iBr.iX + KTextHPad;
    const TInt textWidth = right - StarsWidth() - KTextHPad - textLeft;
    if (textWidth <= 0)
        {
        return;
        }

    const CFont* nameFont = NameFont();
    const CFont* subFont = SubtitleFont();
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);

    TInt nameBaseline;
    TInt subBaseline = 0;
    if (item.iSubtitle.Length() > 0)
        {
        // Name and subtitle as one block, centred vertically
        const TInt nameH = nameFont->HeightInPixels();
        const TInt blockH = nameH + KLineGap + subFont->HeightInPixels();
        const TInt blockTop = aRow.iTl.iY + (aRow.Height() - blockH) / 2;
        nameBaseline = blockTop + nameFont->AscentInPixels();
        subBaseline = blockTop + nameH + KLineGap + subFont->AscentInPixels();
        }
    else
        {
        nameBaseline = aRow.iTl.iY
            + (aRow.Height() + nameFont->AscentInPixels() - nameFont->DescentInPixels()) / 2;
        }

    TBuf<KMaxDrawnText> text(item.iName);
    AknTextUtils::ClipToFit(text, *nameFont, textWidth);   // "..." when too long
    aGc.UseFont(nameFont);
    aGc.SetPenColor(highlighted ? KHiNameColor() : KNameColor());
    aGc.DrawText(text, TPoint(textLeft, nameBaseline));
    aGc.DiscardFont();

    if (item.iSubtitle.Length() > 0)
        {
        text = item.iSubtitle;
        AknTextUtils::ClipToFit(text, *subFont, textWidth);
        aGc.UseFont(subFont);
        aGc.SetPenColor(highlighted ? KHiSubtitleColor() : KSubtitleColor());
        aGc.DrawText(text, TPoint(textLeft, subBaseline));
        aGc.DiscardFont();
        }
    }

void CWunderlandAppList::DrawIcon(CWindowGc& aGc, const TItem& aItem, const TRect& aRect) const
    {
    // Placeholder until the store serves icons: a square in a colour derived
    // from the name, with the name's first letter.
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

// Stars at their native size (like on the web store), centred vertically.
void CWunderlandAppList::DrawStars(CWindowGc& aGc, TInt aRating, const TRect& aRow, TInt aRight) const
    {
    const TSize size(iStarOn->SizeInPixels());
    const TRect source(size);
    const TInt top = aRow.iTl.iY + (aRow.Height() - size.iHeight) / 2;
    TInt x = aRight - StarsWidth();
    for (TInt s = 0; s < KMaxStars; ++s)
        {
        const TBool on = (s < aRating);
        aGc.BitBltMasked(TPoint(x, top), on ? iStarOn : iStarOff, source,
                         on ? iStarOnMask : iStarOffMask, EFalse);
        x += size.iWidth + KStarGap;
        }
    }

void CWunderlandAppList::DrawScrollBar(CWindowGc& aGc) const
    {
    const TRect rect(Rect());
    const TRect track(rect.iBr.iX - KScrollBarWidth, rect.iTl.iY, rect.iBr.iX, rect.iBr.iY);
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
