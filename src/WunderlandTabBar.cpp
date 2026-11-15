#include <aknutils.h>
#include <avkon.hrh>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"
#include "WunderlandTabBar.h"

using WunderlandDraw::FillGradient;

namespace
    {
    const TInt KLabelGranularity = 4;
    const TInt KTabHPad   = 10;  // text padding inside a tab
    const TInt KTabGap    = 2;   // space between tabs
    const TInt KTabVInset = 3;   // frame inset from the bar's top/bottom edge
    const TInt KTabVPad   = 3;   // text padding above/below inside the frame

    // Bar: glossy two-segment vertical gradient around #2878C2.
    const TRgb KBarTopColor()      { return TRgb(0x6C, 0xAC, 0xE6); }
    const TRgb KBarMidHiColor()    { return TRgb(0x44, 0x8E, 0xD2); }   // end of the gloss
    const TRgb KBarMidLoColor()    { return TRgb(0x28, 0x78, 0xC2); }   // #2878C2
    const TRgb KBarBottomColor()   { return TRgb(0x1E, 0x66, 0xAA); }
    const TRgb KBarHighlightColor(){ return TRgb(0xA4, 0xCE, 0xF4); }   // top edge
    const TRgb KBarShadeColor()    { return TRgb(0x10, 0x46, 0x7C); }   // bottom edge
    // Every tab sits in a square frame with a faint whitish border; the
    // selected one has a dark gradient fill and a brighter border.
    const TRgb KSelTopColor()      { return TRgb(0x1C, 0x5E, 0x9E); }
    const TRgb KSelBottomColor()   { return TRgb(0x0A, 0x34, 0x62); }
    const TRgb KSelTextColor()     { return KRgbWhite; }
    const TRgb KSelBorderColor()   { return TRgb(0xC8, 0xE0, 0xF6); }
    const TRgb KTabBorderColor()   { return TRgb(0x8E, 0xBE, 0xEA); }
    const TRgb KTabTextColor()     { return TRgb(0x08, 0x24, 0x44); }   // unselected: dark on bar
    const TRgb KArrowColor()       { return KRgbWhite; }
    const TRgb KArrowDimColor()    { return TRgb(0x8C, 0xBC, 0xE8); }
    const TRgb KArrowShadowColor() { return TRgb(0x10, 0x46, 0x7C); }
    }

CWunderlandTabBar* CWunderlandTabBar::NewL(const CCoeControl& aParent)
    {
    CWunderlandTabBar* self = new (ELeave) CWunderlandTabBar;
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandTabBar::CWunderlandTabBar()
    {
    }

void CWunderlandTabBar::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    iLabels = new (ELeave) CDesCArrayFlat(KLabelGranularity);
    }

CWunderlandTabBar::~CWunderlandTabBar()
    {
    delete iLabels;
    }

void CWunderlandTabBar::AppendTabL(const TDesC& aLabel)
    {
    iLabels->AppendL(aLabel);
    UpdateScroll();
    }

TInt CWunderlandTabBar::Count() const
    {
    return iLabels->MdcaCount();
    }

TInt CWunderlandTabBar::Selected() const
    {
    return iSelected;
    }

TPtrC CWunderlandTabBar::SelectedLabel() const
    {
    return Count() > 0 ? iLabels->MdcaPoint(iSelected) : TPtrC();
    }

void CWunderlandTabBar::SetSelected(TInt aIndex)
    {
    const TInt count = Count();
    if (count == 0)
        {
        return;
        }
    if (aIndex < 0)
        {
        aIndex = 0;
        }
    else if (aIndex >= count)
        {
        aIndex = count - 1;
        }
    iSelected = aIndex;
    UpdateScroll();
    DrawDeferred();
    }

void CWunderlandTabBar::MoveSelectionL(TInt aDelta)
    {
    const TInt old = iSelected;
    SetSelected(iSelected + aDelta);
    if (iSelected != old)
        {
        ReportEventL(MCoeControlObserver::EEventStateChanged);
        }
    }

