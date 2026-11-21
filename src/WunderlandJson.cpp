#include <e32math.h>   // KMaxTReal64
#include "WunderlandJson.h"

namespace
    {
    const TInt KItemGranularity = 8;
    const TUint KReplacementChar = 0xFFFD;

    inline TBool IsDigit(TUint aChar) { return aChar >= '0' && aChar <= '9'; }

    TInt HexValue(TUint aChar)   // -1 if not a hex digit
        {
        if (aChar >= '0' && aChar <= '9') return aChar - '0';
        if (aChar >= 'a' && aChar <= 'f') return aChar - 'a' + 10;
        if (aChar >= 'A' && aChar <= 'F') return aChar - 'A' + 10;
        return -1;
        }
    }

// ---------------------------------------------------------------------------
// Recursive-descent parser over the UTF-8 input
// ---------------------------------------------------------------------------

class CWunderlandJsonValue::TParser
    {
public:
    TParser(const TDesC8& aText);
    CWunderlandJsonValue* ParseDocumentLC();

private:
    CWunderlandJsonValue* ParseValueLC(TInt aDepth);
    CWunderlandJsonValue* ParseObjectLC(TInt aDepth);
    CWunderlandJsonValue* ParseArrayLC(TInt aDepth);
    CWunderlandJsonValue* ParseNumberLC();
    CWunderlandJsonValue* ParseLiteralLC();
    HBufC* ParseStringLC();
    HBufC8* ParseKeyLC();
    TUint ReadEscapedUnitL();   // after "\u": one UTF-16 code unit
    TUint ReadUtf8Char();       // one code point, KReplacementChar for bad bytes

    void SkipWhitespace();
    TBool AtEnd() const { return iPos >= iText.Length(); }
    TUint Peek() const { return AtEnd() ? 0 : iText[iPos]; }
    void ExpectL(TUint aChar);
    static void CorruptL() { User::Leave(KErrCorrupt); }

private:
    TPtrC8 iText;
    TInt iPos;
    };

CWunderlandJsonValue::TParser::TParser(const TDesC8& aText)
    : iText(aText), iPos(0)
    {
    }

CWunderlandJsonValue* CWunderlandJsonValue::TParser::ParseDocumentLC()
    {
    // Tolerate a UTF-8 byte order mark.
    if (iText.Length() >= 3 && iText[0] == 0xEF && iText[1] == 0xBB && iText[2] == 0xBF)
        {
        iPos = 3;
        }
    SkipWhitespace();
    CWunderlandJsonValue* value = ParseValueLC(0);
    SkipWhitespace();
    if (!AtEnd())
        {
        CorruptL();   // trailing garbage
        }
    return value;
    }

void CWunderlandJsonValue::TParser::SkipWhitespace()
    {
    while (!AtEnd())
        {
        TUint c = iText[iPos];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
            {
            break;
            }
        ++iPos;
        }
    }

void CWunderlandJsonValue::TParser::ExpectL(TUint aChar)
    {
    if (Peek() != aChar || AtEnd())
        {
        CorruptL();
        }
    ++iPos;
    }

CWunderlandJsonValue* CWunderlandJsonValue::TParser::ParseValueLC(TInt aDepth)
    {
    if (aDepth > KMaxDepth)
        {
        CorruptL();
        }
    switch (Peek())
        {
        case '{':
            return ParseObjectLC(aDepth + 1);
        case '[':
            return ParseArrayLC(aDepth + 1);
        case '"':
            {
            CWunderlandJsonValue* value = CWunderlandJsonValue::NewLC(EString);
            HBufC* text = ParseStringLC();
            CleanupStack::Pop(text);
            value->iString = text;   // owned by value from here
            return value;
            }
        case '-':
            return ParseNumberLC();
        default:
            if (IsDigit(Peek()))
                {
                return ParseNumberLC();
                }
            return ParseLiteralLC();
        }
    }

CWunderlandJsonValue* CWunderlandJsonValue::TParser::ParseObjectLC(TInt aDepth)
    {
    ExpectL('{');
    CWunderlandJsonValue* object = CWunderlandJsonValue::NewLC(EObject);
    SkipWhitespace();
    if (Peek() == '}')
        {
        ++iPos;
        return object;
        }
    for (;;)
        {
        SkipWhitespace();
        HBufC8* key = ParseKeyLC();
        SkipWhitespace();
        ExpectL(':');
        SkipWhitespace();
        CWunderlandJsonValue* value = ParseValueLC(aDepth);
        // Both arrays grow together so an index always means the same member.
        object->iItems.AppendL(value);
        CleanupStack::Pop(value);
        TInt err = object->iKeys.Append(key);
        if (err != KErrNone)
            {
            delete object->iItems[object->iItems.Count() - 1];
            object->iItems.Remove(object->iItems.Count() - 1);
            User::Leave(err);   // key is still on the cleanup stack
            }
        CleanupStack::Pop(key);
        SkipWhitespace();
        if (Peek() == ',')
            {
            ++iPos;
            continue;
            }
        ExpectL('}');
        return object;
        }
    }

