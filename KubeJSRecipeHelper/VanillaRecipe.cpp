#include "VanillaRecipe.h"
#include <algorithm>
#include <vector>

namespace VanillaRecipe {
namespace {
const Info Infos[] = {
    {L"有序合成", L"shaped", 9, 0}, {L"无序合成", L"shapeless", 9, 0},
    {L"锻造台", L"smithing", 3, 0}, {L"熔炉", L"smelting", 1, 200},
    {L"高炉", L"blasting", 1, 100}, {L"烟熏炉", L"smoking", 1, 100},
    {L"切石机", L"stonecutting", 1, 0}
};
int Index(Method method) { return static_cast<int>(method); }
bool ValidMethod(Method method) { return Index(method) >= 0 && Index(method) < MethodCount; }
int DraftIndex(Method method) { return method == Method::Shapeless ? 0 : Index(method); }
std::wstring Quote(const std::wstring& id) {
    std::wstring text = L"'";
    for (wchar_t ch : id) {
        if (ch == L'\'' || ch == L'\\') text += L'\\';
        text += ch;
    }
    return text + L"'";
}
std::wstring Output(const Entry& entry) {
    return entry.count > 1 ? L"Item.of(" + Quote(entry.id) + L", " + std::to_wstring(entry.count) + L")" : Quote(entry.id);
}
}
const Info& GetInfo(Method method) { return Infos[ValidMethod(method) ? Index(method) : 0]; }
bool IsCrafting(Method method) { return method == Method::Shaped || method == Method::Shapeless; }
bool IsCooking(Method method) { return method >= Method::Smelting && method <= Method::Smoking; }
DraftBank::DraftBank() {
    for (int i = 0; i < MethodCount; ++i) m_drafts[i].time = GetInfo(static_cast<Method>(i)).defaultTime;
}
Draft& DraftBank::Get(Method method) { return m_drafts.at(DraftIndex(method)); }
const Draft& DraftBank::Get(Method method) const { return m_drafts.at(DraftIndex(method)); }
int InputCountLimit(Method method, const Draft& draft, int index) {
    if (method != Method::Shapeless) return 1;
    int remaining = 9;
    for (int i = 0; i < 9; ++i)
        if (i != index && !draft.inputs[i].id.empty()) remaining -= draft.inputs[i].count;
    return (std::max)(0, remaining);
}
bool Validate(Method method, const Draft& draft, std::wstring& error) {
    error.clear();
    if (!ValidMethod(method)) { error = L"请选择合成方式。"; return false; }
    if (draft.output.id.empty()) { error = L"请先选择产物。"; return false; }
    if (draft.output.count < 1 || draft.output.count > 64) { error = L"产物数量须为 1–64。"; return false; }
    int total = 0;
    for (int i = 0; i < GetInfo(method).inputs; ++i) {
        const auto& entry = draft.inputs[i];
        if (entry.id.empty()) {
            if (!IsCrafting(method)) { error = L"请先选择所有原料（锻造须选择模板、升级物品和材料）。"; return false; }
            continue;
        }
        // 有序配方每格代表一个原料，保留共享草稿中无序模式的数量。
        if (method == Method::Shapeless) {
            if (entry.count < 1 || entry.count > 9) { error = L"无序合成的原料总数不能超过 9 个。"; return false; }
            total += entry.count;
        } else {
            if (!IsCrafting(method) && entry.count != 1) { error = L"此合成方式的每格原料数量须为 1。"; return false; }
            ++total;
        }
    }
    if (total == 0) { error = L"请先在九宫格里放上材料。"; return false; }
    if (method == Method::Shapeless && total > 9) { error = L"无序合成的原料总数不能超过 9 个。"; return false; }
    if (IsCooking(method) && (draft.time < 1 || draft.time > 1000000)) {
        error = L"处理时间须为 1–1000000 tick。"; return false;
    }
    return true;
}
std::wstring BuildScript(Method method, const Draft& draft) {
    std::wstring error;
    if (!Validate(method, draft, error)) return L"";
    const auto& info = GetInfo(method);
    std::wstring script = L"ServerEvents.recipes(event => {\r\n  // " + std::wstring(info.name) + L" " + draft.output.id + L"\r\n";
    script += L"  event." + std::wstring(info.scriptMethod) + L"(";
    const auto output = Output(draft.output);
    if (method == Method::Shaped) {
        int top = 3, bottom = -1, left = 3, right = -1;
        for (int i = 0; i < 9; ++i) if (!draft.inputs[i].id.empty()) {
            top = (std::min)(top, i / 3); bottom = (std::max)(bottom, i / 3);
            left = (std::min)(left, i % 3); right = (std::max)(right, i % 3);
        }
        std::vector<std::wstring> keys;
        script += L"\r\n    " + output + L",\r\n    [\r\n";
        for (int row = top; row <= bottom; ++row) {
            std::wstring pattern;
            for (int col = left; col <= right; ++col) {
                const auto& id = draft.inputs[row * 3 + col].id;
                if (id.empty()) { pattern += L' '; continue; }
                auto it = std::find(keys.begin(), keys.end(), id);
                if (it == keys.end()) { keys.push_back(id); it = keys.end() - 1; }
                pattern += static_cast<wchar_t>(L'A' + std::distance(keys.begin(), it));
            }
            script += L"      " + Quote(pattern) + (row < bottom ? L"," : L"") + L"\r\n";
        }
        script += L"    ],\r\n    {\r\n";
        for (size_t i = 0; i < keys.size(); ++i)
            script += L"      " + std::wstring(1, static_cast<wchar_t>(L'A' + i)) + L": " + Quote(keys[i]) +
                (i + 1 < keys.size() ? L"," : L"") + L"\r\n";
        script += L"    }\r\n  )";
    } else if (method == Method::Shapeless) {
        std::vector<std::wstring> ingredients;
        for (const auto& entry : draft.inputs) if (!entry.id.empty())
            for (int n = 0; n < entry.count; ++n) ingredients.push_back(entry.id);
        script += L"\r\n    " + output + L",\r\n    [\r\n";
        for (size_t i = 0; i < ingredients.size(); ++i)
            script += L"      " + Quote(ingredients[i]) + (i + 1 < ingredients.size() ? L"," : L"") + L"\r\n";
        script += L"    ]\r\n  )";
    } else {
        script += output;
        for (int i = 0; i < info.inputs; ++i) script += L", " + Quote(draft.inputs[i].id);
        script += L")";
        if (IsCooking(method)) script += L".cookingTime(" + std::to_wstring(draft.time) + L")";
    }
    return script + L"\r\n})\r\n";
}
}
