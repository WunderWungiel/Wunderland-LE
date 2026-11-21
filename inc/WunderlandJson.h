#ifndef __WUNDERLANDJSON_H__
#define __WUNDERLANDJSON_H__

#include <e32base.h>

// A parsed JSON value (a small DOM) for the store API responses.
//
// ParseL() reads UTF-8 JSON: raw UTF-8 and \uXXXX escapes (surrogate pairs
// included) both decode to 16-bit strings. Object keys stay 8-bit, since the
// API's keys are ASCII. Nesting is limited to KMaxDepth levels.
//
// The accessors never leave and never fail: asking a value for the wrong
// type gives a neutral result (0, EFalse, KNullDesC, NULL), so code reading
// a response doesn't have to check every step. Use Type() when it matters.
class CWunderlandJsonValue : public CBase
    {
public:
    enum TType
        {
        ENull,
        EBool,
        ENumber,
        EString,
        EArray,
        EObject
        };

    enum { KMaxDepth = 32 };

    // Leaves KErrCorrupt if aJson isn't one valid JSON value (whitespace
    // around it is allowed), KErrNoMemory if it runs out of memory.
    static CWunderlandJsonValue* ParseL(const TDesC8& aJson);
    static CWunderlandJsonValue* ParseLC(const TDesC8& aJson);
    ~CWunderlandJsonValue();

    TType Type() const;
    TBool IsNull() const;

    TBool Bool() const;            // EFalse unless EBool
    TReal64 Real() const;          // 0 unless ENumber
    TInt Int() const;              // ENumber truncated and clamped to 32 bits, else 0
    TBool IsInteger() const;       // ENumber written without fraction or exponent
    const TDesC& String() const;   // KNullDesC unless EString

    // Arrays and objects. For an object, At() is the value of the member at
    // aIndex and KeyAt() its key, in document order.
    TInt Count() const;
    const CWunderlandJsonValue& At(TInt aIndex) const;   // panics if out of range
    const TDesC8& KeyAt(TInt aIndex) const;              // KNullDesC8 unless EObject

    // Object members; NULL if this isn't an object or has no such key.
    const CWunderlandJsonValue* Member(const TDesC8& aKey) const;
    // Shortcuts that also accept a missing member or a wrong type.
    const TDesC& MemberString(const TDesC8& aKey) const;
    TInt MemberInt(const TDesC8& aKey, TInt aDefault = 0) const;
    TBool MemberBool(const TDesC8& aKey, TBool aDefault = EFalse) const;
    TBool MemberIsNull(const TDesC8& aKey) const;   // ETrue for null and for a missing key

private:
    class TParser;
    friend class TParser;

    static CWunderlandJsonValue* NewLC(TType aType);
    CWunderlandJsonValue(TType aType);

private:
    TType iType;
    TBool iBool;
    TBool iInteger;
    TReal64 iReal;
    TInt64 iInt;
    HBufC* iString;                               // owned, EString only
    RPointerArray<CWunderlandJsonValue> iItems;   // owned, EArray/EObject
    RPointerArray<HBufC8> iKeys;                  // owned, EObject, parallel to iItems
    };

#endif // __WUNDERLANDJSON_H__
