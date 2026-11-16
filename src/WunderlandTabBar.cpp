#include <avkon.hrh>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"
#include "WunderlandTabBar.h"

using WunderlandDraw::FillGradient;

namespace
    {
    const TInt KMinHeight = 32;
    const TInt KHeightDivisor = 6;   // bar height = shorter screen side / 6 (40 px on 240 px)
    const TInt KSeparatorInset = 6;  // separators stop short of the bar's top edge

    // Bar: white at the top fading to light grey
    const TRgb KBarTopColor()        { return KRgbWhite; }
    const TRgb KBarBottomColor()     { return TRgb(0xD2, 0xD6, 0xDB); }
    const TRgb KBarShadeColor()      { return TRgb(0xA6, 0xAA, 0xB0); }   // bottom edge
    const TRgb KSeparatorColor()     { return TRgb(0xB4, 0xB8, 0xBE); }
    const TRgb KSeparatorHiColor()   { return KRgbWhite; }
    // Focused tab: dark grey gradient
    const TRgb KFocusTopColor()      { return TRgb(0x6C, 0x71, 0x78); }
    const TRgb KFocusBottomColor()   { return TRgb(0x3E, 0x42, 0x48); }
    // Icons
    const TRgb KIconColor()          { return TRgb(0x4C, 0x51, 0x58); }
    const TRgb KIconActiveColor()    { return TRgb(0x1C, 0x64, 0xC8); }
    const TRgb KIconFocusColor()     { return TRgb(0xF0, 0xF0, 0xF0); }
    const TRgb KIconFocusActiveColor(){ return TRgb(0x8C, 0xC0, 0xFF); }

    _LIT(KLabelHome, "Home");
    _LIT(KLabelCategories, "Categories");
    _LIT(KLabelSearch, "Search");
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
    : iActive(ETabHome), iFocused(ETabHome)
    {
    }

void CWunderlandTabBar::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);
    }

CWunderlandTabBar::~CWunderlandTabBar()
    {
    }

TInt CWunderlandTabBar::Active() const
    {
    return iActive;
    }

TInt CWunderlandTabBar::Focused() const
    {
    return iFocused;
    }

TBool CWunderlandTabBar::IsBarFocused() const
    {
    return iBarFocused;
    }

void CWunderlandTabBar::SetBarFocused(TBool aFocused)
    {
    if (!aFocused != !iBarFocused)
        {
        iBarFocused = aFocused;
        if (iBarFocused)
            {
            iFocused = iActive;   // browsing starts from the current page
            }
        DrawDeferred();
        }
    }

TPtrC CWunderlandTabBar::FocusedLabel() const
    {
    switch (iFocused)
        {
        case ETabCategories:
            return TPtrC(KLabelCategories);
        case ETabSearch:
            return TPtrC(KLabelSearch);
        default:
            return TPtrC(KLabelHome);
        }
    }

TInt CWunderlandTabBar::PreferredHeight(const TSize& aArea) const
    {
    const TInt side = aArea.iWidth < aArea.iHeight ? aArea.iWidth : aArea.iHeight;
    const TInt height = side / KHeightDivisor;
    return height > KMinHeight ? height : KMinHeight;
    }

TRect CWunderlandTabBar::TabRect(TInt aIndex) const
    {
    const TRect r(Rect());
    const TInt left = r.iTl.iX + r.Width() * aIndex / ETabCount;
    const TInt right = r.iTl.iX + r.Width() * (aIndex + 1) / ETabCount;
    return TRect(left, r.iTl.iY, right, r.iBr.iY - 1);   // above the bottom edge line
    }

// ---------------------------------------------------------------------------
// Input

void CWunderlandTabBar::MoveFocusL(TInt aDelta)
    {
    TInt target = iFocused + aDelta;
    if (target < 0)
        {
        target = 0;
        }
    else if (target >= ETabCount)
        {
        target = ETabCount - 1;
        }
    if (target != iFocused)
        {
        iFocused = target;
        DrawDeferred();
        ReportEventL(MCoeControlObserver::EEventStateChanged);
        }
    }

void CWunderlandTabBar::ActivateFocusedL()
    {
    if (iActive != iFocused)
        {
        iActive = iFocused;
        DrawDeferred();
        ReportEventL(MCoeControlObserver::EEventStateChanged);
        }
    }

TKeyResponse CWunderlandTabBar::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (aType != EEventKey || !iBarFocused)
        {
        return EKeyWasNotConsumed;
        }
    switch (aKeyEvent.iCode)
        {
        case EKeyLeftArrow:
            MoveFocusL(-1);
            return EKeyWasConsumed;
        case EKeyRightArrow:
            MoveFocusL(1);
            return EKeyWasConsumed;
        case EKeyOK:
        case EKeyEnter:
            ActivateFocusedL();
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
    for (TInt i = 0; i < ETabCount; ++i)
        {
        if (TabRect(i).Contains(aPointerEvent.iPosition))
            {
            const TBool changed = !iBarFocused || iFocused != i || iActive != i;
            iBarFocused = ETrue;
            iFocused = i;
            iActive = i;
            if (changed)
                {
                DrawDeferred();
                ReportEventL(MCoeControlObserver::EEventStateChanged);
                }
            return;
            }
        }
    }

