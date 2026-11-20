#include <aknutils.h>
#include <badesca.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandContentTypePage.h"
#include "WunderlandDrawUtils.h"

using WunderlandDraw::FillGradient;
using WunderlandDraw::FillRoundGradient;

namespace
    {
    const TInt KItemGranularity = 16;
    const TInt KMargin = 6;            // page edges to the rows, header text inset
    const TInt KHeaderVPad = 8;        // above/below the header text
    const TInt KGroupGap = 10;         // above and below "See all"
    const TInt KRowVPad = 14;          // above/below the row text: thick rows
    const TInt KMinRowHeight = 44;     // rows double as touch targets
    const TInt KRowTextPad = 10;       // name and "»" inset inside the row
    const TInt KCornerRadius = 4;      // the "See all" box; the block is square
    const TInt KScrollBarWidth = 4;    // drawn inside the right margin
    const TInt KMinThumbHeight = 8;
    const TInt KDragThreshold = 8;     // pixels before a tap becomes a drag
    const TInt KMaxDrawnText = 64;

    _LIT(KSeeAll, "See all ");
    _LIT(KChevron, "\x00BB");          // »

    const TRgb KBackgroundColor()    { return KRgbWhite; }
    const TRgb KHeaderTopColor()     { return TRgb(0xF2, 0xF3, 0xF4); }
    const TRgb KHeaderBottomColor()  { return TRgb(0xD6, 0xD9, 0xDC); }
    const TRgb KHeaderShadeColor()   { return TRgb(0xB4, 0xB8, 0xBE); }   // bottom edge
    const TRgb KHeaderTextColor()    { return TRgb(0x3A, 0x3D, 0x40); }
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
    delete iItems;
    }

void CWunderlandContentTypePage::SetContentL(const TDesC& aTitle, const MDesCArray& aCategories)
    {
    iItems->Reset();
    iHighlighted = -1;
    iScrollY = 0;
    iDragged = EFalse;
    iTitle = aTitle.Left(iTitle.MaxLength());

    TBuf<KMaxDrawnText> seeAll(KSeeAll);
    TBuf<32> type(iTitle);
    type.LowerCase();
    seeAll.Append(type.Left(seeAll.MaxLength() - seeAll.Length()));
    iItems->AppendL(seeAll);
    const TInt count = aCategories.MdcaCount();
    for (TInt i = 0; i < count; ++i)
        {
        iItems->AppendL(aCategories.MdcaPoint(i));
        }
    DrawDeferred();
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

const CFont* CWunderlandContentTypePage::HeaderFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimaryFont);
    }

const CFont* CWunderlandContentTypePage::RowFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);
    }

TRect CWunderlandContentTypePage::HeaderRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl, TSize(rect.Width(), KHeaderVPad + HeaderFont()->HeightInPixels() + KHeaderVPad));
    }

TRect CWunderlandContentTypePage::ListRect() const
    {
    const TRect rect(Rect());
    return TRect(rect.iTl.iX, HeaderRect().iBr.iY, rect.iBr.iX, rect.iBr.iY);
    }

TInt CWunderlandContentTypePage::RowHeight() const
    {
    const TInt h = RowFont()->HeightInPixels() + 2 * KRowVPad;
    return h > KMinRowHeight ? h : KMinRowHeight;
    }

// Row 0 stands alone; rows 1.. touch, sharing their 1px border lines.
TInt CWunderlandContentTypePage::RowTop(TInt aIndex) const
    {
    if (aIndex == 0)
        {
        return KGroupGap;
        }
    return KGroupGap + RowHeight() + KGroupGap + (aIndex - 1) * RowHeight();
    }

TInt CWunderlandContentTypePage::ContentHeight() const
    {
    const TInt count = Count();
    if (count == 0)
        {
        return 0;
        }
    return RowTop(count - 1) + RowHeight() + 1 + KMargin;   // +1: the block's bottom line
    }

TBool CWunderlandContentTypePage::HasScrollBar() const
    {
    return ContentHeight() > ListRect().Height();
    }

TInt CWunderlandContentTypePage::ItemAt(const TPoint& aPos) const
    {
    const TRect list(ListRect());
    if (!list.Contains(aPos) || aPos.iX < list.iTl.iX + KMargin || aPos.iX >= list.iBr.iX - KMargin)
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

    const TInt left = list.iTl.iX + KMargin;
    const TInt right = list.iBr.iX - KMargin;
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

    if (HasScrollBar())
        {
        DrawScrollBar(gc);
        }
    gc.CancelClippingRect();

    DrawHeader(gc);
    }

void CWunderlandContentTypePage::DrawHeader(CWindowGc& aGc) const
    {
    const TRect header(HeaderRect());
    FillGradient(aGc, TRect(header.iTl, TPoint(header.iBr.iX, header.iBr.iY - 1)),
                 KHeaderTopColor(), KHeaderBottomColor());
    aGc.SetPenColor(KHeaderShadeColor());
    aGc.DrawLine(TPoint(header.iTl.iX, header.iBr.iY - 1), TPoint(header.iBr.iX, header.iBr.iY - 1));

    const CFont* font = HeaderFont();
    TBuf<32> text(iTitle);
    const TInt width = header.Width() - 2 * (KMargin + KRowTextPad);
    if (width <= 0)
        {
        return;
        }
    AknTextUtils::ClipToFit(text, *font, width);
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KHeaderTextColor());
    // Left-aligned with the row text below
    aGc.DrawText(text, TPoint(header.iTl.iX + KMargin + KRowTextPad,
                              header.iTl.iY + KHeaderVPad + font->AscentInPixels()));
    aGc.DiscardFont();
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
    const TInt chevronX = aRow.iBr.iX - KRowTextPad - chevronW;
    aGc.DrawText(KChevron, TPoint(chevronX, baseline));

    const TInt textLeft = aRow.iTl.iX + KRowTextPad;
    const TInt textWidth = chevronX - KRowTextPad - textLeft;
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
