#pragma once
#include <map>
#include <string>

enum RecipeEntryKind { RecipeItem = 1, RecipeFluid = 2 };
using RecipeTypeMap = std::map<std::wstring, unsigned>;
inline bool RecipeIsTag(const std::wstring& id) { return !id.empty() && id.front() == L'#'; }
inline bool RecipeValidId(const std::wstring& value, bool allowTag = false)
{
    const std::wstring id = RecipeIsTag(value) && allowTag ? value.substr(1) : value;
    const auto colon = id.find(L':');
    if (colon == std::wstring::npos || colon == 0 || colon + 1 == id.size()) return false;
    for (size_t i = 0; i < id.size(); ++i) {
        const auto c = id[i]; if (i == colon) continue;
        if ((c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9') || c == L'_' || c == L'-' || c == L'.' || (i > colon && c == L'/')) continue;
        return false;
    }
    return true;
}
// Tag references keep the registry kind; # distinguishes them from concrete IDs.
inline bool RecipeTagFile(const std::string& path, std::string& id, unsigned& kind)
{
    if (path.compare(0, 5, "data/") != 0 || path.size() < 6 || path.compare(path.size() - 5, 5, ".json") != 0) return false;
    const auto slash = path.find('/', 5); if (slash == std::string::npos) return false;
    const auto tail = path.substr(slash + 1);
    for (const auto& dir : {"tags/item/", "tags/items/", "tags/fluid/", "tags/fluids/"}) {
        const std::string prefix(dir);
        if (tail.compare(0, prefix.size(), prefix) != 0) continue;
        id = "#" + path.substr(5, slash - 5) + ":" + tail.substr(prefix.size(), tail.size() - prefix.size() - 5);
        kind = prefix.find("/item") != std::string::npos ? RecipeItem : RecipeFluid;
        return RecipeValidId(std::wstring(id.begin(), id.end()), true);
    }
    return false;
}
inline unsigned RecipeKinds(const RecipeTypeMap* catalog, const std::wstring& id)
{
    if (catalog) {
        const auto it = catalog->find(id);
        if (it != catalog->end()) return it->second;
    }
    return RecipeItem;
}