// ---------------------------------------------------------------------------
// Geometry

const CFont* CWunderlandTabBar::Font() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);   // small bold, like Ovi Store
    }

TInt CWunderlandTabBar::PreferredHeight() const
    {
    return Font()->HeightInPixels() + 2 * (KTabVInset + KTabVPad);
    }

TInt CWunderlandTabBar::ArrowWidth() const
    {
    return Rect().Height() * 2 / 3;
    }

TRect CWunderlandTabBar::LeftArrowRect() const
    {
    const TRect r(Rect());
    return TRect(r.iTl, TSize(ArrowWidth(), r.Height()));
    }

TRect CWunderlandTabBar::RightArrowRect() const
    {
    const TRect r(Rect());
    return TRect(TPoint(r.iBr.iX - ArrowWidth(), r.iTl.iY), r.iBr);
    }

TRect CWunderlandTabBar::Viewport() const
    {
    return TRect(LeftArrowRect().iBr.iX, Rect().iTl.iY, RightArrowRect().iTl.iX, Rect().iBr.iY);
    }

TInt CWunderlandTabBar::TabWidth(TInt aIndex) const
    {
    return Font()->TextWidthInPixels(iLabels->MdcaPoint(aIndex)) + 2 * KTabHPad;
    }

TInt CWunderlandTabBar::TotalTabsWidth() const
    {
    TInt total = 0;
    const TInt count = Count();
    for (TInt i = 0; i < count; ++i)
        {
        total += TabWidth(i);
        }
    if (count > 1)
        {
        total += (count - 1) * KTabGap;
        }
    return total;
    }

TRect CWunderlandTabBar::TabRect(TInt aIndex) const
    {
    const TRect view(Viewport());
    TInt x = view.iTl.iX - iScrollX;
    for (TInt i = 0; i < aIndex; ++i)
        {
        x += TabWidth(i) + KTabGap;
        }
    return TRect(x, view.iTl.iY + KTabVInset, x + TabWidth(aIndex), view.iBr.iY - KTabVInset);
    }

void CWunderlandTabBar::UpdateScroll()
    {
    const TInt viewWidth = Viewport().Width();
    const TInt total = TotalTabsWidth();
    if (Count() == 0 || viewWidth <= 0 || total <= viewWidth)
        {
        iScrollX = 0;   // everything fits: left-aligned, no scrolling
        return;
        }

    // Offset of the selected tab's centre from the start of the strip.
    TInt centre = 0;
    for (TInt i = 0; i < iSelected; ++i)
        {
        centre += TabWidth(i) + KTabGap;
        }
    centre += TabWidth(iSelected) / 2;

    TInt scroll = centre - viewWidth / 2;
    const TInt maxScroll = total - viewWidth;
    if (scroll < 0)
        {
        scroll = 0;
        }
    else if (scroll > maxScroll)
        {
        scroll = maxScroll;
        }
    iScrollX = scroll;
    }

void CWunderlandTabBar::SizeChanged()
    {
    UpdateScroll();   // rotation / resolution change
    }

// ---------------------------------------------------------------------------
// Input

TKeyResponse CWunderlandTabBar::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (aType != EEventKey)
        {
        return EKeyWasNotConsumed;
        }
    switch (aKeyEvent.iCode)
        {
        case EKeyLeftArrow:
            MoveSelectionL(-1);
            return EKeyWasConsumed;
        case EKeyRightArrow:
            MoveSelectionL(1);
            return EKeyWasConsumed;
        default:
            return EKeyWasNotConsumed;
        }
    }

