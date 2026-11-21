#ifndef __WUNDERLANDAPITYPES_H__
#define __WUNDERLANDAPITYPES_H__

#include <e32base.h>

class CWunderlandJsonValue;

// Typed, read-only views over the objects of the Wunderland Store JSON API v1
// (docs/api.md in https://github.com/WunderWungiel/Wunderland-Store).
//
// A view holds a pointer into a parsed response and copies nothing, so it is
// only valid while that response (or the CWunderlandJsonValue taken from it)
// is alive. A view over NULL or over the wrong JSON type is allowed: every
// accessor then returns 0 / EFalse / KNullDesC. Unknown fields are ignored,
// as the API asks.
//
// Text accessors return the string as the server sent it. Fields that may be
// null in the API return KNullDesC for null.

namespace WunderlandApi
    {
    // Parses an API timestamp ("2026-11-17T10:04:00Z", UTC) into aTime.
    // Returns KErrCorrupt if aText isn't in that exact form.
    TInt ParseTimestamp(const TDesC& aText, TTime& aTime);
    }

// Pagination info of /content and /news ("meta").
class TWunderlandPageMeta
    {
public:
    TWunderlandPageMeta(const CWunderlandJsonValue* aJson);
    TInt Page() const;
    TInt PerPage() const;
    TInt Total() const;
    TInt TotalPages() const;
    TBool HasNextPage() const;   // Page() < TotalPages()
private:
    const CWunderlandJsonValue* iJson;
    };

// GET /
class TWunderlandServerInfo
    {
public:
    TWunderlandServerInfo(const CWunderlandJsonValue* aJson);
    const TDesC& Name() const;
    const TDesC& ApiVersion() const;
    TInt PerPageDefault() const;
    TInt PerPageMax() const;
    TInt ServerTime(TTime& aTime) const;    // KErrCorrupt if missing
    const TDesC& ServerTimeText() const;    // for updated_since
    const TDesC& Link(const TDesC8& aName) const;   // "content", "news", "web", ...
private:
    const CWunderlandJsonValue* iJson;
    };

// Content type (GET /content-types)
class TWunderlandContentType
    {
public:
    TWunderlandContentType(const CWunderlandJsonValue* aJson);
    const TDesC& Id() const;          // "apps", "games", ...
    const TDesC& Name() const;
    const TDesC& Prefix() const;      // for /legacy
    TBool ShowIcons() const;
    TBool ShowCategories() const;     // EFalse: browse /content?type= directly
private:
    const CWunderlandJsonValue* iJson;
    };

// Platform (GET /platforms; also inside content items, with Id and Name only)
class TWunderlandPlatform
    {
public:
    TWunderlandPlatform(const CWunderlandJsonValue* aJson);
    const TDesC& Id() const;          // "s60v3", ...
    const TDesC& Name() const;
    const TDesC& ParentId() const;    // KNullDesC for a top-level platform
private:
    const CWunderlandJsonValue* iJson;
    };

// Category (GET /categories)
class TWunderlandCategory
    {
public:
    TWunderlandCategory(const CWunderlandJsonValue* aJson);
    TInt Id() const;
    const TDesC& Name() const;
    const TDesC& TypeId() const;
    TInt ContentCount() const;
private:
    const CWunderlandJsonValue* iJson;
    };

// Rating: inside content items, and GET /content/{id}/rating (which adds
// ContentId and Distribution).
class TWunderlandRating
    {
public:
    TWunderlandRating(const CWunderlandJsonValue* aJson);
    TBool HasAverage() const;        // EFalse when nobody rated the item
    TReal64 Average() const;         // to one decimal; 0 when unrated
    TInt AverageHalfStars() const;   // Average rounded to the nearest half star, x2 (0-10)
    TInt Count() const;
    TInt Rounded() const;            // 0-5 whole stars, as the web store shows
    TInt ContentId() const;          // rating endpoint only
    TInt Distribution(TInt aStars) const;   // ratings with aStars (1-5); rating endpoint only
private:
    const CWunderlandJsonValue* iJson;
    };

// Download info of a content item.
class TWunderlandDownload
    {
public:
    TWunderlandDownload(const CWunderlandJsonValue* aJson);
    const TDesC& Url() const;         // use this one (redirects to the file)
    const TDesC& DirectUrl() const;   // informational only
    TInt Size() const;                // bytes, or KErrNotFound when unknown
    TBool IsExternal() const;         // hosted elsewhere: may need modern TLS, warn the user
private:
    const CWunderlandJsonValue* iJson;
    };

// Content item: a summary in lists, a detail from GET /content/{id}. The
// detail-only accessors return empty values on a summary.
class TWunderlandContent
    {
public:
    TWunderlandContent(const CWunderlandJsonValue* aJson);
    TBool IsValid() const;            // ETrue if this is an object with an id

    // Summary fields
    TInt Id() const;
    const TDesC& Title() const;
    const TDesC& TypeId() const;
    const TDesC& TypeName() const;
    TInt CategoryId() const;
    const TDesC& CategoryName() const;
    const TDesC& Publisher() const;
    const TDesC& Version() const;     // KNullDesC if none; show as is
    const TDesC& IconUrl() const;
    TWunderlandRating Rating() const;
    TInt PlatformCount() const;       // 0 means all platforms
    TWunderlandPlatform PlatformAt(TInt aIndex) const;
    TWunderlandDownload Download() const;
    TInt CreatedAt(TTime& aTime) const;
    TInt UpdatedAt(TTime& aTime) const;
    const TDesC& SelfUrl() const;
    const TDesC& WebUrl() const;

    // Detail fields
    const TDesC& Description() const;   // plain text, '\n' line breaks
    const TDesC& Uid() const;           // "0x2001C3A2" or KNullDesC
    TInt UidValue(TUint32& aUid) const; // Uid() as a number; KErrNotFound if none
    TInt ScreenshotCount() const;
    const TDesC& ScreenshotUrl(TInt aIndex) const;
    TBool HasAddon() const;
    const TDesC& AddonText() const;
    const TDesC& AddonUrl() const;
    const TDesC& RelatedUrl() const;
    const TDesC& RatingUrl() const;

private:
    const CWunderlandJsonValue* iJson;
    };

// News item (GET /news)
class TWunderlandNews
    {
public:
    TWunderlandNews(const CWunderlandJsonValue* aJson);
    TInt Id() const;
    const TDesC& Title() const;
    TInt CreatedAt(TTime& aTime) const;
    const TDesC& Body() const;         // HTML
    const TDesC& SelfUrl() const;
    const TDesC& WebUrl() const;
private:
    const CWunderlandJsonValue* iJson;
    };

// One entry of the POST /updates result.
class TWunderlandUpdate
    {
public:
    enum TComparison
        {
        EComparisonNone,      // no matching item, or it has no version
        EComparisonNumeric,   // UpdateAvailable() means "newer"
        EComparisonString     // UpdateAvailable() means only "different"
        };

    TWunderlandUpdate(const CWunderlandJsonValue* aJson);
    const TDesC& Uid() const;
    const TDesC& InstalledVersion() const;
    TBool HasContent() const;
    TWunderlandContent Content() const;
    TComparison Comparison() const;
    TBool UpdateAvailable() const;
private:
    const CWunderlandJsonValue* iJson;
    };

// One installed app for POST /updates.
class TWunderlandInstalledApp
    {
public:
    TWunderlandInstalledApp(TUint32 aUid, const TDesC& aVersion);
    TUint32 iUid;
    TPtrC iVersion;   // "2.09(0)" or "1.4.0"; 1-64 characters
    };

#endif // __WUNDERLANDAPITYPES_H__
