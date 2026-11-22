#include <aknutils.h>
#include <badesca.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandContentTypePage.h"
#include "WunderlandDrawUtils.h"

using WunderlandDraw::FillRoundGradient;
using WunderlandDraw::Px;

namespace
    {
    // Pixel sizes for a 360 px short screen side; used through Px().
    const TInt KItemGranularity = 16;
    const TInt KMargin = 10;           // page edges to the rows, header text inset
    const TInt KGroupGap = 14;         // above and below "See all"
    const TInt KRowVPad = 22;          // above/below the row text: thick rows
    const TInt KMinRowHeight = 60;     // rows double as touch targets
    const TInt KRowTextPad = 14;       // name and "»" inset inside the row
    const TInt KCornerRadius = 4;      // the "See all" box; the block is square
    const TInt KScrollBarWidth = 4;    // drawn inside the right margin
    const TInt KMinThumbHeight = 8;
    const TInt KDragThreshold = 8;     // pixels before a tap becomes a drag
    const TInt KMaxDrawnText = 64;

    _LIT(KSeeAll, "See all ");
    _LIT(KChevron, "\x00BB");          // »

    const TRgb KBackgroundColor()    { return KRgbWhite; }
    // Rows: the same look as on the Categories page
    const TRgb KRowTopColor()        { return KRgbWhite; }
    const TRgb KRowBottomColor()     { return TRgb(0xEA, 0xEC, 0xEF); }
    const TRgb KRowBorderColor()     { return TRgb(0xC4, 0xC9, 0xCE); }
    const TRgb KRowTextColor()       { return TRgb(0x3A, 0x3D, 0x40); }
    const TRgb KHiTopColor()         { return TRgb(0x6C, 0x71, 0x78); }
    const TRgb KHiBottomColor()      { return TRgb(0x3E, 0x42, 0x48); }
    const TRgb KHiBorderColor()      { return TRgb(0x33, 0x36, 0x3A); }
    const TRgb KHiTextColor()        { return KRgbWhite; }
    const TRgb KTrackColor()         { return TRgb(0xE8, 0xE8, 0xE8); }
    const TRgb KThumbColor()         { return TRgb(0x8A, 0x8A, 0x8A); }
    }

CWunderlandContentTypePage* CWunderlandContentTypePage::NewL(const CCoeControl& aParent)
    {
    CWunderlandContentTypePage* self = new (ELeave) CWunderlandContentTypePage;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandContentTypePage::CWunderlandContentTypePage()
    : iHighlighted(-1)
    {
    }

void CWunderlandContentTypePage::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iItems = new (ELeave) CDesC16ArrayFlat(KItemGranularity);
    }

CWunderlandContentTypePage::~CWunderlandContentTypePage()
    {
    delete iStatus;
    delete iItems;
    }

void CWunderlandContentTypePage::SetContentL(const TDesC& aTitle)
    {
    iItems->Reset();
    iHighlighted = -1;
    iScrollY = 0;
    iDragged = EFalse;
    delete iStatus;
    iStatus = NULL;
    iRetry = EFalse;
    iTitle = aTitle.Left(iTitle.MaxLength());

    TBuf<KMaxDrawnText> seeAll(KSeeAll);
    TBuf<32> type(iTitle);
    type.LowerCase();
    seeAll.Append(type.Left(seeAll.MaxLength() - seeAll.Length()));
    iItems->AppendL(seeAll);
    DrawDeferred();
    }

// Builds the new rows first, so a leave keeps the old ones.
void CWunderlandContentTypePage::SetCategoriesL(const MDesCArray& aCategories)
    {
    const TInt count = aCategories.MdcaCount();
    CDesC16ArrayFlat* items = new (ELeave) CDesC16ArrayFlat(KItemGranularity);
    CleanupStack::PushL(items);
    items->AppendL(iItems->MdcaCount() > 0 ? iItems->MdcaPoint(0) : KNullDesC());   // "See all"
    for (TInt i = 0; i < count; ++i)
        {
        items->AppendL(aCategories.MdcaPoint(i));
        }
    CleanupStack::Pop(items);
    delete iItems;
    iItems = items;
    if (iHighlighted >= Count())
        {
        iHighlighted = -1;
        }
    SetScroll(iScrollY);   // re-clamp
    DrawDeferred();
    }

