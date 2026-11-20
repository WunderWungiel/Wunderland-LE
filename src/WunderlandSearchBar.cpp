#include <aknutils.h>
#include <eikedwin.h>
#include <eikon.hrh>
#include <gdi.h>
#include <txtfmlyr.h>
#include <txtfrmat.h>
#include <w32std.h>
#include "WunderlandDrawUtils.h"
#include "WunderlandSearchBar.h"

namespace
    {
    const TInt KHeaderMargin = 9;   // around the field and button, inside the bar
    const TInt KFieldGap = 8;       // between the field and the button
    const TInt KFieldVPad = 5;      // text to the field's top/bottom edge
    const TInt KFieldHPad = 7;      // text to the field's left edge
    const TInt KFocusFrame = 2;     // accent frame of the focused field / button
    const TInt KButtonHPad = 16;    // "Go" to the button's left/right edge

    _LIT(KPlaceholder, "Type here...");
    _LIT(KGo, "Go");

    const TRgb KHeaderColor()       { return TRgb(0x5E, 0x62, 0x68); }
    const TRgb KHeaderShadeColor()  { return TRgb(0x45, 0x48, 0x4D); }   // bottom edge
    const TRgb KFieldColor()        { return KRgbWhite; }
    const TRgb KFieldShadeColor()   { return TRgb(0xB4, 0xB8, 0xBE); }   // inner top edge
    const TRgb KTextColor()         { return KRgbBlack; }
    const TRgb KPlaceholderColor()  { return TRgb(0x9A, 0x9A, 0x9A); }
    // Store accent: the same blue as the active tab icon
    const TRgb KAccentColor()       { return TRgb(0x4A, 0x97, 0xF0); }
    const TRgb KButtonTopColor()    { return TRgb(0x4A, 0x97, 0xF0); }
    const TRgb KButtonBottomColor() { return TRgb(0x10, 0x56, 0xB4); }
    // Focused or pressed: darker, with a white frame
    const TRgb KButtonHiTopColor()    { return TRgb(0x1E, 0x6E, 0xD2); }
    const TRgb KButtonHiBottomColor() { return TRgb(0x08, 0x38, 0x7C); }
    const TRgb KButtonFrameColor()    { return KRgbWhite; }
    const TRgb KButtonTextColor()   { return KRgbWhite; }
    }

