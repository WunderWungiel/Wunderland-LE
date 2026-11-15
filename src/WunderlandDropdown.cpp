#include <aknutils.h>
#include <eikon.hrh>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"
#include "WunderlandDropdown.h"

using WunderlandDraw::FillGradient;

namespace
    {
    const TInt KItemGranularity = 16;
    const TInt KBorder = 1;
    const TInt KRowVPad = 5;           // text padding above/below; rows double as touch targets
    const TInt KTextHPad = 8;
    const TInt KScrollBarWidth = 5;
    const TInt KMinThumbHeight = 8;
    const TInt KMaxDrawnText = 128;

    const TRgb KBackgroundColor()  { return KRgbWhite; }
    const TRgb KBorderColor()      { return TRgb(0x3A, 0x3A, 0x3A); }
    const TRgb KSeparatorColor()   { return TRgb(0xDC, 0xDC, 0xDC); }
    const TRgb KTextColor()        { return KRgbBlack; }
    const TRgb KHiTopColor()       { return TRgb(0x3E, 0x8A, 0xD0); }   // same blue as the bars
    const TRgb KHiBottomColor()    { return TRgb(0x1C, 0x5E, 0x9E); }
    const TRgb KHiTextColor()      { return KRgbWhite; }
    const TRgb KTrackColor()       { return TRgb(0xE8, 0xE8, 0xE8); }
    const TRgb KThumbColor()       { return TRgb(0x8A, 0x8A, 0x8A); }
    }

CWunderlandDropdown* CWunderlandDropdown::NewL(const CCoeControl& aParent)
    {
    CWunderlandDropdown* self = new (ELeave) CWunderlandDropdown;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandDropdown::CWunderlandDropdown()
    : iDownItem(-1)
    {
    }

void CWunderlandDropdown::ConstructL(const CCoeControl& aParent)
    {
    CreateWindowL(&aParent);   // child window: drawn above the parent's content
    EnableDragEvents();        // touch scrolling
    iItems = new (ELeave) CDesCArrayFlat(KItemGranularity);
    MakeVisible(EFalse);
    }

CWunderlandDropdown::~CWunderlandDropdown()
    {
    delete iItems;
    }

void CWunderlandDropdown::SetItemsL(const MDesCArray& aItems)
    {
    iItems->Reset();
    const TInt count = aItems.MdcaCount();
    for (TInt i = 0; i < count; ++i)
        {
        iItems->AppendL(aItems.MdcaPoint(i));
        }
    iHighlighted = 0;
    iTop = 0;
    }

void CWunderlandDropdown::Open(const TRect& aMaxRect, TInt aHighlighted)
    {
    const TInt count = Count();
    if (count == 0)
        {
        return;
        }
    TInt rows = (aMaxRect.Height() - 2 * KBorder) / RowHeight();
    if (rows < 1)
        {
        rows = 1;
        }
    else if (rows > count)
        {
        rows = count;
        }
    SetRect(TRect(aMaxRect.iTl, TSize(aMaxRect.Width(), rows * RowHeight() + 2 * KBorder)));

    iHighlighted = (aHighlighted >= 0 && aHighlighted < count) ? aHighlighted : 0;
    iTop = 0;
    EnsureHighlightVisible();
    iDownItem = -1;
    iDragged = EFalse;

    DrawableWindow()->SetOrdinalPosition(0);   // above any sibling windows
    MakeVisible(ETrue);
    DrawDeferred();
    }

void CWunderlandDropdown::Close()
    {
    MakeVisible(EFalse);
    }

TBool CWunderlandDropdown::IsOpen() const
    {
    return IsVisible();
    }

TInt CWunderlandDropdown::Highlighted() const
    {
    return iHighlighted;
    }

void CWunderlandDropdown::ChooseL()
    {
    if (IsOpen() && Count() > 0)
        {
        ReportEventL(MCoeControlObserver::EEventStateChanged);
        }
    }

// ---------------------------------------------------------------------------
// Geometry (own window coordinates: the window's top-left is 0,0)

const CFont* CWunderlandDropdown::Font() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);   // smaller, regular weight
    }

TInt CWunderlandDropdown::Count() const
    {
    return iItems->MdcaCount();
    }

TInt CWunderlandDropdown::RowHeight() const
    {
    return Font()->HeightInPixels() + 2 * KRowVPad;
    }

TInt CWunderlandDropdown::VisibleRows() const
    {
    const TInt rows = InnerRect().Height() / RowHeight();
    return rows > 0 ? rows : 1;
    }

TBool CWunderlandDropdown::HasScrollBar() const
    {
    return Count() > VisibleRows();
    }

TRect CWunderlandDropdown::InnerRect() const
    {
    TRect rect(Size());
    rect.Shrink(KBorder, KBorder);
    return rect;
    }

TRect CWunderlandDropdown::RowRect(TInt aRow) const
    {
    const TRect inner(InnerRect());
    const TInt rowH = RowHeight();
    const TInt right = HasScrollBar() ? inner.iBr.iX - KScrollBarWidth : inner.iBr.iX;
    const TInt top = inner.iTl.iY + aRow * rowH;
    return TRect(inner.iTl.iX, top, right, top + rowH);
    }

TInt CWunderlandDropdown::ItemAt(const TPoint& aPos) const
    {
    const TRect inner(InnerRect());
    if (!inner.Contains(aPos))
        {
        return -1;
        }
    const TInt item = iTop + (aPos.iY - inner.iTl.iY) / RowHeight();
    return item < Count() ? item : -1;
    }

