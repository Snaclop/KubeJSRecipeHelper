#include "VanillaRecipe.h"
#include <algorithm>
#include <vector>

namespace VanillaRecipe {
namespace {
const Info arrInfos[] = {
    {L"有序合成", L"shaped", 9, 0}, {L"无序合成", L"shapeless", 9, 0},
    {L"锻造台", L"smithing", 3, 0}, {L"熔炉", L"smelting", 1, 200},
    {L"高炉", L"blasting", 1, 100}, {L"烟熏炉", L"smoking", 1, 100},
    {L"切石机", L"stonecutting", 1, 0}
};
int Index(Method eMethod) { return static_cast<int>(eMethod); }
bool ValidMethod(Method eMethod) { return Index(eMethod) >= 0 && Index(eMethod) < MethodCount; }
int DraftIndex(Method eMethod) { return eMethod == Method::Shapeless ? 0 : Index(eMethod); }
std::wstring Quote(const std::wstring& strId) {
    std::wstring strText = L"'";
    for (wchar_t chCharacter : strId) {
        if (chCharacter == L'\'' || chCharacter == L'\\') strText += L'\\';
        strText += chCharacter;
    }
    return strText + L"'";
}
std::wstring Output(const Entry& entry) {
    return entry.nCount > 1 ? L"Item.of(" + Quote(entry.strId) + L", " + std::to_wstring(entry.nCount) + L")" : Quote(entry.strId);
}
}
const Info& GetInfo(Method eMethod) { return arrInfos[ValidMethod(eMethod) ? Index(eMethod) : 0]; }
bool IsCrafting(Method eMethod) { return eMethod == Method::Shaped || eMethod == Method::Shapeless; }
bool IsCooking(Method eMethod) { return eMethod >= Method::Smelting && eMethod <= Method::Smoking; }
DraftBank::DraftBank() {
    for (int nIndex = 0; nIndex < MethodCount; ++nIndex) m_arrDrafts[nIndex].nTime = GetInfo(static_cast<Method>(nIndex)).nDefaultTime;
}
Draft& DraftBank::Get(Method eMethod) { return m_arrDrafts.at(DraftIndex(eMethod)); }
int InputCountLimit(Method eMethod, const Draft& draft, int nSlotIndex) {
    if (eMethod != Method::Shapeless) return 1;
    int nRemaining = 9;
    for (int nIndex = 0; nIndex < 9; ++nIndex)
        if (nIndex != nSlotIndex && !draft.arrInputs[nIndex].strId.empty()) nRemaining -= draft.arrInputs[nIndex].nCount;
    return (std::max)(0, nRemaining);
}
bool Validate(Method eMethod, const Draft& draft, std::wstring& strError) {
    strError.clear();
    if (!ValidMethod(eMethod)) { strError = L"请选择合成方式。"; return false; }
    if (draft.output.strId.empty()) { strError = L"请先选择产物。"; return false; }
    if (!RecipeValidId(draft.output.strId)) { strError = L"产物和过渡物品必须是具体 ID，不能使用标签。"; return false; }
    if (draft.output.nCount < 1 || draft.output.nCount > 64) { strError = L"产物数量须为 1–64。"; return false; }
    int nTotal = 0;
    for (int nIndex = 0; nIndex < GetInfo(eMethod).nInputs; ++nIndex) {
        const auto& entry = draft.arrInputs[nIndex];
        if (entry.strId.empty()) {
            if (!IsCrafting(eMethod)) { strError = L"请先选择所有原料（锻造须选择模板、升级物品和材料）。"; return false; }
            continue;
        }
        // 有序配方每格代表一个原料，保留共享草稿中无序模式的数量。
        if (!RecipeValidId(entry.strId, true)) { strError = L"物品或流体 ID 无效。"; return false; }
        if (eMethod == Method::Shapeless) {
            if (entry.nCount < 1 || entry.nCount > 9) { strError = L"无序合成的原料总数不能超过 9 个。"; return false; }
            nTotal += entry.nCount;
        } else {
            if (!IsCrafting(eMethod) && entry.nCount != 1) { strError = L"此合成方式的每格原料数量须为 1。"; return false; }
            ++nTotal;
        }
    }
    if (nTotal == 0) { strError = L"请先在九宫格里放上材料。"; return false; }
    if (eMethod == Method::Shapeless && nTotal > 9) { strError = L"无序合成的原料总数不能超过 9 个。"; return false; }
    if (IsCooking(eMethod) && (draft.nTime < 1 || draft.nTime > 1000000)) {
        strError = L"处理时间须为 1–1000000 tick。"; return false;
    }
    return true;
}
std::wstring BuildScript(Method eMethod, const Draft& draft) {
    std::wstring strError;
    if (!Validate(eMethod, draft, strError)) return L"";
    const auto& methodInfo = GetInfo(eMethod);
    std::wstring strScript = L"ServerEvents.recipes(event => {\r\n  // " + std::wstring(methodInfo.pName) + L" " + draft.output.strId + L"\r\n";
    strScript += L"  event." + std::wstring(methodInfo.pScriptMethod) + L"(";
    const auto strOutput = Output(draft.output);
    if (eMethod == Method::Shaped) {
        int nTop = 3, nBottom = -1, nLeft = 3, nRight = -1;
        for (int nIndex = 0; nIndex < 9; ++nIndex) if (!draft.arrInputs[nIndex].strId.empty()) {
            nTop = (std::min)(nTop, nIndex / 3); nBottom = (std::max)(nBottom, nIndex / 3);
            nLeft = (std::min)(nLeft, nIndex % 3); nRight = (std::max)(nRight, nIndex % 3);
        }
        std::vector<std::wstring> arrKeys;
        strScript += L"\r\n    " + strOutput + L",\r\n    [\r\n";
        for (int nRow = nTop; nRow <= nBottom; ++nRow) {
            std::wstring strPattern;
            for (int nColumn = nLeft; nColumn <= nRight; ++nColumn) {
                const auto& strId = draft.arrInputs[nRow * 3 + nColumn].strId;
                if (strId.empty()) { strPattern += L' '; continue; }
                auto iterator = std::find(arrKeys.begin(), arrKeys.end(), strId);
                if (iterator == arrKeys.end()) { arrKeys.push_back(strId); iterator = arrKeys.end() - 1; }
                strPattern += static_cast<wchar_t>(L'A' + std::distance(arrKeys.begin(), iterator));
            }
            strScript += L"      " + Quote(strPattern) + (nRow < nBottom ? L"," : L"") + L"\r\n";
        }
        strScript += L"    ],\r\n    {\r\n";
        for (size_t uIndex = 0; uIndex < arrKeys.size(); ++uIndex)
            strScript += L"      " + std::wstring(1, static_cast<wchar_t>(L'A' + uIndex)) + L": " + Quote(arrKeys[uIndex]) +
                (uIndex + 1 < arrKeys.size() ? L"," : L"") + L"\r\n";
        strScript += L"    }\r\n  )";
    } else if (eMethod == Method::Shapeless) {
        std::vector<std::wstring> arrIngredients;
        for (const auto& entry : draft.arrInputs) if (!entry.strId.empty())
            for (int nRepeat = 0; nRepeat < entry.nCount; ++nRepeat) arrIngredients.push_back(entry.strId);
        strScript += L"\r\n    " + strOutput + L",\r\n    [\r\n";
        for (size_t uIndex = 0; uIndex < arrIngredients.size(); ++uIndex)
            strScript += L"      " + Quote(arrIngredients[uIndex]) + (uIndex + 1 < arrIngredients.size() ? L"," : L"") + L"\r\n";
        strScript += L"    ]\r\n  )";
    } else {
        strScript += strOutput;
        for (int nIndex = 0; nIndex < methodInfo.nInputs; ++nIndex) strScript += L", " + Quote(draft.arrInputs[nIndex].strId);
        strScript += L")";
        if (IsCooking(eMethod)) strScript += L".cookingTime(" + std::to_wstring(draft.nTime) + L")";
    }
    return strScript + L"\r\n})\r\n";
}
}
