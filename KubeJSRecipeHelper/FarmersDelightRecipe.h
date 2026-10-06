#pragma once
#include "RecipeCatalog.h"
#include <vector>

namespace FarmersDelightRecipe {
enum class Version { Minecraft1201, Minecraft1211 };
enum class Method { Cooking, Cutting };
enum class Tool { Knives, Axes, Item, Tag };
struct Entry {
    std::wstring id;
    int count = 1;
    double chance = 100;
    RecipeEntryKind kind = RecipeItem;
};
struct Draft {
    std::vector<Entry> inputs, outputs;
    Entry container, toolItem;
    Tool tool = Tool::Knives;
    std::wstring toolTag, sound, group, tab;
    int cookingTime = 200;
    double experience = 0;
};
bool Build(Method method, Version version, const Draft& draft, std::wstring& script, std::wstring& error);
}