void CWunderlandContentTypePage::SetStatusL(const TDesC& aText, TBool aRetry)
    {
    HBufC* status = aText.Length() > 0 ? aText.AllocL() : NULL;
    delete iStatus;
    iStatus = status;
    iRetry = status && aRetry;
    SetScroll(iScrollY);   // the content got shorter or longer
    DrawDeferred();
    }

TBool CWunderlandContentTypePage::CanRetry() const
    {
    return iRetry;
    }

void CWunderlandContentTypePage::SetRetryObserver(MWunderlandRetryObserver* aObserver)
    {
    iRetryObserver = aObserver;
    }

TInt CWunderlandContentTypePage::Count() const
    {
    return iItems->MdcaCount();
    }

TInt CWunderlandContentTypePage::Highlighted() const
    {
    return iHighlighted;
    }

void CWunderlandContentTypePage::SetHighlighted(TInt aIndex)
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

TBool CWunderlandContentTypePage::MoveHighlight(TInt aDelta)
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
// Geometry

const CFont* CWunderlandContentTypePage::RowFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);
    }

TRect CWunderlandContentTypePage::HeaderRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl, TSize(rect.Width(), WunderlandDraw::PageHeaderHeight()));
    }

TRect CWunderlandContentTypePage::ListRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl.iX, HeaderRect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    }

TInt CWunderlandContentTypePage::RowHeight() const
    {
    const TInt h = RowFont()->HeightInPixels() + 2 * Px(KRowVPad);
    return h > Px(KMinRowHeight) ? h : Px(KMinRowHeight);
    }

// Row 0 stands alone; rows 1.. touch, sharing their 1px border lines.
TInt CWunderlandContentTypePage::RowTop(TInt aIndex) const
    {
    if (aIndex == 0)
        {
        return Px(KGroupGap);
        }
    return Px(KGroupGap) + RowHeight() + Px(KGroupGap) + (aIndex - 1) * RowHeight();
    }

TInt CWunderlandContentTypePage::ContentHeight() const
    {
    const TInt count = Count();
    if (count == 0)
        {
        return 0;
        }
    if (iStatus)
        {
        return StatusTop() + RowHeight() + Px(KMargin);
        }
    return RowTop(count - 1) + RowHeight() + 1 + Px(KMargin);   // +1: the block's bottom line
    }

// Right under the last row, where the next row would go (with the gap under
// "See all" when that is the only row).
TInt CWunderlandContentTypePage::StatusTop() const
    {
    const TInt count = Count();
    return count <= 1 ? RowTop(1) : RowTop(count) + 1;
    }

TBool CWunderlandContentTypePage::IsStatusAt(const TPoint& aPos) const
    {
    const TRect list(ListRect());
    if (!iStatus || !list.Contains(aPos))
        {
        return EFalse;
        }
    const TInt y = aPos.iY - list.iTl.iY + iScrollY;
    return y >= StatusTop() && y < StatusTop() + RowHeight();
    }

TBool CWunderlandContentTypePage::HasScrollBar() const
    {
    return ContentHeight() > ListRect().Height();
    }

TInt CWunderlandContentTypePage::ItemAt(const TPoint& aPos) const
    {
    const TRect list(ListRect());
    if (!list.Contains(aPos) || aPos.iX < list.iTl.iX + Px(KMargin) || aPos.iX >= list.iBr.iX - Px(KMargin))
        {
        return -1;
        }
    const TInt y = aPos.iY - list.iTl.iY + iScrollY;
    const TInt rowH = RowHeight();
    if (y >= RowTop(0) && y < RowTop(0) + rowH)
        {
        return 0;
        }
    const TInt blockTop = RowTop(1);
    if (y < blockTop)
        {
        return -1;   // margin or the gap under "See all"
        }
    const TInt item = 1 + (y - blockTop) / rowH;
    return item < Count() ? item : -1;
    }

