#pragma once
#include <map>
#include <string>

enum RecipeEntryKind { RecipeItem = 1, RecipeFluid = 2 };
using RecipeTypeMap = std::map<std::wstring, unsigned>;
namespace RecipeCatalogDetail {
template<class Character> bool NamespaceCharacter(Character chCharacter) {
    return (chCharacter >= 'a' && chCharacter <= 'z') || (chCharacter >= '0' && chCharacter <= '9') ||
        chCharacter == '_' || chCharacter == '-' || chCharacter == '.';
}
template<class Character> bool IsTag(const std::basic_string<Character>& strId) {
    return !strId.empty() && strId.front() == '#';
}
template<class Character> bool ValidNamespace(const std::basic_string<Character>& strNamespace) {
    if (strNamespace.empty()) return false;
    for (const auto chCharacter : strNamespace) if (!NamespaceCharacter(chCharacter)) return false;
    return true;
}
template<class Character> bool ValidId(const std::basic_string<Character>& strValue, bool bAllowTag) {
    const auto uStart = bAllowTag && IsTag(strValue) ? 1u : 0u;
    const auto uColon = strValue.find(static_cast<Character>(':'), uStart);
    if (uColon == std::basic_string<Character>::npos || uColon == uStart || uColon + 1 == strValue.size()) return false;
    for (size_t uIndex = uStart; uIndex < strValue.size(); ++uIndex) {
        if (uIndex == uColon) continue;
        const auto chCharacter = strValue[uIndex];
        if (!NamespaceCharacter(chCharacter) && !(uIndex > uColon && chCharacter == '/')) return false;
    }
    return true;
}
}
inline bool RecipeIsTag(const std::string& strId) { return RecipeCatalogDetail::IsTag(strId); }
inline bool RecipeIsTag(const std::wstring& strId) { return RecipeCatalogDetail::IsTag(strId); }
inline bool RecipeValidNamespace(const std::string& strNamespace) { return RecipeCatalogDetail::ValidNamespace(strNamespace); }
inline bool RecipeValidNamespace(const std::wstring& strNamespace) { return RecipeCatalogDetail::ValidNamespace(strNamespace); }
inline bool RecipeValidId(const std::string& strValue, bool bAllowTag = false) { return RecipeCatalogDetail::ValidId(strValue, bAllowTag); }
inline bool RecipeValidId(const std::wstring& strValue, bool bAllowTag = false) { return RecipeCatalogDetail::ValidId(strValue, bAllowTag); }
// Tag references keep the registry kind; # distinguishes them from concrete IDs.
inline bool RecipeTagFile(const std::string& strPath, std::string& strId, unsigned& uKind)
{
    if (strPath.compare(0, 5, "data/") != 0 || strPath.size() < 6 || strPath.compare(strPath.size() - 5, 5, ".json") != 0) return false;
    const auto uSlash = strPath.find('/', 5); if (uSlash == std::string::npos) return false;
    const auto strTail = strPath.substr(uSlash + 1);
    for (const auto* pDirectory : {"tags/item/", "tags/items/", "tags/fluid/", "tags/fluids/"}) {
        const std::string strPrefix(pDirectory);
        if (strTail.compare(0, strPrefix.size(), strPrefix) != 0) continue;
        strId = "#" + strPath.substr(5, uSlash - 5) + ":" + strTail.substr(strPrefix.size(), strTail.size() - strPrefix.size() - 5);
        uKind = strPrefix.find("/item") != std::string::npos ? RecipeItem : RecipeFluid;
        return RecipeValidId(strId, true);
    }
    return false;
}
inline unsigned RecipeKinds(const RecipeTypeMap* pCatalog, const std::wstring& strId)
{
    if (pCatalog) {
        const auto iterator = pCatalog->find(strId);
        if (iterator != pCatalog->end()) return iterator->second;
    }
    return RecipeItem;
}
