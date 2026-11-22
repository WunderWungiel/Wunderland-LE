#include <aknutils.h>
#include <badesca.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandCategoryList.h"
#include "WunderlandDrawUtils.h"

using WunderlandDraw::FillRoundGradient;

namespace
    {
    const TInt KItemGranularity = 8;
    const TInt KMargin = 10;           // page edges to the rows and the heading
    const TInt KHeadingVPad = 8;       // above/below the heading text
    const TInt KRowGap = 8;            // between rows
    const TInt KRowVPad = 22;          // above/below the row text: thick rows
    const TInt KMinRowHeight = 60;     // rows double as touch targets
    const TInt KRowTextPad = 14;       // name and "»" inset inside the row
    const TInt KCornerRadius = 4;
    const TInt KScrollBarWidth = 4;    // drawn inside the right margin
    const TInt KMinThumbHeight = 8;
    const TInt KDragThreshold = 8;     // pixels before a tap becomes a drag
    const TInt KMaxDrawnText = 64;

    _LIT(KHeading, "Categories");
    _LIT(KChevron, "\x00BB");          // »

    const TRgb KBackgroundColor()    { return KRgbWhite; }
    const TRgb KHeadingColor()       { return TRgb(0x6A, 0x6E, 0x73); }
    const TRgb KRowTopColor()        { return KRgbWhite; }
    const TRgb KRowBottomColor()     { return TRgb(0xEA, 0xEC, 0xEF); }
    const TRgb KRowBorderColor()     { return TRgb(0xC4, 0xC9, 0xCE); }
    const TRgb KRowTextColor()       { return TRgb(0x3A, 0x3D, 0x40); }
    // Highlight: the focused tab's dark grey gradient
    const TRgb KHiTopColor()         { return TRgb(0x6C, 0x71, 0x78); }
    const TRgb KHiBottomColor()      { return TRgb(0x3E, 0x42, 0x48); }
    const TRgb KHiBorderColor()      { return TRgb(0x33, 0x36, 0x3A); }
    const TRgb KHiTextColor()        { return KRgbWhite; }
    const TRgb KTrackColor()         { return TRgb(0xE8, 0xE8, 0xE8); }
    const TRgb KThumbColor()         { return TRgb(0x8A, 0x8A, 0x8A); }
    }

CWunderlandCategoryList* CWunderlandCategoryList::NewL(const CCoeControl& aParent)
    {
    CWunderlandCategoryList* self = new (ELeave) CWunderlandCategoryList;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandCategoryList::CWunderlandCategoryList()
    : iHighlighted(-1)
    {
    }

void CWunderlandCategoryList::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iItems = new (ELeave) CDesC16ArrayFlat(KItemGranularity);
    }

CWunderlandCategoryList::~CWunderlandCategoryList()
    {
    delete iStatus;
    delete iItems;
    }

void CWunderlandCategoryList::Reset()
    {
    iItems->Reset();
    iHighlighted = -1;
    iScrollY = 0;
    iDragged = EFalse;
    DrawDeferred();
    }

void CWunderlandCategoryList::AppendL(const TDesC& aName)
    {
    iItems->AppendL(aName);
    DrawDeferred();
    }

void CWunderlandCategoryList::SetStatusL(const TDesC& aText, TBool aRetry)
    {
    HBufC* status = aText.Length() > 0 ? aText.AllocL() : NULL;
    delete iStatus;
    iStatus = status;
    iRetry = status && aRetry;
    SetScroll(iScrollY);   // the content got shorter or longer
    DrawDeferred();
    }

TBool CWunderlandCategoryList::CanRetry() const
    {
    return iRetry;
    }

void CWunderlandCategoryList::SetRetryObserver(MWunderlandRetryObserver* aObserver)
    {
    iRetryObserver = aObserver;
    }

TInt CWunderlandCategoryList::Count() const
    {
    return iItems->MdcaCount();
    }

TInt CWunderlandCategoryList::Highlighted() const
    {
    return iHighlighted;
    }

void CWunderlandCategoryList::SetHighlighted(TInt aIndex)
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

TBool CWunderlandCategoryList::MoveHighlight(TInt aDelta)
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

const CFont* CWunderlandCategoryList::HeadingFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontTitleFont);   // large bold
    }

const CFont* CWunderlandCategoryList::RowFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);
    }

TInt CWunderlandCategoryList::HeadingHeight() const
    {
    return KHeadingVPad + HeadingFont()->HeightInPixels() + KHeadingVPad;
    }

TInt CWunderlandCategoryList::RowHeight() const
    {
    const TInt h = RowFont()->HeightInPixels() + 2 * KRowVPad;
    return h > KMinRowHeight ? h : KMinRowHeight;
    }

