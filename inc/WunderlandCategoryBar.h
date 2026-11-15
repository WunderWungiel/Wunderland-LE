#ifndef __WUNDERLANDCATEGORYBAR_H__
#define __WUNDERLANDCATEGORYBAR_H__

#include <coecntrl.h>

class CFont;

// Grey gradient strip under the tab bar showing "<Content type> > <Category>"
// with a drop-down arrow. Turns blue when highlighted (d-pad focus).
// Non-window-owning; the parent handles input and opens the category list.
class CWunderlandCategoryBar : public CCoeControl
    {
public:
    static CWunderlandCategoryBar* NewL(const CCoeControl& aParent);
    ~CWunderlandCategoryBar();

    void SetPathL(const TDesC& aContentType, const TDesC& aCategory);
    void SetHighlighted(TBool aHighlighted);   // redraws if changed
    TBool IsHighlighted() const;

    TInt PreferredHeight() const;   // depends on the current layout font

private: // from CCoeControl
    void Draw(const TRect& aRect) const;

private:
    CWunderlandCategoryBar();
    void ConstructL(const CCoeControl& aParent);
    const CFont* Font() const;

private:
    HBufC* iText;   // owned
    TBool iHighlighted;
    };

#endif // __WUNDERLANDCATEGORYBAR_H__
