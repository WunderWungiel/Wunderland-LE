#ifndef __WUNDERLANDSEARCHPAGE_H__
#define __WUNDERLANDSEARCHPAGE_H__

#include <coecntrl.h>
#include <e32base.h>
#include "WunderlandPage.h"

class CFont;
class CWindowGc;

// Search page: a dark grey header with a white search field (placeholder
// text for now, not editable yet) and a blue "Go" button of the same height
// to its right; white below, with a hint text centred in that space. Has no rows, so the d-pad focus never enters it
// (Up goes to the tab bar, Left/Right switch tabs).
// Non-window-owning: lives inside the parent control's window.
class CWunderlandSearchPage : public CCoeControl, public MWunderlandPage
    {
public:
    static CWunderlandSearchPage* NewL(const CCoeControl& aParent);
    ~CWunderlandSearchPage();

public: // from MWunderlandPage
    TInt Count() const;
    TInt Highlighted() const;
    void SetHighlighted(TInt aIndex);
    TBool MoveHighlight(TInt aDelta);

private: // from CCoeControl
    void Draw(const TRect& aRect) const;
    void SizeChanged();

private:
    CWunderlandSearchPage();
    void ConstructL(const CCoeControl& aParent);

    const CFont* FieldFont() const;
    const CFont* ButtonFont() const;
    TInt FieldHeight() const;
    TRect HeaderRect() const;
    TRect GoRect() const;
    TRect FieldRect() const;
    void DrawField(CWindowGc& aGc) const;
    void DrawGoButton(CWindowGc& aGc) const;
    const CFont* HintFont() const;
    TInt HintLineWidth() const;
    void WrapHintL();
    void DrawHint(CWindowGc& aGc) const;

private:
    CArrayFixFlat<TPtrC>* iHintLines;   // owned; the hint wrapped to the page width
    };

#endif // __WUNDERLANDSEARCHPAGE_H__