CWunderlandJsonValue* CWunderlandJsonValue::TParser::ParseArrayLC(TInt aDepth)
    {
    ExpectL('[');
    CWunderlandJsonValue* array = CWunderlandJsonValue::NewLC(EArray);
    SkipWhitespace();
    if (Peek() == ']')
        {
        ++iPos;
        return array;
        }
    for (;;)
        {
        SkipWhitespace();
        CWunderlandJsonValue* value = ParseValueLC(aDepth);
        array->iItems.AppendL(value);
        CleanupStack::Pop(value);
        SkipWhitespace();
        if (Peek() == ',')
            {
            ++iPos;
            continue;
            }
        ExpectL(']');
        return array;
        }
    }

CWunderlandJsonValue* CWunderlandJsonValue::TParser::ParseNumberLC()
    {
    const TInt start = iPos;
    TBool integer = ETrue;
    if (Peek() == '-')
        {
        ++iPos;
        }
    // int: 0 | [1-9][0-9]*
    if (Peek() == '0' && !AtEnd())
        {
        ++iPos;
        }
    else if (IsDigit(Peek()) && !AtEnd())
        {
        while (!AtEnd() && IsDigit(Peek())) ++iPos;
        }
    else
        {
        CorruptL();
        }
    // frac
    if (!AtEnd() && Peek() == '.')
        {
        integer = EFalse;
        ++iPos;
        if (AtEnd() || !IsDigit(Peek())) CorruptL();
        while (!AtEnd() && IsDigit(Peek())) ++iPos;
        }
    // exp
    if (!AtEnd() && (Peek() == 'e' || Peek() == 'E'))
        {
        integer = EFalse;
        ++iPos;
        if (!AtEnd() && (Peek() == '+' || Peek() == '-')) ++iPos;
        if (AtEnd() || !IsDigit(Peek())) CorruptL();
        while (!AtEnd() && IsDigit(Peek())) ++iPos;
        }

    TPtrC8 token(iText.Mid(start, iPos - start));
    CWunderlandJsonValue* value = CWunderlandJsonValue::NewLC(ENumber);
    if (integer)
        {
        TLex8 lex(token);
        TInt64 number = 0;
        if (lex.Val(number) == KErrNone && lex.Eos())
            {
            value->iInteger = ETrue;
            value->iInt = number;
            value->iReal = static_cast<TReal64>(number);
            return value;
            }
        // Too big for 64 bits: fall back to a real.
        }
    TLex8 lex(token);
    TReal64 real = 0;
    TInt err = lex.Val(real, TChar('.'));   // explicit point: don't use the locale's
    if (err == KErrOverflow)
        {
        // Valid JSON, just out of range (1e999): clamp instead of failing the document.
        real = (token[0] == '-') ? -KMaxTReal64 : KMaxTReal64;
        }
    else if (err == KErrUnderflow)
        {
        real = 0;
        }
    else if (err != KErrNone)
        {
        CorruptL();
        }
    value->iReal = real;
    value->iInt = (real >= 9.2e18 || real <= -9.2e18) ? 0 : static_cast<TInt64>(real);
    return value;
    }

CWunderlandJsonValue* CWunderlandJsonValue::TParser::ParseLiteralLC()
    {
    _LIT8(KTrue, "true");
    _LIT8(KFalse, "false");
    _LIT8(KNull, "null");
    TPtrC8 rest(iText.Mid(iPos));
    if (rest.Left(KTrue().Length()) == KTrue)
        {
        iPos += KTrue().Length();
        CWunderlandJsonValue* value = CWunderlandJsonValue::NewLC(EBool);
        value->iBool = ETrue;
        return value;
        }
    if (rest.Left(KFalse().Length()) == KFalse)
        {
        iPos += KFalse().Length();
        return CWunderlandJsonValue::NewLC(EBool);
        }
    if (rest.Left(KNull().Length()) == KNull)
        {
        iPos += KNull().Length();
        return CWunderlandJsonValue::NewLC(ENull);
        }
    CorruptL();
    return NULL;   // not reached
    }

