#pragma once
#include <map>
#include <string>

enum RecipeEntryKind { RecipeItem = 1, RecipeFluid = 2 };
using RecipeTypeMap = std::map<std::wstring, unsigned>;
inline unsigned RecipeKinds(const RecipeTypeMap* catalog, const std::wstring& id)
{
    if (catalog) {
        const auto it = catalog->find(id);
        if (it != catalog->end()) return it->second;
    }
    return RecipeItem;
}
