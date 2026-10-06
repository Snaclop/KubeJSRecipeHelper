#pragma once
#include "RecipeCatalog.h"
#include <vector>

namespace CreateRecipe {
enum class Version { Minecraft1201, Minecraft1211 };
inline int DefaultLoops(Version version) { return version == Version::Minecraft1201 ? 4 : 5; }
enum Method { Compacting, Crushing, Cutting, Deploying, Emptying, Filling,
    Haunting, MechanicalCrafting, Milling, Mixing, Pressing, Polishing,
    SequencedAssembly, Splashing, MethodCount };
struct Info {
    const wchar_t* name;
    const wchar_t* function;
    int itemsIn, itemsOut, fluidsIn, fluidsOut;
    bool time, heat;
};
const Info& GetInfo(Method method);
struct Entry {
    std::wstring id;
    RecipeEntryKind kind = RecipeItem;
    int count = 1;
    double chance = 100; // Percentage for processing; relative weight for assembly.
};
struct Step {
    Method method = Pressing;
    Entry secondary;
    bool keepHeld = false, customTime = false;
    int time = 100;
};
struct Draft {
    std::vector<Entry> inputs, outputs;
    bool customTime = false, keepHeld = false, mirrored = true;
    int time = 100, heat = 0, rows = 3, columns = 3, loops = 5;
    std::vector<Entry> grid = std::vector<Entry>(81);
    Entry transition;
    std::vector<Step> steps;
};
bool Build(Method method, const Draft& draft, std::wstring& script, std::wstring& error, Version version);
}