void CWunderlandContentTypePage::SetScroll(TInt aScrollY)
    {
    TInt maxScroll = ContentHeight() - ListRect().Height();
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

void CWunderlandContentTypePage::EnsureVisible(TInt aIndex)
    {
    // The first row brings the top margin along, the last row the bottom one
    const TInt top = aIndex == 0 ? 0 : RowTop(aIndex);
    const TInt bottom = aIndex == Count() - 1 ? ContentHeight() : RowTop(aIndex) + RowHeight() + 1;
    const TInt view = ListRect().Height();
    if (top < iScrollY)
        {
        SetScroll(top);
        }
    else if (bottom > iScrollY + view)
        {
        SetScroll(bottom - view);
        }
    }

void CWunderlandContentTypePage::SizeChanged()
    {
    SetScroll(iScrollY);   // re-clamp after rotation / layout change
    if (iHighlighted >= 0)
        {
        EnsureVisible(iHighlighted);
        }
    }

// ---------------------------------------------------------------------------
// Input

void CWunderlandContentTypePage::HandlePointerEventL(const TPointerEvent& aPointerEvent)
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
                else if (iRetry && iRetryObserver && IsStatusAt(pos))
                    {
                    iRetryObserver->HandleRetryL(*this);
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

void CWunderlandContentTypePage::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect list(ListRect());
    gc.SetClippingRect(list);

    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KBackgroundColor());
    gc.DrawRect(list);

    const TInt left = list.iTl.iX + Px(KMargin);
    const TInt right = list.iBr.iX - Px(KMargin);
    const TInt listTop = list.iTl.iY - iScrollY;
    const TInt rowH = RowHeight();
    const TInt count = Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TInt top = listTop + RowTop(i);
        if (top >= list.iBr.iY)
            {
            break;
            }
        // The highlighted row goes last so its dark border isn't covered by
        // the next row's top line.
        if (i != iHighlighted && top + rowH + 1 > list.iTl.iY)
            {
            DrawRow(gc, i, TRect(left, top, right, top + rowH + (i == 0 ? 0 : 1)),
                    i == 0 ? KCornerRadius : 0);
            }
        }
    if (iHighlighted >= 0)
        {
        const TInt top = listTop + RowTop(iHighlighted);
        DrawRow(gc, iHighlighted, TRect(left, top, right, top + rowH + (iHighlighted == 0 ? 0 : 1)),
                iHighlighted == 0 ? KCornerRadius : 0);
        }
    if (iStatus)
        {
        const TInt top = listTop + StatusTop();
        WunderlandDraw::DrawStatusText(gc, TRect(left, top, right, top + rowH), *iStatus);
        }

    if (HasScrollBar())
        {
        DrawScrollBar(gc);
        }
    gc.CancelClippingRect();

    WunderlandDraw::DrawPageHeader(gc, HeaderRect(), iTitle);
    }

void CWunderlandContentTypePage::DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow,
                                         TInt aRadius) const
    {
    const TBool highlighted = (aIndex == iHighlighted);

    // 1px border: the outer shape in the border colour, the gradient inside it
    const TRgb border(highlighted ? KHiBorderColor() : KRowBorderColor());
    FillRoundGradient(aGc, aRow, border, border, aRadius);
    TRect inner(aRow);
    inner.Shrink(1, 1);
    FillRoundGradient(aGc, inner,
                      highlighted ? KHiTopColor() : KRowTopColor(),
                      highlighted ? KHiBottomColor() : KRowBottomColor(),
                      aRadius > 0 ? aRadius - 1 : 0);

    const CFont* font = RowFont();
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(highlighted ? KHiTextColor() : KRowTextColor());
    const TInt baseline = aRow.iTl.iY
        + (aRow.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;

    // "»" stuck to the right, the name in the space left of it
    const TInt chevronW = font->TextWidthInPixels(KChevron);
    const TInt chevronX = aRow.iBr.iX - Px(KRowTextPad) - chevronW;
    aGc.DrawText(KChevron, TPoint(chevronX, baseline));

    const TInt textLeft = aRow.iTl.iX + Px(KRowTextPad);
    const TInt textWidth = chevronX - Px(KRowTextPad) - textLeft;
    if (textWidth > 0)
        {
        TBuf<KMaxDrawnText> text(iItems->MdcaPoint(aIndex).Left(KMaxDrawnText));
        AknTextUtils::ClipToFit(text, *font, textWidth);   // "..." when too long
        aGc.DrawText(text, TPoint(textLeft, baseline));
        }
    aGc.DiscardFont();
    }

void CWunderlandContentTypePage::DrawScrollBar(CWindowGc& aGc) const
    {
    const TRect list(ListRect());
    const TRect track(list.iBr.iX - KScrollBarWidth, list.iTl.iY, list.iBr.iX, list.iBr.iY);
    const TInt content = ContentHeight();
    const TInt view = list.Height();
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
