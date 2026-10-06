#pragma once
#include "FarmersDelightRecipe.h"

namespace CreateAddonRecipe {
using Entry = FarmersDelightRecipe::Entry;
using Version = FarmersDelightRecipe::Version;
enum Method { Coloring, Freezing, Ending, Sanding, BasinFermenting, BulkFermenting,
    Distillation, CompressionMolding, Casting, Hammering, WireCutting, MethodCount };
struct Info {
    const wchar_t* name;
    const wchar_t* type;
    int itemsIn, itemsOut, fluidsIn, fluidsOut;
    bool time, heat, mold;
};
const Info& GetInfo(Method method);
struct Draft {
    std::vector<Entry> inputs, outputs, fluidInputs, fluidOutputs;
    bool customTime=false;
    int time=100, heat=0, dyeAmount=32;
    std::wstring color=L"minecraft:red", mold=L"createdieselgenerators:bar";
};
Draft MakeDraft(Method method);
bool Build(Method method, Version version, const Draft& draft, std::wstring& script, std::wstring& error);
}
