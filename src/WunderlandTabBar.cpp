#include <akniconutils.h>
#include <aknutils.h>
#include <avkon.hrh>
#include <fbs.h>
#include <gdi.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"
#include "WunderlandTabBar.h"
#include <wunderlandle_0xe4f76ead_tabs.mbg>

using WunderlandDraw::FillGradient;

namespace
    {
    const TInt KMinHeight = 32;
    const TInt KHeightDivisor = 6;   // bar height = shorter screen side / 6 (40 px on 240 px)
    const TInt KSeparatorInset = 5;  // separators stop short of the bar's top edge
    const TInt KIconPercent = 58;    // icon size as a share of the tab height

    _LIT(KTabsFile, "\\resource\\apps\\WunderlandLE_0xE4F76EAD_tabs.mif");
    // The MIF holds 4 states per tab (normal, active, focused, focused + active),
    // each a bitmap followed by its mask; see Wunderland_icons.mk.
    const TInt KFirstIconId = EMbmWunderlandle_0xe4f76ead_tabsTab_home;

    // Bar: white at the top fading to a slightly bluish light grey
    const TRgb KBarTopColor()        { return TRgb(0xFB, 0xFC, 0xFD); }
    const TRgb KBarBottomColor()     { return TRgb(0xC8, 0xCF, 0xD6); }
    const TRgb KBarShadeColor()      { return TRgb(0x9E, 0xA5, 0xAD); }   // bottom edge
    const TRgb KSeparatorColor()     { return TRgb(0xAE, 0xB5, 0xBD); }
    const TRgb KSeparatorHiColor()   { return KRgbWhite; }
    // Active tab: lighter, pressed in (inner shadow along the top)
    const TRgb KActiveTopColor()     { return TRgb(0xF2, 0xF4, 0xF6); }
    const TRgb KActiveBottomColor()  { return TRgb(0xE4, 0xE8, 0xEC); }
    const TRgb KActiveShadow1Color() { return TRgb(0xB8, 0xBE, 0xC5); }
    const TRgb KActiveShadow2Color() { return TRgb(0xD6, 0xDA, 0xDF); }
    // Focused tab: dark grey gradient
    const TRgb KFocusTopColor()      { return TRgb(0x6C, 0x71, 0x78); }
    const TRgb KFocusBottomColor()   { return TRgb(0x3E, 0x42, 0x48); }

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

    TFileName file(KTabsFile);
    User::LeaveIfError(CompleteWithAppPath(file));   // drive the app is installed on
    for (TInt tab = 0; tab < ETabCount; ++tab)
        {
        for (TInt state = 0; state < EIconStateCount; ++state)
            {
            const TInt id = KFirstIconId + (tab * EIconStateCount + state) * 2;
            AknIconUtils::CreateIconL(iIcons[tab][state], iIconMasks[tab][state], file, id, id + 1);
            }
        }
    }

CWunderlandTabBar::~CWunderlandTabBar()
    {
    for (TInt tab = 0; tab < ETabCount; ++tab)
        {
        for (TInt state = 0; state < EIconStateCount; ++state)
            {
            delete iIcons[tab][state];
            delete iIconMasks[tab][state];
            }
        }
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

TSize CWunderlandTabBar::IconSize() const
    {
    TInt size = (Rect().Height() - 1) * KIconPercent / 100;
    const TInt maxSize = Rect().Width() / ETabCount - 4;
    if (size > maxSize)
        {
        size = maxSize;
        }
    return TSize(size, size);
    }

void CWunderlandTabBar::SizeChanged()
    {
    // SVG icons are rendered at the size set here (rotation / resolution change)
    const TSize size(IconSize());
    if (size.iWidth <= 0)
        {
        return;
        }
    for (TInt tab = 0; tab < ETabCount; ++tab)
        {
        for (TInt state = 0; state < EIconStateCount; ++state)
            {
            AknIconUtils::SetSize(iIcons[tab][state], size, EAspectRatioNotPreserved);
            }
        }
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

TBool CWunderlandTabBar::SwitchTabL(TInt aDelta)
    {
    const TInt target = iActive + aDelta;
    if (target < 0 || target >= ETabCount || target == iActive)
        {
        return EFalse;   // already the first / last tab
        }
    iActive = target;
    iFocused = target;
    DrawDeferred();
    ReportEventL(MCoeControlObserver::EEventStateChanged);
    return ETrue;
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

void CWunderlandTabBar::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());

    FillGradient(gc, rect, KBarTopColor(), KBarBottomColor());
    gc.SetPenColor(KBarShadeColor());
    gc.DrawLine(TPoint(rect.iTl.iX, rect.iBr.iY - 1), TPoint(rect.iBr.iX, rect.iBr.iY - 1));

    const TSize iconSize(IconSize());
    for (TInt i = 0; i < ETabCount; ++i)
        {
        const TRect tab(TabRect(i));
        const TBool focused = iBarFocused && i == iFocused;
        const TBool active = (i == iActive);
        if (focused)
            {
            FillGradient(gc, tab, KFocusTopColor(), KFocusBottomColor());
            }
        else if (active)
            {
            // Pressed in: lighter face with a soft shadow along the top
            FillGradient(gc, tab, KActiveTopColor(), KActiveBottomColor());
            gc.SetPenColor(KActiveShadow1Color());
            gc.DrawLine(tab.iTl, TPoint(tab.iBr.iX, tab.iTl.iY));
            gc.SetPenColor(KActiveShadow2Color());
            gc.DrawLine(TPoint(tab.iTl.iX, tab.iTl.iY + 1), TPoint(tab.iBr.iX, tab.iTl.iY + 1));
            }

        // Engraved separator on the left edge, except next to the dark tab
        const TBool nextToFocus = iBarFocused && (i == iFocused || i - 1 == iFocused);
        if (i > 0 && !nextToFocus)
            {
            const TInt top = active || i - 1 == iActive ? tab.iTl.iY : tab.iTl.iY + KSeparatorInset;
            gc.SetPenColor(KSeparatorColor());
            gc.DrawLine(TPoint(tab.iTl.iX, top), TPoint(tab.iTl.iX, tab.iBr.iY));
            gc.SetPenColor(KSeparatorHiColor());
            gc.DrawLine(TPoint(tab.iTl.iX + 1, top), TPoint(tab.iTl.iX + 1, tab.iBr.iY));
            }

        TInt state = EIconNormal;
        if (focused)
            {
            state = active ? EIconFocusedActive : EIconFocused;
            }
        else if (active)
            {
            state = EIconActive;
            }
        const TPoint c(tab.Center());
        const TPoint pos(c.iX - iconSize.iWidth / 2, c.iY - iconSize.iHeight / 2);
        gc.BitBltMasked(pos, iIcons[i][state], TRect(iconSize), iIconMasks[i][state], EFalse);
        }
    }