// Decodes the string starting at the opening quote into 16-bit text.
HBufC* CWunderlandJsonValue::TParser::ParseStringLC()
    {
    ExpectL('"');
    // Find the closing quote first. Each input byte gives at most one UTF-16
    // unit (a 4-byte UTF-8 sequence gives two, \uXXXX gives one for six
    // bytes), so the byte count is a safe buffer size.
    TInt end = iPos;
    while (end < iText.Length() && iText[end] != '"')
        {
        end += (iText[end] == '\\') ? 2 : 1;
        }
    if (end >= iText.Length())
        {
        CorruptL();   // unterminated
        }
    HBufC* text = HBufC::NewLC(end - iPos);
    TPtr des(text->Des());
    while (iPos < end)
        {
        TUint c = iText[iPos];
        if (c == '\\')
            {
            ++iPos;
            TUint esc = iText[iPos++];
            switch (esc)
                {
                case '"':  des.Append('"');  break;
                case '\\': des.Append('\\'); break;
                case '/':  des.Append('/');  break;
                case 'b':  des.Append(0x08); break;
                case 'f':  des.Append(0x0C); break;
                case 'n':  des.Append('\n'); break;
                case 'r':  des.Append('\r'); break;
                case 't':  des.Append('\t'); break;
                case 'u':
                    {
                    // A surrogate pair arrives as two escapes; descriptors are
                    // UTF-16, so a valid pair is stored as its two units. A
                    // surrogate without its partner becomes U+FFFD.
                    TUint unit = ReadEscapedUnitL();
                    if (unit >= 0xD800 && unit <= 0xDBFF)
                        {
                        TBool paired = EFalse;
                        if (iPos + 6 <= end && iText[iPos] == '\\' && iText[iPos + 1] == 'u')
                            {
                            const TInt next = iPos;
                            iPos += 2;
                            TUint low = ReadEscapedUnitL();
                            if (low >= 0xDC00 && low <= 0xDFFF)
                                {
                                des.Append(unit);
                                des.Append(low);
                                paired = ETrue;
                                }
                            else
                                {
                                iPos = next;   // decode that escape on its own
                                }
                            }
                        if (!paired)
                            {
                            des.Append(KReplacementChar);
                            }
                        }
                    else if (unit >= 0xDC00 && unit <= 0xDFFF)
                        {
                        des.Append(KReplacementChar);   // lone low surrogate
                        }
                    else
                        {
                        des.Append(unit);
                        }
                    break;
                    }
                default:
                    CorruptL();
                }
            }
        else if (c < 0x20)
            {
            CorruptL();   // raw control characters must be escaped in JSON
            }
        else if (c < 0x80)
            {
            des.Append(c);
            ++iPos;
            }
        else
            {
            TUint cp = ReadUtf8Char();
            if (cp >= 0x10000)
                {
                cp -= 0x10000;
                des.Append(0xD800 + (cp >> 10));
                des.Append(0xDC00 + (cp & 0x3FF));
                }
            else
                {
                des.Append(cp);
                }
            }
        }
    if (iPos != end)
        {
        CorruptL();   // a UTF-8 sequence or escape ran past the quote
        }
    ++iPos;   // closing quote
    return text;
    }

HBufC8* CWunderlandJsonValue::TParser::ParseKeyLC()
    {
    // Keys are ASCII in this API; anything else is narrowed (non-ASCII
    // characters would become unreadable but never crash).
    HBufC* key16 = ParseStringLC();
    HBufC8* key = HBufC8::NewL(key16->Length());
    key->Des().Copy(*key16);
    CleanupStack::PopAndDestroy(key16);
    CleanupStack::PushL(key);
    return key;
    }

TUint CWunderlandJsonValue::TParser::ReadEscapedUnitL()
    {
    if (iPos + 4 > iText.Length())
        {
        CorruptL();
        }
    TUint unit = 0;
    for (TInt i = 0; i < 4; ++i)
        {
        TInt digit = HexValue(iText[iPos++]);
        if (digit < 0)
            {
            CorruptL();
            }
        unit = (unit << 4) | digit;
        }
    return unit;
    }

