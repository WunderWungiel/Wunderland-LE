#ifndef __WUNDERLANDSEARCHBAR_H__
#define __WUNDERLANDSEARCHBAR_H__

#include <coecntrl.h>
#include <gdi.h>

const TInt KWunderlandMaxQuery = 64;   // characters in a search query

class CEikEdwin;
class CFont;
class CWunderlandSearchBar;

// Told when the user starts a search (OK in the field, Go pressed or tapped).
class MWunderlandSearchObserver
    {
public:
    virtual void HandleSearchL(const TDesC& aQuery) = 0;
    };

// The owner of a search bar (a page) hears about taps and searches through this.
class MWunderlandSearchBarObserver
    {
public:
    virtual void HandleSearchBarTappedL(CWunderlandSearchBar& aBar) = 0;   // focus moved into the bar
    virtual void HandleSearchRequestL(CWunderlandSearchBar& aBar) = 0;
    };

// Dark grey bar with a white text field (a single-line CEikEdwin) and a blue
// "Go" button of the same height to its right. Shared by the Search tab and
// the search results page.
// D-pad: the bar has one focus spot at a time, the field or Go. In the field
// Left/Right move the cursor; Right at the end of the text goes to Go, Left
// on Go comes back. OK in either starts a search. Keys the bar doesn't use
// (Up/Down, Left at the start of the field, Right on Go) are not consumed,
// so the owner can move the focus elsewhere.
// While the field is empty and not focused, the editor is hidden and a grey
// placeholder is drawn instead.
// Non-window-owning: lives inside the parent control's window.
class CWunderlandSearchBar : public CCoeControl
    {
public:
    enum TFocus
        {
        EFocusNone,
        EFocusField,
        EFocusGo
        };

    static CWunderlandSearchBar* NewL(const CCoeControl& aParent, MWunderlandSearchBarObserver& aObserver);
    ~CWunderlandSearchBar();

    TInt PreferredHeight() const;   // depends on the layout's fonts

    TFocus FocusSpot() const;
    void SetFocusSpot(TFocus aFocus);   // redraws if changed

    void GetQuery(TDes& aQuery) const;
    void SetQueryL(const TDesC& aQuery);   // cursor at the end

public: // from CCoeControl
    void ActivateL();
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;

private:
    CWunderlandSearchBar(MWunderlandSearchBarObserver& aObserver);
    void ConstructL(const CCoeControl& aParent);

    const CFont* FieldFont() const;
    const CFont* ButtonFont() const;
    TInt FieldHeight() const;
    TRect GoRect() const;
    TRect FieldRect() const;
    TRect EditorRect() const;
    void UpdateEditorFontL();
    void UpdateEditorVisibility();
    void CursorToEndL();
    void DrawField(CWindowGc& aGc) const;
    void DrawGoButton(CWindowGc& aGc) const;

private:
    MWunderlandSearchBarObserver& iObserver;
    CEikEdwin* iEditor;   // owned
    TFontSpec iEditorFontSpec;  // the font the editor's format was made for
    TBool iEditorFontSet;       // iEditorFontSpec is valid
    TFocus iFocus;
    TBool iGoPressed;     // touch: finger down on Go
    };

#endif // __WUNDERLANDSEARCHBAR_H__