CWunderlandSearchBar* CWunderlandSearchBar::NewL(const CCoeControl& aParent,
                                                 MWunderlandSearchBarObserver& aObserver)
    {
    CWunderlandSearchBar* self = new (ELeave) CWunderlandSearchBar(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL(aParent);
    CleanupStack::Pop(self);
    return self;
    }

CWunderlandSearchBar::CWunderlandSearchBar(MWunderlandSearchBarObserver& aObserver)
    : iObserver(aObserver), iFocus(EFocusNone)
    {
    }

void CWunderlandSearchBar::ConstructL(const CCoeControl& aParent)
    {
    SetContainerWindowL(aParent);

    iEditor = new (ELeave) CEikEdwin;
    iEditor->SetContainerWindowL(*this);
    iEditor->ConstructL(CEikEdwin::ENoLineOrParaBreaks | CEikEdwin::ENoAutoSelection
                        | CEikEdwin::EJustAutoCurEnd | CEikEdwin::ENoWrap,
                        0, KWunderlandMaxQuery, 1);
    // Search terms: lower case first, like the S60 find boxes; the user can
    // still switch case and input mode.
    iEditor->SetAknEditorCase(EAknEditorLowerCase);
    iEditor->SetAknEditorPermittedCaseModes(EAknEditorAllCaseModes);
    // The rest of the editor setup needs its text view, which the editor
    // only creates when it is activated: see ActivateL().
    }

// Activates the editor first (that creates its text view), then gives it our
// look instead of the skin's: white field, black text in the field font.
void CWunderlandSearchBar::ActivateL()
    {
    CCoeControl::ActivateL();
    iEditor->SetSkinBackgroundControlContextL(NULL);
    iEditor->SetBackgroundColorL(KFieldColor());
    TMargins8 margins;
    margins.SetAllValuesTo(0);
    iEditor->SetBorderViewMargins(margins);   // the field's padding is ours
    iEditor->SetRect(EditorRect());           // again, with the margins gone
    UpdateEditorFontL();
    UpdateEditorVisibility();
    }

CWunderlandSearchBar::~CWunderlandSearchBar()
    {
    delete iEditor;
    }

// The edwin's text in the field font. The fonts follow the layout, so this
// is checked on every size change; the format is only replaced when the font
// actually changed. Compared by spec, not by pointer: Avkon layout fonts can
// be updated in place on a layout switch.
void CWunderlandSearchBar::UpdateEditorFontL()
    {
    const TFontSpec spec(FieldFont()->FontSpecInTwips());
    if (iEditorFontSet && spec == iEditorFontSpec)
        {
        return;
        }
    TCharFormat format;
    TCharFormatMask mask;
    format.iFontSpec = spec;
    format.iFontPresentation.iTextColor = TLogicalRgb(KTextColor());
    mask.SetAttrib(EAttFontTypeface);
    mask.SetAttrib(EAttFontHeight);
    mask.SetAttrib(EAttFontStrokeWeight);
    mask.SetAttrib(EAttColor);
    CCharFormatLayer* layer = CCharFormatLayer::NewL(format, mask);
    // Assumed to take ownership, as on other Symbian releases; the 9.1 SDK
    // doesn't document it. If it doesn't, this leaks one small layer per real
    // font change (a layout switch), which is the lesser risk.
    iEditor->SetCharFormatLayer(layer);
    iEditorFontSpec = spec;
    iEditorFontSet = ETrue;
    iEditor->NotifyNewFormatL();          // re-lay out the text in the new font
    }

TInt CWunderlandSearchBar::PreferredHeight() const
    {
    return FieldHeight() + 2 * KHeaderMargin;
    }

CWunderlandSearchBar::TFocus CWunderlandSearchBar::FocusSpot() const
    {
    return iFocus;
    }

void CWunderlandSearchBar::SetFocusSpot(TFocus aFocus)
    {
    if (aFocus == iFocus)
        {
        return;
        }
    iFocus = aFocus;
    UpdateEditorVisibility();   // before the focus: a focused editor must be shown
    // Cursor and input on/off. No EDrawNow: the editor draws straight into the
    // shared window, even when its page is no longer shown (it would flash
    // over the results page); DrawDeferred() below repaints it in place.
    iEditor->SetFocus(aFocus == EFocusField);
    DrawDeferred();
    }

void CWunderlandSearchBar::GetQuery(TDes& aQuery) const
    {
    aQuery.Zero();
    if (iEditor->TextLength() <= aQuery.MaxLength())
        {
        iEditor->GetText(aQuery);
        }
    }

void CWunderlandSearchBar::SetQueryL(const TDesC& aQuery)
    {
    const TPtrC text(aQuery.Left(KWunderlandMaxQuery));
    iEditor->SetTextL(&text);
    CursorToEndL();
    UpdateEditorVisibility();
    iEditor->DrawDeferred();
    DrawDeferred();
    }

void CWunderlandSearchBar::CursorToEndL()
    {
    if (iEditor->TextView())   // none until the editor is activated
        {
        iEditor->SetCursorPosL(iEditor->TextLength(), EFalse);
        }
    }

// The placeholder shows through only while the field is empty and idle.
void CWunderlandSearchBar::UpdateEditorVisibility()
    {
    const TBool visible = (iFocus == EFocusField || iEditor->TextLength() > 0);
    if (visible != iEditor->IsVisible())
        {
        iEditor->MakeVisible(visible);
        DrawDeferred();   // the placeholder goes or comes back
        }
    }

// ---------------------------------------------------------------------------
// Geometry

const CFont* CWunderlandSearchBar::FieldFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);
    }

const CFont* CWunderlandSearchBar::ButtonFont() const
    {
    return AknLayoutUtils::FontFromId(EAknLogicalFontPrimarySmallFont);   // small bold
    }

// Tall enough for either font; both depend on the layout (portrait,
// landscape, touch), so the bar grows with them.
TInt CWunderlandSearchBar::FieldHeight() const
    {
    TInt textH = FieldFont()->HeightInPixels();
    const TInt buttonH = ButtonFont()->HeightInPixels();
    if (buttonH > textH)
        {
        textH = buttonH;
        }
    return (textH + 2 * KFieldVPad) * 13 / 10;   // roomier than a plain text line
    }

