#include "WunderlandApiTypes.h"
#include "WunderlandJson.h"

namespace
    {
    // JSON keys
    _LIT8(KKeyAddon, "addon");
    _LIT8(KKeyApiVersion, "api_version");
    _LIT8(KKeyAverage, "average");
    _LIT8(KKeyBody, "body");
    _LIT8(KKeyCategory, "category");
    _LIT8(KKeyComparison, "comparison");
    _LIT8(KKeyContent, "content");
    _LIT8(KKeyContentCount, "content_count");
    _LIT8(KKeyContentId, "content_id");
    _LIT8(KKeyContentType, "content_type");
    _LIT8(KKeyCount, "count");
    _LIT8(KKeyCreatedAt, "created_at");
    _LIT8(KKeyDescription, "description");
    _LIT8(KKeyDirectUrl, "direct_url");
    _LIT8(KKeyDistribution, "distribution");
    _LIT8(KKeyDownload, "download");
    _LIT8(KKeyExternal, "external");
    _LIT8(KKeyIconUrl, "icon_url");
    _LIT8(KKeyId, "id");
    _LIT8(KKeyInstalledVersion, "installed_version");
    _LIT8(KKeyLinks, "links");
    _LIT8(KKeyName, "name");
    _LIT8(KKeyPage, "page");
    _LIT8(KKeyParentId, "parent_id");
    _LIT8(KKeyPerPage, "per_page");
    _LIT8(KKeyPerPageDefault, "per_page_default");
    _LIT8(KKeyPerPageMax, "per_page_max");
    _LIT8(KKeyPlatforms, "platforms");
    _LIT8(KKeyPrefix, "prefix");
    _LIT8(KKeyPublisher, "publisher");
    _LIT8(KKeyRating, "rating");
    _LIT8(KKeyRelated, "related");
    _LIT8(KKeyRounded, "rounded");
    _LIT8(KKeyScreenshots, "screenshots");
    _LIT8(KKeySelf, "self");
    _LIT8(KKeyServerTime, "server_time");
    _LIT8(KKeyShowCategories, "show_categories");
    _LIT8(KKeyShowIcons, "show_icons");
    _LIT8(KKeySize, "size");
    _LIT8(KKeyText, "text");
    _LIT8(KKeyTitle, "title");
    _LIT8(KKeyTotal, "total");
    _LIT8(KKeyTotalPages, "total_pages");
    _LIT8(KKeyTypeId, "type_id");
    _LIT8(KKeyUid, "uid");
    _LIT8(KKeyUpdateAvailable, "update_available");
    _LIT8(KKeyUpdatedAt, "updated_at");
    _LIT8(KKeyUrl, "url");
    _LIT8(KKeyVersion, "version");
    _LIT8(KKeyWeb, "web");

    _LIT(KComparisonNumeric, "numeric");
    _LIT(KComparisonString, "string");

    // NULL-safe member access
    const CWunderlandJsonValue* Get(const CWunderlandJsonValue* aObj, const TDesC8& aKey)
        {
        return aObj ? aObj->Member(aKey) : NULL;
        }

    const TDesC& Str(const CWunderlandJsonValue* aObj, const TDesC8& aKey)
        {
        return aObj ? aObj->MemberString(aKey) : KNullDesC();
        }

    TInt Int(const CWunderlandJsonValue* aObj, const TDesC8& aKey, TInt aDefault = 0)
        {
        return aObj ? aObj->MemberInt(aKey, aDefault) : aDefault;
        }

    TBool Bool(const CWunderlandJsonValue* aObj, const TDesC8& aKey)
        {
        return aObj ? aObj->MemberBool(aKey) : EFalse;
        }

    // Item aIndex of the array member aKey; NULL if out of range.
    const CWunderlandJsonValue* Item(const CWunderlandJsonValue* aObj, const TDesC8& aKey, TInt aIndex)
        {
        const CWunderlandJsonValue* array = Get(aObj, aKey);
        if (array && array->Type() == CWunderlandJsonValue::EArray
            && aIndex >= 0 && aIndex < array->Count())
            {
            return &array->At(aIndex);
            }
        return NULL;
        }

    TInt ArrayCount(const CWunderlandJsonValue* aObj, const TDesC8& aKey)
        {
        const CWunderlandJsonValue* array = Get(aObj, aKey);
        return (array && array->Type() == CWunderlandJsonValue::EArray) ? array->Count() : 0;
        }

    TInt Timestamp(const CWunderlandJsonValue* aObj, const TDesC8& aKey, TTime& aTime)
        {
        return WunderlandApi::ParseTimestamp(Str(aObj, aKey), aTime);
        }

    // aCount decimal digits at aPos, or -1
    TInt Digits(const TDesC& aText, TInt aPos, TInt aCount)
        {
        TInt value = 0;
        for (TInt i = aPos; i < aPos + aCount; ++i)
            {
            TUint c = aText[i];
            if (c < '0' || c > '9')
                {
                return -1;
                }
            value = value * 10 + (c - '0');
            }
        return value;
        }
    }

