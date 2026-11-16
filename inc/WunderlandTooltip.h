#ifndef __WUNDERLANDTOOLTIP_H__
#define __WUNDERLANDTOOLTIP_H__

#include <coecntrl.h>

class CFont;

// Small dark grey box with a single line of grey text, shown under the
// focused tab. Non-window-owning; the parent positions it (PreferredSize)
// and keeps it last in its component list so it draws on top.
class CWunderlandTooltip : public CCoeControl
    {
public:
    static CWunderlandTooltip* NewL(const CCoeControl& aParent);
    ~CWunderlandTooltip();

    void SetTextL(const TDesC& aText);
    TSize PreferredSize() const;   // depends on the text and the layout font

private: // from CCoeControl
    void Draw(const TRect& aRect) const;

private:
    CWunderlandTooltip();
    void ConstructL(const CCoeControl& aParent);
    const CFont* Font() const;

private:
    HBufC* iText;   // owned
    };

#endif // __WUNDERLANDTOOLTIP_H__