TInt CWunderlandCategoryList::RowTop(TInt aIndex) const
    {
    return HeadingHeight() + aIndex * (RowHeight() + KRowGap);
    }

// The status line, if any, takes the place of one more row.
TInt CWunderlandCategoryList::ContentHeight() const
    {
    const TInt rows = Count() + (iStatus ? 1 : 0);
    return RowTop(rows) - KRowGap + KMargin;   // bottom margin after the last row
    }

TBool CWunderlandCategoryList::HasScrollBar() const
    {
    return ContentHeight() > Rect().Height();
    }

TInt CWunderlandCategoryList::ItemAt(const TPoint& aPos) const
    {
    const TRect rect(Rect());
    if (!rect.Contains(aPos) || aPos.iX < rect.iTl.iX + KMargin || aPos.iX >= rect.iBr.iX - KMargin)
        {
        return -1;
        }
    const TInt y = aPos.iY - rect.iTl.iY + iScrollY - HeadingHeight();
    if (y < 0)
        {
        return -1;
        }
    const TInt step = RowHeight() + KRowGap;
    const TInt item = y / step;
    if (item >= Count() || y % step >= RowHeight())   // in the gap between rows
        {
        return -1;
        }
    return item;
    }

TBool CWunderlandCategoryList::IsStatusAt(const TPoint& aPos) const
    {
    if (!iStatus || !Rect().Contains(aPos))
        {
        return EFalse;
        }
    const TInt y = aPos.iY - Rect().iTl.iY + iScrollY;
    return y >= RowTop(Count()) && y < RowTop(Count()) + RowHeight();
    }

void CWunderlandCategoryList::SetScroll(TInt aScrollY)
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

void CWunderlandCategoryList::EnsureVisible(TInt aIndex)
    {
    // The first row brings the heading along, the last row its bottom margin
    const TInt top = aIndex == 0 ? 0 : RowTop(aIndex) - KRowGap;
    const TInt bottom = aIndex == Count() - 1 ? ContentHeight() : RowTop(aIndex) + RowHeight() + KRowGap;
    if (top < iScrollY)
        {
        SetScroll(top);
        }
    else if (bottom > iScrollY + Rect().Height())
        {
        SetScroll(bottom - Rect().Height());
        }
    }

void CWunderlandCategoryList::SizeChanged()
    {
    SetScroll(iScrollY);   // re-clamp after rotation / layout change
    if (iHighlighted >= 0)
        {
        EnsureVisible(iHighlighted);
        }
    }

// ---------------------------------------------------------------------------
// Input

void CWunderlandCategoryList::HandlePointerEventL(const TPointerEvent& aPointerEvent)
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

void CWunderlandCategoryList::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    gc.SetClippingRect(rect);

    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KBackgroundColor());
    gc.DrawRect(rect);

    const TInt left = rect.iTl.iX + KMargin;
    const TInt right = rect.iBr.iX - KMargin;
    const TInt pageTop = rect.iTl.iY - iScrollY;

    // Heading, left-aligned with the rows
    const CFont* headingFont = HeadingFont();
    gc.UseFont(headingFont);
    gc.SetBrushStyle(CGraphicsContext::ENullBrush);
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KHeadingColor());
    gc.DrawText(KHeading, TPoint(left, pageTop + KHeadingVPad + headingFont->AscentInPixels()));
    gc.DiscardFont();

    const TInt rowH = RowHeight();
    const TInt count = Count();
    for (TInt i = 0; i < count; ++i)
        {
        const TInt top = pageTop + RowTop(i);
        if (top >= rect.iBr.iY)
            {
            break;
            }
        if (top + rowH > rect.iTl.iY)
            {
            DrawRow(gc, i, TRect(left, top, right, top + rowH));
            }
        }
    if (iStatus)
        {
        const TInt top = pageTop + RowTop(count);
        WunderlandDraw::DrawStatusText(gc, TRect(left, top, right, top + rowH), *iStatus);
        }

    if (HasScrollBar())
        {
        DrawScrollBar(gc);
        }
    gc.CancelClippingRect();
    }

void CWunderlandCategoryList::DrawRow(CWindowGc& aGc, TInt aIndex, const TRect& aRow) const
    {
    const TBool highlighted = (aIndex == iHighlighted);

    // 1px border: the outer shape in the border colour, the gradient inside it
    const TRgb border(highlighted ? KHiBorderColor() : KRowBorderColor());
    FillRoundGradient(aGc, aRow, border, border, KCornerRadius);
    TRect inner(aRow);
    inner.Shrink(1, 1);
    FillRoundGradient(aGc, inner,
                      highlighted ? KHiTopColor() : KRowTopColor(),
                      highlighted ? KHiBottomColor() : KRowBottomColor(),
                      KCornerRadius - 1);

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

void CWunderlandCategoryList::DrawScrollBar(CWindowGc& aGc) const
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