TUint CWunderlandJsonValue::TParser::ReadUtf8Char()
    {
    TUint lead = iText[iPos++];
    TInt extra;
    TUint cp;
    TUint min;
    if (lead >= 0xC2 && lead <= 0xDF)      { extra = 1; cp = lead & 0x1F; min = 0x80; }
    else if (lead >= 0xE0 && lead <= 0xEF) { extra = 2; cp = lead & 0x0F; min = 0x800; }
    else if (lead >= 0xF0 && lead <= 0xF4) { extra = 3; cp = lead & 0x07; min = 0x10000; }
    else
        {
        return KReplacementChar;   // stray continuation byte or invalid lead byte
        }
    for (TInt i = 0; i < extra; ++i)
        {
        if (iPos >= iText.Length() || (iText[iPos] & 0xC0) != 0x80)
            {
            return KReplacementChar;   // truncated: resume at the offending byte
            }
        cp = (cp << 6) | (iText[iPos++] & 0x3F);
        }
    if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        {
        return KReplacementChar;   // overlong, out of range, or an encoded surrogate
        }
    return cp;
    }

// ---------------------------------------------------------------------------
// CWunderlandJsonValue
// ---------------------------------------------------------------------------

CWunderlandJsonValue* CWunderlandJsonValue::ParseL(const TDesC8& aJson)
    {
    CWunderlandJsonValue* value = ParseLC(aJson);
    CleanupStack::Pop(value);
    return value;
    }

CWunderlandJsonValue* CWunderlandJsonValue::ParseLC(const TDesC8& aJson)
    {
    TParser parser(aJson);
    return parser.ParseDocumentLC();
    }

CWunderlandJsonValue* CWunderlandJsonValue::NewLC(TType aType)
    {
    CWunderlandJsonValue* self = new (ELeave) CWunderlandJsonValue(aType);
    CleanupStack::PushL(self);
    return self;
    }

CWunderlandJsonValue::CWunderlandJsonValue(TType aType)
    : iType(aType), iItems(KItemGranularity), iKeys(KItemGranularity)
    {
    }

CWunderlandJsonValue::~CWunderlandJsonValue()
    {
    delete iString;
    iItems.ResetAndDestroy();
    iKeys.ResetAndDestroy();
    }

CWunderlandJsonValue::TType CWunderlandJsonValue::Type() const
    {
    return iType;
    }

TBool CWunderlandJsonValue::IsNull() const
    {
    return iType == ENull;
    }

TBool CWunderlandJsonValue::Bool() const
    {
    return iType == EBool && iBool;
    }

TReal64 CWunderlandJsonValue::Real() const
    {
    return iType == ENumber ? iReal : 0;
    }

TInt CWunderlandJsonValue::Int() const
    {
    if (iType != ENumber)
        {
        return 0;
        }
    if (iInt > KMaxTInt) return KMaxTInt;
    if (iInt < KMinTInt) return KMinTInt;
    return static_cast<TInt>(iInt);
    }

TBool CWunderlandJsonValue::IsInteger() const
    {
    return iType == ENumber && iInteger;
    }

const TDesC& CWunderlandJsonValue::String() const
    {
    if (iType == EString && iString)
        {
        return *iString;
        }
    return KNullDesC;
    }

TInt CWunderlandJsonValue::Count() const
    {
    return iItems.Count();
    }

const CWunderlandJsonValue& CWunderlandJsonValue::At(TInt aIndex) const
    {
    return *iItems[aIndex];
    }

const TDesC8& CWunderlandJsonValue::KeyAt(TInt aIndex) const
    {
    if (iType == EObject && aIndex >= 0 && aIndex < iKeys.Count())
        {
        return *iKeys[aIndex];
        }
    return KNullDesC8;
    }

const CWunderlandJsonValue* CWunderlandJsonValue::Member(const TDesC8& aKey) const
    {
    if (iType != EObject)
        {
        return NULL;
        }
    // Objects here are small (under 20 members), so a linear scan is fine.
    const TInt count = iKeys.Count();
    for (TInt i = 0; i < count; ++i)
        {
        if (*iKeys[i] == aKey)
            {
            return iItems[i];
            }
        }
    return NULL;
    }

const TDesC& CWunderlandJsonValue::MemberString(const TDesC8& aKey) const
    {
    const CWunderlandJsonValue* value = Member(aKey);
    return value ? value->String() : KNullDesC();
    }

TInt CWunderlandJsonValue::MemberInt(const TDesC8& aKey, TInt aDefault) const
    {
    const CWunderlandJsonValue* value = Member(aKey);
    return (value && value->iType == ENumber) ? value->Int() : aDefault;
    }

TBool CWunderlandJsonValue::MemberBool(const TDesC8& aKey, TBool aDefault) const
    {
    const CWunderlandJsonValue* value = Member(aKey);
    return (value && value->iType == EBool) ? value->iBool : aDefault;
    }

TBool CWunderlandJsonValue::MemberIsNull(const TDesC8& aKey) const
    {
    const CWunderlandJsonValue* value = Member(aKey);
    return !value || value->iType == ENull;
    }