void CWunderlandDropdown::SetTop(TInt aTop)
    {
    TInt maxTop = Count() - VisibleRows();
    if (maxTop < 0)
        {
        maxTop = 0;
        }
    if (aTop < 0)
        {
        aTop = 0;
        }
    else if (aTop > maxTop)
        {
        aTop = maxTop;
        }
    iTop = aTop;
    }

void CWunderlandDropdown::EnsureHighlightVisible()
    {
    const TInt rows = VisibleRows();
    if (iHighlighted < iTop)
        {
        SetTop(iHighlighted);
        }
    else if (iHighlighted >= iTop + rows)
        {
        SetTop(iHighlighted - rows + 1);
        }
    }

void CWunderlandDropdown::MoveHighlight(TInt aDelta)
    {
    const TInt count = Count();
    if (count == 0)
        {
        return;
        }
    iHighlighted = (iHighlighted + aDelta + count) % count;
    EnsureHighlightVisible();
    DrawDeferred();
    }

// ---------------------------------------------------------------------------
// Input

TKeyResponse CWunderlandDropdown::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    // Modal while open: swallow everything so the tabs don't move underneath.
    // Softkeys never get here; the CBA turns them into commands.
    if (aType == EEventKey)
        {
        switch (aKeyEvent.iCode)
            {
            case EKeyUpArrow:
                MoveHighlight(-1);
                break;
            case EKeyDownArrow:
                MoveHighlight(1);
                break;
            case EKeyOK:
            case EKeyEnter:
                ChooseL();
                break;
            default:
                break;
            }
        }
    return EKeyWasConsumed;
    }

void CWunderlandDropdown::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
    // Only delivered on touch devices (S60 5th Edition). Tap picks a row,
    // dragging scrolls the list a row at a time.
    const TPoint pos(aPointerEvent.iPosition);
    switch (aPointerEvent.iType)
        {
        case TPointerEvent::EButton1Down:
            iDownItem = ItemAt(pos);
            iDragStartY = pos.iY;
            iDragStartTop = iTop;
            iDragged = EFalse;
            if (iDownItem >= 0)
                {
                iHighlighted = iDownItem;
                DrawDeferred();
                }
            break;
        case TPointerEvent::EDrag:
            {
            const TInt rowH = RowHeight();
            const TInt dy = iDragStartY - pos.iY;
            if (!iDragged && Abs(dy) >= rowH / 2)
                {
                iDragged = ETrue;
                }
            if (iDragged)
                {
                const TInt oldTop = iTop;
                SetTop(iDragStartTop + dy / rowH);
                if (iTop != oldTop)
                    {
                    DrawDeferred();
                    }
                }
            break;
            }
        case TPointerEvent::EButton1Up:
            if (!iDragged && iDownItem >= 0 && ItemAt(pos) == iDownItem)
                {
                iHighlighted = iDownItem;
                ChooseL();
                }
            iDownItem = -1;
            break;
        default:
            break;
        }
    }

// ---------------------------------------------------------------------------
// Drawing

void CWunderlandDropdown::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Size());

    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KBorderColor());
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KBackgroundColor());
    gc.DrawRect(rect);

    const CFont* font = Font();
    const TInt count = Count();
    const TInt rows = VisibleRows();
    gc.UseFont(font);
    for (TInt row = 0; row < rows && iTop + row < count; ++row)
        {
        const TInt item = iTop + row;
        const TRect rowRect(RowRect(row));
        const TBool highlighted = (item == iHighlighted);
        if (highlighted)
            {
            FillGradient(gc, rowRect, KHiTopColor(), KHiBottomColor());
            }
        else if (row < rows - 1 && item < count - 1)
            {
            gc.SetPenStyle(CGraphicsContext::ESolidPen);
            gc.SetPenColor(KSeparatorColor());
            gc.DrawLine(TPoint(rowRect.iTl.iX + KTextHPad, rowRect.iBr.iY - 1),
                        TPoint(rowRect.iBr.iX - KTextHPad, rowRect.iBr.iY - 1));
            }

        TRect textRect(rowRect);
        textRect.Shrink(KTextHPad, 0);
        TBuf<KMaxDrawnText> text(iItems->MdcaPoint(item).Left(KMaxDrawnText));
        AknTextUtils::ClipToFit(text, *font, textRect.Width());
        gc.SetBrushStyle(CGraphicsContext::ENullBrush);
        gc.SetPenStyle(CGraphicsContext::ESolidPen);
        gc.SetPenColor(highlighted ? KHiTextColor() : KTextColor());
        const TInt baseline = (textRect.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
        gc.DrawText(text, textRect, baseline, CGraphicsContext::ELeft);
        }
    gc.DiscardFont();

    if (HasScrollBar())
        {
        const TRect inner(InnerRect());
        const TRect track(inner.iBr.iX - KScrollBarWidth, inner.iTl.iY, inner.iBr.iX, inner.iBr.iY);
        TInt thumbH = track.Height() * rows / count;
        if (thumbH < KMinThumbHeight)
            {
            thumbH = KMinThumbHeight;
            }
        const TInt maxTop = count - rows;
        const TInt thumbY = track.iTl.iY + (track.Height() - thumbH) * iTop / (maxTop > 0 ? maxTop : 1);
        gc.SetPenStyle(CGraphicsContext::ENullPen);
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(KTrackColor());
        gc.DrawRect(track);
        gc.SetBrushColor(KThumbColor());
        gc.DrawRect(TRect(track.iTl.iX + 1, thumbY, track.iBr.iX - 1, thumbY + thumbH));
        }
    }