// ---------------------------------------------------------------------------

TInt WunderlandApi::ParseTimestamp(const TDesC& aText, TTime& aTime)
    {
    // 2026-11-17T10:04:00Z
    // 01234567890123456789
    const TInt KTimestampLength = 20;
    if (aText.Length() != KTimestampLength
        || aText[4] != '-' || aText[7] != '-' || aText[10] != 'T'
        || aText[13] != ':' || aText[16] != ':' || aText[19] != 'Z')
        {
        return KErrCorrupt;
        }
    TInt year = Digits(aText, 0, 4);
    TInt month = Digits(aText, 5, 2);
    TInt day = Digits(aText, 8, 2);
    TInt hour = Digits(aText, 11, 2);
    TInt minute = Digits(aText, 14, 2);
    TInt second = Digits(aText, 17, 2);
    if (year < 0 || month < 1 || month > 12 || day < 1 || hour < 0 || minute < 0 || second < 0)
        {
        return KErrCorrupt;
        }
    TDateTime dateTime;
    // TDateTime months and days count from 0.
    TInt err = dateTime.Set(year, static_cast<TMonth>(month - 1), day - 1, hour, minute, second, 0);
    if (err == KErrNone)
        {
        aTime = dateTime;
        }
    return err == KErrNone ? KErrNone : KErrCorrupt;
    }

// ---------------------------------------------------------------------------

