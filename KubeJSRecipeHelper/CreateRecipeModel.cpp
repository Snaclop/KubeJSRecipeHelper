#ifndef CREATE_MODEL_STANDALONE
#include "pch.h"
#endif
#include "CreateRecipeModel.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <limits>
#include <sstream>

namespace CreateRecipe {
const Info& GetInfo(Method eMethod) {
    static const Info arrMethodInfos[] = {
        { L"塑形", L"compacting", 64, 4, 2, 2, true, true },
        { L"粉碎", L"crushing", 1, 7, 0, 0, true, false },
        { L"切削", L"cutting", 1, 4, 0, 0, true, false },
        { L"使用（机械手）", L"deploying", 2, 4, 0, 0, false, false },
        { L"分液", L"emptying", 1, 1, 0, 1, false, false },
        { L"注液", L"filling", 1, 1, 1, 0, false, false },
        { L"批量缠魂", L"haunting", 1, 12, 0, 0, false, false },
        { L"动力合成", L"mechanical_crafting", 81, 1, 0, 0, false, false },
        { L"研磨", L"milling", 1, 4, 0, 0, true, false },
        { L"混合搅拌", L"mixing", 64, 4, 2, 2, true, true },
        { L"冲压", L"pressing", 1, 2, 0, 0, false, false },
        { L"砂纸打磨", L"sandpaper_polishing", 1, 1, 0, 0, false, false },
        { L"序列装配", L"sequenced_assembly", 1, 0, 0, 0, false, false },
        { L"批量洗涤", L"splashing", 1, 12, 0, 0, false, false }
    };
    return arrMethodInfos[eMethod];
}
static std::wstring Number(double dNumber) {
    std::wostringstream outputStream; outputStream.imbue(std::locale::classic());
    outputStream << std::setprecision(std::numeric_limits<double>::max_digits10) << dNumber; return outputStream.str();
}
static std::wstring Quote(const std::wstring& strId) {
    std::wstring strQuoted = L"'";
    for (wchar_t chCharacter : strId) {
        if (chCharacter == L'\'' || chCharacter == L'\\') strQuoted += L'\\';
        if (chCharacter == L'\r') strQuoted += L"\\r";
        else if (chCharacter == L'\n') strQuoted += L"\\n";
        else strQuoted += chCharacter;
    }
    return strQuoted + L"'";
}
static std::wstring Item(const Entry& entry) {
    return L"Item.of(" + Quote(entry.strId) + L", " + std::to_wstring(entry.nCount) + L")";
}
static std::wstring Value(const Entry& entry, bool bOutput, Version eVersion, double dWeightScale = 0) {
    if (entry.eKind == RecipeFluid && RecipeIsTag(entry.strId)) {
        if (eVersion == Version::Minecraft1211)
            return L"Fluid.sizedIngredientOf(Fluid.ingredientOf(" + Quote(entry.strId) + L"), " + std::to_wstring(entry.nCount) + L")";
        return L"{ fluidTag: " + Quote(entry.strId.substr(1)) + L", amount: " + std::to_wstring(entry.nCount) + L" }";
    }
    if (entry.eKind == RecipeFluid)
        return L"Fluid.of(" + Quote(entry.strId) + L", " + std::to_wstring(entry.nCount) + L")";
    if (!bOutput) return L"Ingredient.of(" + Quote(entry.strId) + L")";
    if (dWeightScale > 0 || entry.dChance != 100) {
        if (eVersion == Version::Minecraft1201)
            return Item(entry) + L".withChance(" + Number(dWeightScale > 0 ? entry.dChance : entry.dChance / 100) + L")";
        return L"CreateItem.of(" + Item(entry) + L", " + Number(dWeightScale > 0 ? entry.dChance / dWeightScale : entry.dChance / 100) + L")";
    }
    return Item(entry);
}
static std::wstring Array(const std::vector<Entry>& arrEntries, bool bOutput, Version eVersion, double dScale = 0) {
    std::wstring strResult = L"["; bool bFirst = true;
    for (const auto& entry : arrEntries) {
        if (entry.strId.empty()) continue;
        const int nRepeat = !bOutput && entry.eKind == RecipeItem ? entry.nCount : 1;
        for (int nIndex = 0; nIndex < nRepeat; ++nIndex) {
            if (!bFirst) strResult += L", "; bFirst = false;
            strResult += Value(entry, bOutput, eVersion, dScale);
        }
    }
    return strResult + L"]";
}
static bool Check(const Entry& entry, bool bOutput, bool bWeights, std::wstring& strError) {
    if (entry.strId.empty()) return true;
    if (bOutput && RecipeIsTag(entry.strId)) { strError = L"产物和过渡物品必须是具体 ID，不能使用标签。"; return false; }
    if (!RecipeValidId(entry.strId, !bOutput)) { strError = L"物品或流体 ID 无效。"; return false; }
    if (entry.nCount < 1 || entry.nCount > (entry.eKind == RecipeFluid ? 99999999 : 64)) {
        strError = L"物品数量须为 1–64，流体量须为 1–99999999 mB。"; return false;
    }
    if (bOutput && entry.eKind == RecipeItem && (!std::isfinite(entry.dChance) ||
        (bWeights ? entry.dChance <= 0 || entry.dChance > 1000000 : entry.dChance < 0 || entry.dChance > 100))) {
        strError = bWeights ? L"产物权重须大于 0 且不超过 1000000。" : L"概率须为 0–100%。"; return false;
    }
    if (entry.eKind == RecipeFluid && bOutput && entry.dChance != 100) {
        strError = L"流体产物不支持概率。"; return false;
    }
    return true;
}
bool Build(Method eMethod, const Draft& draft, std::wstring& strScript, std::wstring& strError, Version eVersion) {
    strScript.clear(); strError.clear();
    const auto& methodInfo = GetInfo(eMethod);
    int nItemInputs = 0, nFluidInputs = 0, nItemOutputs = 0, nFluidOutputs = 0;
    for (const auto& entry : draft.arrInputs) {
        if (!Check(entry, false, false, strError)) return false;
        if (!entry.strId.empty()) { if (entry.eKind == RecipeFluid) ++nFluidInputs; else nItemInputs += entry.nCount; }
    }
    for (const auto& entry : draft.arrOutputs) {
        if (!Check(entry, true, eMethod == SequencedAssembly, strError)) return false;
        if (!entry.strId.empty()) { if (entry.eKind == RecipeFluid) ++nFluidOutputs; else ++nItemOutputs; }
    }
    if (!nItemOutputs && !nFluidOutputs) { strError = L"请至少选择一个产物。"; return false; }
    if (draft.bCustomTime && methodInfo.bTime && (draft.nTime < 1 || draft.nTime > 99999999)) {
        strError = L"处理时间须为 1–99999999 tick。"; return false;
    }
    std::wstring strCall;
    if (eMethod == MechanicalCrafting) {
        if (nItemOutputs != 1 || nFluidOutputs || draft.nRows < 1 || draft.nRows > 9 || draft.nColumns < 1 || draft.nColumns > 9 || draft.arrGrid.size() != 81) {
            strError = L"动力合成须有一个物品产物，网格大小须为 1–9 行/列。"; return false;
        }
        for (const auto& entry : draft.arrOutputs) if (!entry.strId.empty() && entry.dChance != 100) {
            strError = L"动力合成产物不支持概率。"; return false;
        }
        std::map<std::wstring, wchar_t> mapKeys;
        std::wstring strPattern = L"[", strMapping = L"{";
        // 81 different symbols, excluding space, quotes and backslash.
        const std::wstring strSymbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,-./:;<=>?@[]^_`{|}~";
        for (int nRow = 0; nRow < draft.nRows; ++nRow) {
            std::wstring strLine;
            for (int nColumn = 0; nColumn < draft.nColumns; ++nColumn) {
                const auto& entry = draft.arrGrid[nRow * 9 + nColumn];
                if (entry.strId.empty()) { strLine += L' '; continue; }
                if (!Check(entry, false, false, strError)) return false;
                if (entry.eKind != RecipeItem || entry.nCount != 1) { strError = L"合成网格每格只能放一个物品。"; return false; }
                auto iterator = mapKeys.find(entry.strId);
                if (iterator == mapKeys.end()) {
                    wchar_t chKey = strSymbols[mapKeys.size()]; mapKeys[entry.strId] = chKey;
                    if (mapKeys.size() > 1) strMapping += L", ";
                    strMapping += Quote(std::wstring(1, chKey)) + L": Ingredient.of(" + Quote(entry.strId) + L")";
                    strLine += chKey;
                } else strLine += iterator->second;
            }
            if (nRow) strPattern += L", "; strPattern += Quote(strLine);
        }
        if (mapKeys.empty()) { strError = L"请在动力合成网格中选择原料。"; return false; }
        const Entry* pOutput = nullptr;
        for (const auto& entry : draft.arrOutputs) if (!entry.strId.empty()) pOutput = &entry;
        strCall = L"event.recipes.create.mechanical_crafting(" + Item(*pOutput) + L", " + strPattern + L"], " + strMapping + L"})";
        if (eVersion == Version::Minecraft1201)
            strCall += draft.bMirrored ? L".merge({ acceptMirrored: true })" : L".merge({ acceptMirrored: false })";
        else
            strCall += draft.bMirrored ? L".acceptMirrored(true)" : L".acceptMirrored(false)";
    } else if (eMethod == SequencedAssembly) {
        if (RecipeIsTag(draft.transition.strId)) { strError = L"产物和过渡物品必须是具体 ID，不能使用标签。"; return false; }
        if (!draft.transition.strId.empty() && !Check(draft.transition, true, false, strError)) return false;
        if (nItemInputs != 1 || nFluidInputs || nFluidOutputs || draft.transition.strId.empty() || draft.transition.eKind != RecipeItem || draft.transition.nCount != 1 ||
            draft.arrSteps.empty() || draft.nLoops < 1 || draft.nLoops > 99999999) {
            strError = L"序列装配需要一个原料、过渡物品、至少一个步骤和大于 0 的循环次数；产物只能是物品。"; return false;
        }
        double dLargest = 0;
        for (const auto& entry : draft.arrOutputs) if (!entry.strId.empty()) dLargest = (std::max)(dLargest, entry.dChance);
        strCall = L"event.recipes.create.sequenced_assembly(" + Array(draft.arrOutputs, true, eVersion, dLargest) + L", " + Array(draft.arrInputs, false, eVersion).substr(1);
        // The assembly ingredient is singular, not an array.
        strCall.pop_back(); strCall += L", [\r\n";
        for (size_t uIndex = 0; uIndex < draft.arrSteps.size(); ++uIndex) {
            const auto& step = draft.arrSteps[uIndex];
            if (step.eMethod != Pressing && step.eMethod != Cutting && step.eMethod != Deploying && step.eMethod != Filling) {
                strError = L"序列步骤只支持冲压、切削、使用和注液。"; return false;
            }
            if (!Check(step.secondary, false, false, strError)) return false;
            std::vector<Entry> arrInputs = { draft.transition };
            if (step.eMethod == Deploying || step.eMethod == Filling) {
                if (step.secondary.strId.empty() || step.secondary.eKind != (step.eMethod == Filling ? RecipeFluid : RecipeItem) ||
                    (step.eMethod == Deploying && step.secondary.nCount != 1)) {
                    strError = L"请为机械手步骤选择一个工具/材料，为注液步骤选择流体。"; return false;
                }
                arrInputs.push_back(step.secondary);
            }
            strCall += L"    event.recipes.create." + std::wstring(GetInfo(step.eMethod).pFunction) + L"(" + Item(draft.transition) + L", " + Array(arrInputs, false, eVersion) + L")";
            if (step.eMethod == Deploying && step.bKeepHeld) strCall += L".keepHeldItem()";
            if (step.eMethod == Cutting && step.bCustomTime) {
                if (step.nTime < 1 || step.nTime > 99999999) { strError = L"步骤处理时间须大于 0。"; return false; }
                strCall += L".processingTime(" + std::to_wstring(step.nTime) + L")";
            }
            strCall += uIndex + 1 < draft.arrSteps.size() ? L",\r\n" : L"\r\n";
        }
        strCall += L"  ]).transitionalItem(" + Quote(draft.transition.strId) + L").loops(" + std::to_wstring(draft.nLoops) + L")";
    } else {
        if (!nItemInputs && !nFluidInputs) { strError = L"请至少选择一个原料。"; return false; }
        if (nItemInputs > methodInfo.nItemsIn || nItemOutputs > methodInfo.nItemsOut || nFluidInputs > methodInfo.nFluidsIn || nFluidOutputs > methodInfo.nFluidsOut) {
            strError = L"原料或产物数量超过此处理方法的限制。"; return false;
        }
        if ((eMethod == Deploying && nItemInputs != 2) || (eMethod == Filling && (nItemInputs != 1 || nFluidInputs != 1)) ||
            (eMethod == Emptying && (nItemInputs != 1 || nItemOutputs != 1 || nFluidOutputs != 1)) || (eMethod == Compacting && nItemOutputs == 0)) {
            strError = L"使用需要两个物品原料；注液需要物品和流体；分液需要物品和流体产物；塑形需要物品产物。"; return false;
        }
        strCall = L"event.recipes.create." + std::wstring(methodInfo.pFunction) + L"(" + Array(draft.arrOutputs, true, eVersion) + L", " + Array(draft.arrInputs, false, eVersion) + L")";
        if (methodInfo.bHeat) {
            if (draft.nHeat < 0 || draft.nHeat > 2) { strError = L"请选择合法的加热条件。"; return false; }
            if (draft.nHeat == 1) strCall += L".heated()";
            if (draft.nHeat == 2) strCall += L".superheated()";
        }
        if (methodInfo.bTime && draft.bCustomTime) strCall += L".processingTime(" + std::to_wstring(draft.nTime) + L")";
        if (eMethod == Deploying && draft.bKeepHeld) strCall += L".keepHeldItem()";
    }
    const wchar_t* pTarget = eVersion == Version::Minecraft1201 ? L"1.20.1" : L"1.21.1";
    strScript = L"ServerEvents.recipes(event => {\r\n  // Minecraft " + std::wstring(pTarget) + L" · " + methodInfo.pName + L"\r\n  " + strCall + L"\r\n})\r\n";
    return true;
}
}