// ---------------------------------------------------------------------------
// Drawing

// Icons are drawn as shapes so they scale with the bar on every resolution.
void CWunderlandTabBar::DrawIcon(CWindowGc& aGc, TInt aTab, const TRect& aBox, const TRgb& aColour) const
    {
    const TInt s = aBox.Width();
    const TInt cx = aBox.iTl.iX + s / 2;
    const TInt top = aBox.iTl.iY;
    const TInt bottom = aBox.iBr.iY;
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(aColour);
    aGc.SetBrushColor(aColour);

    switch (aTab)
        {
        case ETabHome:
            {
            // Roof and walls as one polygon with the door cut out of the bottom
            const TInt eaves = top + s * 9 / 20;
            const TInt halfRoof = s / 2;
            const TInt halfWall = s * 7 / 20;
            const TInt halfDoor = s * 2 / 20 + 1;
            const TInt doorTop = bottom - s * 6 / 20;
            const TPoint pts[] =
                {
                TPoint(cx, top),
                TPoint(cx + halfRoof, eaves),
                TPoint(cx + halfWall, eaves),
                TPoint(cx + halfWall, bottom),
                TPoint(cx + halfDoor, bottom),
                TPoint(cx + halfDoor, doorTop),
                TPoint(cx - halfDoor, doorTop),
                TPoint(cx - halfDoor, bottom),
                TPoint(cx - halfWall, bottom),
                TPoint(cx - halfWall, eaves),
                TPoint(cx - halfRoof, eaves)
                };
            aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
            aGc.DrawPolygon(pts, sizeof(pts) / sizeof(pts[0]));
            break;
            }
        case ETabCategories:
            {
            // Three stacked bars
            const TInt bar = s / 5 > 2 ? s / 5 : 2;
            const TInt halfWidth = s * 9 / 20;
            const TSize corner(bar / 2, bar / 2);
            aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
            for (TInt i = 0; i < 3; ++i)
                {
                const TInt y = top + (s - bar) * i / 2;
                aGc.DrawRoundRect(TRect(cx - halfWidth, y, cx + halfWidth, y + bar), corner);
                }
            break;
            }
        case ETabSearch:
            {
            // Magnifying glass: ring in the upper left, handle to the lower right
            const TInt pen = s / 9 > 2 ? s / 9 : 2;
            const TInt d = s * 13 / 20;
            const TRect ring(TPoint(aBox.iTl.iX + pen / 2, top + pen / 2), TSize(d, d));
            aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
            aGc.SetPenSize(TSize(pen, pen));
            aGc.DrawEllipse(ring);
            const TInt handle = pen * 3 / 2 + 1;
            aGc.SetPenSize(TSize(handle, handle));
            const TInt start = d * 85 / 100;   // just outside the ring on its diagonal
            aGc.DrawLine(TPoint(ring.iTl.iX + start, ring.iTl.iY + start),
                         TPoint(aBox.iBr.iX - handle / 2, bottom - handle / 2));
            aGc.SetPenSize(TSize(1, 1));
            break;
            }
        default:
            break;
        }
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    }

void CWunderlandTabBar::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());

    FillGradient(gc, rect, KBarTopColor(), KBarBottomColor());
    gc.SetPenColor(KBarShadeColor());
    gc.DrawLine(TPoint(rect.iTl.iX, rect.iBr.iY - 1), TPoint(rect.iBr.iX, rect.iBr.iY - 1));

    for (TInt i = 0; i < ETabCount; ++i)
        {
        const TRect tab(TabRect(i));
        const TBool focused = iBarFocused && i == iFocused;
        if (focused)
            {
            FillGradient(gc, tab, KFocusTopColor(), KFocusBottomColor());
            }
        else if (i > 0 && !(iBarFocused && i - 1 == iFocused))
            {
            // Engraved separator on the left edge (not next to the dark tab)
            gc.SetPenColor(KSeparatorColor());
            gc.DrawLine(TPoint(tab.iTl.iX, tab.iTl.iY + KSeparatorInset), TPoint(tab.iTl.iX, tab.iBr.iY));
            gc.SetPenColor(KSeparatorHiColor());
            gc.DrawLine(TPoint(tab.iTl.iX + 1, tab.iTl.iY + KSeparatorInset), TPoint(tab.iTl.iX + 1, tab.iBr.iY));
            }

        TInt size = tab.Height() * 11 / 20;
        if (size > tab.Width() - 4)
            {
            size = tab.Width() - 4;
            }
        const TPoint c(tab.Center());
        const TRect box(TPoint(c.iX - size / 2, c.iY - size / 2), TSize(size, size));
        const TBool active = (i == iActive);
        TRgb colour;
        if (focused)
            {
            colour = active ? KIconFocusActiveColor() : KIconFocusColor();
            }
        else
            {
            colour = active ? KIconActiveColor() : KIconColor();
            }
        DrawIcon(gc, i, box, colour);
        }
    }