TRect CWunderlandSearchBar::GoRect() const
    {
    const TRect rect(Rect());
    const TInt h = FieldHeight();
    TInt w = ButtonFont()->TextWidthInPixels(KGo) + 2 * KButtonHPad;
    if (w < h)
        {
        w = h;
        }
    w = w * 9 / 10;   // a bit narrower than that
    const TInt right = rect.iBr.iX - KHeaderMargin;
    const TInt top = rect.iTl.iY + KHeaderMargin;
    return TRect(right - w, top, right, top + h);
    }

TRect CWunderlandSearchBar::FieldRect() const
    {
    const TRect go(GoRect());
    return TRect(Rect().iTl.iX + KHeaderMargin, go.iTl.iY, go.iTl.iX - KFieldGap, go.iBr.iY);
    }

// Inside the field's padding and focus frame, one text line high, centred.
TRect CWunderlandSearchBar::EditorRect() const
    {
    const TRect field(FieldRect());
    const TInt left = field.iTl.iX + KFieldHPad;
    const TInt right = field.iBr.iX - KFocusFrame - 1;
    TInt h = FieldFont()->HeightInPixels() + FieldFont()->DescentInPixels();
    const TInt maxH = field.Height() - 2 * KFocusFrame;
    if (h > maxH)
        {
        h = maxH;
        }
    const TInt top = field.iTl.iY + (field.Height() - h) / 2;
    if (right <= left || h <= 0)
        {
        return TRect(field.Center(), TSize(0, 0));
        }
    return TRect(left, top, right, top + h);
    }

void CWunderlandSearchBar::SizeChanged()
    {
    if (!iEditor)
        {
        return;
        }
    iEditor->SetRect(EditorRect());
    if (IsActivated())   // before that the editor has no text view; ActivateL() does it
        {
        TRAP_IGNORE(UpdateEditorFontL());   // only leaves on OOM; the old font stays
        }
    }

TInt CWunderlandSearchBar::CountComponentControls() const
    {
    return iEditor ? 1 : 0;
    }

CCoeControl* CWunderlandSearchBar::ComponentControl(TInt aIndex) const
    {
    return aIndex == 0 ? iEditor : NULL;
    }

// ---------------------------------------------------------------------------
// Input

TKeyResponse CWunderlandSearchBar::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (iFocus == EFocusNone)
        {
        return EKeyWasNotConsumed;
        }
    if (aType != EEventKey)
        {
        return iFocus == EFocusField ? iEditor->OfferKeyEventL(aKeyEvent, aType) : EKeyWasNotConsumed;
        }
    switch (aKeyEvent.iCode)
        {
        case EKeyUpArrow:
        case EKeyDownArrow:
            return EKeyWasNotConsumed;   // the owner moves the focus
        case EKeyOK:
        case EKeyEnter:
            iObserver.HandleSearchRequestL(*this);
            return EKeyWasConsumed;
        case EKeyLeftArrow:
            if (iFocus == EFocusGo)
                {
                SetFocusSpot(EFocusField);
                CursorToEndL();
                return EKeyWasConsumed;
                }
            if (iEditor->CursorPos() == 0)
                {
                return EKeyWasNotConsumed;
                }
            break;
        case EKeyRightArrow:
            if (iFocus == EFocusGo)
                {
                return EKeyWasNotConsumed;
                }
            if (iEditor->CursorPos() >= iEditor->TextLength())
                {
                SetFocusSpot(EFocusGo);
                return EKeyWasConsumed;
                }
            break;
        default:
            break;
        }
    if (iFocus != EFocusField)
        {
        return EKeyWasNotConsumed;
        }
    const TKeyResponse response = iEditor->OfferKeyEventL(aKeyEvent, aType);
    UpdateEditorVisibility();
    return response;
    }