TWunderlandPageMeta::TWunderlandPageMeta(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
TInt TWunderlandPageMeta::Page() const        { return Int(iJson, KKeyPage); }
TInt TWunderlandPageMeta::PerPage() const     { return Int(iJson, KKeyPerPage); }
TInt TWunderlandPageMeta::Total() const       { return Int(iJson, KKeyTotal); }
TInt TWunderlandPageMeta::TotalPages() const  { return Int(iJson, KKeyTotalPages); }
TBool TWunderlandPageMeta::HasNextPage() const { return Page() < TotalPages(); }

// ---------------------------------------------------------------------------

TWunderlandServerInfo::TWunderlandServerInfo(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
const TDesC& TWunderlandServerInfo::Name() const        { return Str(iJson, KKeyName); }
const TDesC& TWunderlandServerInfo::ApiVersion() const  { return Str(iJson, KKeyApiVersion); }
TInt TWunderlandServerInfo::PerPageDefault() const      { return Int(iJson, KKeyPerPageDefault); }
TInt TWunderlandServerInfo::PerPageMax() const          { return Int(iJson, KKeyPerPageMax); }
TInt TWunderlandServerInfo::ServerTime(TTime& aTime) const { return Timestamp(iJson, KKeyServerTime, aTime); }
const TDesC& TWunderlandServerInfo::ServerTimeText() const { return Str(iJson, KKeyServerTime); }
const TDesC& TWunderlandServerInfo::Link(const TDesC8& aName) const
    {
    return Str(Get(iJson, KKeyLinks), aName);
    }

// ---------------------------------------------------------------------------

TWunderlandContentType::TWunderlandContentType(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
const TDesC& TWunderlandContentType::Id() const     { return Str(iJson, KKeyId); }
const TDesC& TWunderlandContentType::Name() const   { return Str(iJson, KKeyName); }
const TDesC& TWunderlandContentType::Prefix() const { return Str(iJson, KKeyPrefix); }
TBool TWunderlandContentType::ShowIcons() const      { return Bool(iJson, KKeyShowIcons); }
TBool TWunderlandContentType::ShowCategories() const { return Bool(iJson, KKeyShowCategories); }

// ---------------------------------------------------------------------------

TWunderlandPlatform::TWunderlandPlatform(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
const TDesC& TWunderlandPlatform::Id() const       { return Str(iJson, KKeyId); }
const TDesC& TWunderlandPlatform::Name() const     { return Str(iJson, KKeyName); }
const TDesC& TWunderlandPlatform::ParentId() const { return Str(iJson, KKeyParentId); }

// ---------------------------------------------------------------------------

TWunderlandCategory::TWunderlandCategory(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
TInt TWunderlandCategory::Id() const             { return Int(iJson, KKeyId); }
const TDesC& TWunderlandCategory::Name() const   { return Str(iJson, KKeyName); }
const TDesC& TWunderlandCategory::TypeId() const { return Str(iJson, KKeyTypeId); }
TInt TWunderlandCategory::ContentCount() const   { return Int(iJson, KKeyContentCount); }

// ---------------------------------------------------------------------------

TWunderlandRating::TWunderlandRating(const CWunderlandJsonValue* aJson) : iJson(aJson) {}

TBool TWunderlandRating::HasAverage() const
    {
    const CWunderlandJsonValue* average = Get(iJson, KKeyAverage);
    return average && average->Type() == CWunderlandJsonValue::ENumber;
    }

TReal64 TWunderlandRating::Average() const
    {
    const CWunderlandJsonValue* average = Get(iJson, KKeyAverage);
    return average ? average->Real() : 0;
    }

TInt TWunderlandRating::AverageHalfStars() const
    {
    TInt halves = static_cast<TInt>(Average() * 2 + 0.5);
    return halves < 0 ? 0 : (halves > 10 ? 10 : halves);
    }

TInt TWunderlandRating::Count() const     { return Int(iJson, KKeyCount); }
TInt TWunderlandRating::Rounded() const   { return Int(iJson, KKeyRounded); }
TInt TWunderlandRating::ContentId() const { return Int(iJson, KKeyContentId); }

TInt TWunderlandRating::Distribution(TInt aStars) const
    {
    if (aStars < 1 || aStars > 5)
        {
        return 0;
        }
    TBuf8<1> key;
    key.Append(TChar('0' + aStars));
    return Int(Get(iJson, KKeyDistribution), key);
    }

// ---------------------------------------------------------------------------

TWunderlandDownload::TWunderlandDownload(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
const TDesC& TWunderlandDownload::Url() const       { return Str(iJson, KKeyUrl); }
const TDesC& TWunderlandDownload::DirectUrl() const { return Str(iJson, KKeyDirectUrl); }
TInt TWunderlandDownload::Size() const              { return Int(iJson, KKeySize, KErrNotFound); }
TBool TWunderlandDownload::IsExternal() const       { return Bool(iJson, KKeyExternal); }

// ---------------------------------------------------------------------------

TWunderlandContent::TWunderlandContent(const CWunderlandJsonValue* aJson) : iJson(aJson) {}

TBool TWunderlandContent::IsValid() const
    {
    const CWunderlandJsonValue* id = Get(iJson, KKeyId);
    return id && id->IsInteger();
    }

TInt TWunderlandContent::Id() const                 { return Int(iJson, KKeyId); }
const TDesC& TWunderlandContent::Title() const      { return Str(iJson, KKeyTitle); }
const TDesC& TWunderlandContent::TypeId() const     { return Str(Get(iJson, KKeyContentType), KKeyId); }
const TDesC& TWunderlandContent::TypeName() const   { return Str(Get(iJson, KKeyContentType), KKeyName); }
TInt TWunderlandContent::CategoryId() const         { return Int(Get(iJson, KKeyCategory), KKeyId); }
const TDesC& TWunderlandContent::CategoryName() const { return Str(Get(iJson, KKeyCategory), KKeyName); }
const TDesC& TWunderlandContent::Publisher() const  { return Str(iJson, KKeyPublisher); }
const TDesC& TWunderlandContent::Version() const    { return Str(iJson, KKeyVersion); }
const TDesC& TWunderlandContent::IconUrl() const    { return Str(iJson, KKeyIconUrl); }
TWunderlandRating TWunderlandContent::Rating() const { return TWunderlandRating(Get(iJson, KKeyRating)); }
TInt TWunderlandContent::PlatformCount() const      { return ArrayCount(iJson, KKeyPlatforms); }

TWunderlandPlatform TWunderlandContent::PlatformAt(TInt aIndex) const
    {
    return TWunderlandPlatform(Item(iJson, KKeyPlatforms, aIndex));
    }

TWunderlandDownload TWunderlandContent::Download() const
    {
    return TWunderlandDownload(Get(iJson, KKeyDownload));
    }

TInt TWunderlandContent::CreatedAt(TTime& aTime) const { return Timestamp(iJson, KKeyCreatedAt, aTime); }
TInt TWunderlandContent::UpdatedAt(TTime& aTime) const { return Timestamp(iJson, KKeyUpdatedAt, aTime); }
const TDesC& TWunderlandContent::SelfUrl() const    { return Str(Get(iJson, KKeyLinks), KKeySelf); }
const TDesC& TWunderlandContent::WebUrl() const     { return Str(Get(iJson, KKeyLinks), KKeyWeb); }
const TDesC& TWunderlandContent::Description() const { return Str(iJson, KKeyDescription); }
const TDesC& TWunderlandContent::Uid() const        { return Str(iJson, KKeyUid); }

TInt TWunderlandContent::UidValue(TUint32& aUid) const
    {
    const TDesC& uid = Uid();
    const TInt KUidLength = 10;   // "0x" + 8 hex digits
    if (uid.Length() != KUidLength || uid[0] != '0' || (uid[1] != 'x' && uid[1] != 'X'))
        {
        return KErrNotFound;
        }
    TLex lex(uid.Mid(2));
    TUint value = 0;
    if (lex.Val(value, EHex) != KErrNone || !lex.Eos())
        {
        return KErrNotFound;
        }
    aUid = value;
    return KErrNone;
    }

TInt TWunderlandContent::ScreenshotCount() const    { return ArrayCount(iJson, KKeyScreenshots); }

const TDesC& TWunderlandContent::ScreenshotUrl(TInt aIndex) const
    {
    return Str(Item(iJson, KKeyScreenshots, aIndex), KKeyUrl);
    }

TBool TWunderlandContent::HasAddon() const
    {
    const CWunderlandJsonValue* addon = Get(iJson, KKeyAddon);
    return addon && addon->Type() == CWunderlandJsonValue::EObject;
    }

const TDesC& TWunderlandContent::AddonText() const  { return Str(Get(iJson, KKeyAddon), KKeyText); }
const TDesC& TWunderlandContent::AddonUrl() const   { return Str(Get(iJson, KKeyAddon), KKeyUrl); }
const TDesC& TWunderlandContent::RelatedUrl() const { return Str(Get(iJson, KKeyLinks), KKeyRelated); }
const TDesC& TWunderlandContent::RatingUrl() const  { return Str(Get(iJson, KKeyLinks), KKeyRating); }

// ---------------------------------------------------------------------------

TWunderlandNews::TWunderlandNews(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
TInt TWunderlandNews::Id() const                { return Int(iJson, KKeyId); }
const TDesC& TWunderlandNews::Title() const     { return Str(iJson, KKeyTitle); }
TInt TWunderlandNews::CreatedAt(TTime& aTime) const { return Timestamp(iJson, KKeyCreatedAt, aTime); }
const TDesC& TWunderlandNews::Body() const      { return Str(iJson, KKeyBody); }
const TDesC& TWunderlandNews::SelfUrl() const   { return Str(Get(iJson, KKeyLinks), KKeySelf); }
const TDesC& TWunderlandNews::WebUrl() const    { return Str(Get(iJson, KKeyLinks), KKeyWeb); }

// ---------------------------------------------------------------------------

TWunderlandUpdate::TWunderlandUpdate(const CWunderlandJsonValue* aJson) : iJson(aJson) {}
const TDesC& TWunderlandUpdate::Uid() const              { return Str(iJson, KKeyUid); }
const TDesC& TWunderlandUpdate::InstalledVersion() const { return Str(iJson, KKeyInstalledVersion); }

TBool TWunderlandUpdate::HasContent() const
    {
    const CWunderlandJsonValue* content = Get(iJson, KKeyContent);
    return content && content->Type() == CWunderlandJsonValue::EObject;
    }

TWunderlandContent TWunderlandUpdate::Content() const
    {
    return TWunderlandContent(Get(iJson, KKeyContent));
    }

TWunderlandUpdate::TComparison TWunderlandUpdate::Comparison() const
    {
    const TDesC& comparison = Str(iJson, KKeyComparison);
    if (comparison == KComparisonNumeric) return EComparisonNumeric;
    if (comparison == KComparisonString) return EComparisonString;
    return EComparisonNone;
    }

TBool TWunderlandUpdate::UpdateAvailable() const { return Bool(iJson, KKeyUpdateAvailable); }

// ---------------------------------------------------------------------------

TWunderlandInstalledApp::TWunderlandInstalledApp(TUint32 aUid, const TDesC& aVersion)
    : iUid(aUid), iVersion(aVersion)
    {
    }
