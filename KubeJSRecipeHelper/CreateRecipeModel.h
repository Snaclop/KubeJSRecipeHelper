#pragma once
#include "RecipeCatalog.h"
#include <vector>

namespace CreateRecipe {
enum class Version { Minecraft1201, Minecraft1211 };
inline int DefaultLoops(Version eVersion) { return eVersion == Version::Minecraft1201 ? 4 : 5; }
enum Method { Compacting, Crushing, Cutting, Deploying, Emptying, Filling,
    Haunting, MechanicalCrafting, Milling, Mixing, Pressing, Polishing,
    SequencedAssembly, Splashing, MethodCount };
struct Info {
    const wchar_t* pName;
    const wchar_t* pFunction;
    int nItemsIn, nItemsOut, nFluidsIn, nFluidsOut;
    bool bTime, bHeat;
};
const Info& GetInfo(Method eMethod);
struct Entry {
    std::wstring strId;
    RecipeEntryKind eKind = RecipeItem;
    int nCount = 1;
    double dChance = 100; // Percentage for processing; relative weight for assembly.
};
struct Step {
    Method eMethod = Pressing;
    Entry secondary;
    bool bKeepHeld = false, bCustomTime = false;
    int nTime = 100;
};
struct Draft {
    std::vector<Entry> arrInputs, arrOutputs;
    bool bCustomTime = false, bKeepHeld = false, bMirrored = true;
    int nTime = 100, nHeat = 0, nRows = 3, nColumns = 3, nLoops = 5;
    std::vector<Entry> arrGrid = std::vector<Entry>(81);
    Entry transition;
    std::vector<Step> arrSteps;
};
bool Build(Method eMethod, const Draft& draft, std::wstring& strScript, std::wstring& strError, Version eVersion);
}