// Only delivered on touch devices (S60 5th Edition). A tap on the field
// focuses it and goes on to the editor (cursor, virtual keyboard); Go
// searches when the finger is lifted over it.
void CWunderlandSearchBar::HandlePointerEventL(const TPointerEvent& aPointerEvent)
    {
    const TPoint pos(aPointerEvent.iPosition);
    switch (aPointerEvent.iType)
        {
        case TPointerEvent::EButton1Down:
            iGoPressed = GoRect().Contains(pos);
            if (iGoPressed)
                {
                DrawDeferred();
                return;
                }
            if (FieldRect().Contains(pos) && iFocus != EFocusField)
                {
                SetFocusSpot(EFocusField);   // shows the editor, so it gets this tap too
                iObserver.HandleSearchBarTappedL(*this);
                }
            break;
        case TPointerEvent::EDrag:
            if (iGoPressed)
                {
                return;
                }
            break;
        case TPointerEvent::EButton1Up:
            if (iGoPressed)
                {
                iGoPressed = EFalse;
                DrawDeferred();
                if (GoRect().Contains(pos))
                    {
                    iObserver.HandleSearchRequestL(*this);
                    }
                return;
                }
            break;
        default:
            break;
        }
    CCoeControl::HandlePointerEventL(aPointerEvent);   // to the editor
    }

// ---------------------------------------------------------------------------
// Drawing

void CWunderlandSearchBar::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    const TRect rect(Rect());
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(KHeaderColor());
    gc.DrawRect(rect);
    gc.SetBrushColor(KHeaderShadeColor());
    gc.DrawRect(TRect(rect.iTl.iX, rect.iBr.iY - 1, rect.iBr.iX, rect.iBr.iY));

    DrawField(gc);
    DrawGoButton(gc);
    }

void CWunderlandSearchBar::DrawField(CWindowGc& aGc) const
    {
    const TRect field(FieldRect());
    if (field.Width() <= 2 * KFieldHPad)
        {
        return;
        }
    aGc.SetPenStyle(CGraphicsContext::ENullPen);
    aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    if (iFocus == EFocusField)
        {
        // Accent frame; the white field inside it
        aGc.SetBrushColor(KAccentColor());
        aGc.DrawRect(field);
        TRect inner(field);
        inner.Shrink(KFocusFrame, KFocusFrame);
        aGc.SetBrushColor(KFieldColor());
        aGc.DrawRect(inner);
        }
    else
        {
        aGc.SetBrushColor(KFieldColor());
        aGc.DrawRect(field);
        aGc.SetBrushColor(KFieldShadeColor());   // a little depth: the field sits in the bar
        aGc.DrawRect(TRect(field.iTl, TSize(field.Width(), 1)));
        }

    if (iEditor->IsVisible())
        {
        return;   // the editor draws the text
        }
    const CFont* font = FieldFont();
    TBuf<32> text(KPlaceholder);
    AknTextUtils::ClipToFit(text, *font, field.Width() - 2 * KFieldHPad);
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KPlaceholderColor());
    const TInt baseline = (field.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    aGc.DrawText(text, field, baseline, CGraphicsContext::ELeft, KFieldHPad);
    aGc.DiscardFont();
    }

void CWunderlandSearchBar::DrawGoButton(CWindowGc& aGc) const
    {
    const TRect go(GoRect());
    if (iFocus == EFocusGo || iGoPressed)
        {
        aGc.SetPenStyle(CGraphicsContext::ENullPen);
        aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        aGc.SetBrushColor(KButtonFrameColor());
        aGc.DrawRect(go);
        TRect inner(go);
        inner.Shrink(KFocusFrame, KFocusFrame);
        WunderlandDraw::FillGradient(aGc, inner, KButtonHiTopColor(), KButtonHiBottomColor());
        }
    else
        {
        WunderlandDraw::FillGradient(aGc, go, KButtonTopColor(), KButtonBottomColor());
        }

    const CFont* font = ButtonFont();
    aGc.UseFont(font);
    aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
    aGc.SetPenStyle(CGraphicsContext::ESolidPen);
    aGc.SetPenColor(KButtonTextColor());
    const TInt baseline = (go.Height() + font->AscentInPixels() - font->DescentInPixels()) / 2;
    aGc.DrawText(KGo, go, baseline, CGraphicsContext::ECenter);
    aGc.DiscardFont();
    }