void CWunderlandTabBar::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
    // Only delivered on touch devices (S60 5th Edition); harmless elsewhere.
    if (aPointerEvent.iType != TPointerEvent::EButton1Down)
        {
        return;
        }
    const TPoint pos(aPointerEvent.iPosition);
    if (LeftArrowRect().Contains(pos))
        {
        MoveSelectionL(-1);
        return;
        }
    if (RightArrowRect().Contains(pos))
        {
        MoveSelectionL(1);
        return;
        }
    const TInt count = Count();
    for (TInt i = 0; i < count; ++i)
        {
        if (TabRect(i).Contains(pos))
            {
            MoveSelectionL(i - iSelected);
            return;
            }
        }
    }

// ---------------------------------------------------------------------------
// Drawing

void CWunderlandTabBar::DrawArrow(CWindowGc& aGc, const TRect& aRect, TBool aLeft, TBool aEnabled) const
    {
    const TInt h = aRect.Height() / 4;            // half the triangle's height
    const TInt w = h;                             // triangle depth
    const TPoint c(aRect.Center());
    TPoint pts[3];
    if (aLeft)
        {
        pts[0] = TPoint(c.iX - w / 2, c.iY);
        pts[1] = TPoint(c.iX + w / 2, c.iY - h);
        pts[2] = TPoint(c.iX + w / 2, c.iY + h);
        }
    else
        {
        pts[0] = TPoint(c.iX + w / 2, c.iY);
        pts[1] = TPoint(c.iX - w / 2, c.iY - h);
        pts[2] = TPoint(c.iX - w / 2, c.iY + h);
        }
    // 1 px drop shadow so the white arrow stands out on the light gloss
    WunderlandDraw::FillTriangle(aGc, pts, aEnabled ? KArrowColor() : KArrowDimColor(),
                                 aEnabled ? KArrowShadowColor() : KArrowDimColor());
    }

void CWunderlandTabBar::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());

    // Background: glossy gradient, light upper half, base colour lower half
    const TInt split = rect.iTl.iY + rect.Height() / 2;
    FillGradient(gc, TRect(rect.iTl, TPoint(rect.iBr.iX, split)), KBarTopColor(), KBarMidHiColor());
    FillGradient(gc, TRect(TPoint(rect.iTl.iX, split), rect.iBr), KBarMidLoColor(), KBarBottomColor());
    gc.SetPenColor(KBarHighlightColor());
    gc.DrawLine(rect.iTl, TPoint(rect.iBr.iX, rect.iTl.iY));
    gc.SetPenColor(KBarShadeColor());
    gc.DrawLine(TPoint(rect.iTl.iX, rect.iBr.iY - 1), TPoint(rect.iBr.iX, rect.iBr.iY - 1));

    const TInt count = Count();
    DrawArrow(gc, LeftArrowRect(), ETrue, iSelected > 0);
    DrawArrow(gc, RightArrowRect(), EFalse, iSelected < count - 1);

    if (count == 0)
        {
        return;
        }

    // Tabs, clipped to the area between the arrows. Each tab is a square
    // frame; the selected one is filled dark with white text.
    const TRect view(Viewport());
    gc.SetClippingRect(view);
    const CFont* font = Font();
    gc.UseFont(font);
    for (TInt i = 0; i < count; ++i)
        {
        const TRect tab(TabRect(i));
        if (tab.iBr.iX <= view.iTl.iX || tab.iTl.iX >= view.iBr.iX)
            {
            continue;   // fully scrolled out
            }
        const TBool selected = (i == iSelected);
        if (selected)
            {
            FillGradient(gc, tab, KSelTopColor(), KSelBottomColor());
            }

        gc.SetBrushStyle(CGraphicsContext::ENullBrush);   // outline only; DrawText(box) must not clear
        gc.SetPenStyle(CGraphicsContext::ESolidPen);
        gc.SetPenColor(selected ? KSelBorderColor() : KTabBorderColor());
        gc.DrawRect(tab);
        gc.SetPenColor(selected ? KSelTextColor() : KTabTextColor());
        const TInt baseline = (tab.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
        gc.DrawText(iLabels->MdcaPoint(i), tab, baseline, CGraphicsContext::ECenter);
        }
    gc.DiscardFont();
    gc.CancelClippingRect();
    }
