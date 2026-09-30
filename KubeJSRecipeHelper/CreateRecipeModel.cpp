#ifndef CREATE_MODEL_STANDALONE
#include "pch.h"
#endif
#include "CreateRecipeModel.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace CreateRecipe {
const Info& GetInfo(Method method) {
    static const Info info[] = {
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
    return info[method];
}
static std::wstring Number(double n) {
    std::wostringstream out; out.imbue(std::locale::classic());
    out << std::setprecision(15) << n; return out.str();
}
static std::wstring Quote(const std::wstring& id) {
    std::wstring out = L"'";
    for (wchar_t c : id) {
        if (c == L'\'' || c == L'\\') out += L'\\';
        if (c == L'\r') out += L"\\r";
        else if (c == L'\n') out += L"\\n";
        else out += c;
    }
    return out + L"'";
}
static std::wstring Item(const Entry& e) {
    return L"Item.of(" + Quote(e.id) + L", " + std::to_wstring(e.count) + L")";
}
static std::wstring Value(const Entry& e, bool output, double weightScale = 0) {
    if (e.kind == RecipeFluid)
        return L"Fluid.of(" + Quote(e.id) + L", " + std::to_wstring(e.count) + L")";
    if (!output) return L"Ingredient.of(" + Quote(e.id) + L")";
    if (weightScale > 0 || e.chance != 100)
        return L"CreateItem.of(" + Item(e) + L", " + Number(weightScale > 0 ? e.chance / weightScale : e.chance / 100) + L")";
    return Item(e);
}
static std::wstring Array(const std::vector<Entry>& entries, bool output, double scale = 0) {
    std::wstring result = L"["; bool first = true;
    for (const auto& e : entries) {
        if (e.id.empty()) continue;
        const int repeat = !output && e.kind == RecipeItem ? e.count : 1;
        for (int n = 0; n < repeat; ++n) {
            if (!first) result += L", "; first = false;
            result += Value(e, output, scale);
        }
    }
    return result + L"]";
}
static bool Check(const Entry& e, bool output, bool weights, std::wstring& error) {
    if (e.id.empty()) return true;
    if (e.count < 1 || e.count > (e.kind == RecipeFluid ? 99999999 : 64)) {
        error = L"物品数量须为 1–64，流体量须为 1–99999999 mB。"; return false;
    }
    if (output && e.kind == RecipeItem && (!std::isfinite(e.chance) ||
        (weights ? e.chance <= 0 || e.chance > 1000000 : e.chance < 0 || e.chance > 100))) {
        error = weights ? L"产物权重须大于 0 且不超过 1000000。" : L"概率须为 0–100%。"; return false;
    }
    if (e.kind == RecipeFluid && output && e.chance != 100) {
        error = L"流体产物不支持概率。"; return false;
    }
    return true;
}
bool Build(Method method, const Draft& d, std::wstring& script, std::wstring& error) {
    script.clear(); error.clear();
    const auto& info = GetInfo(method);
    int ii = 0, fi = 0, io = 0, fo = 0;
    for (const auto& e : d.inputs) {
        if (!Check(e, false, false, error)) return false;
        if (!e.id.empty()) { if (e.kind == RecipeFluid) ++fi; else ii += e.count; }
    }
    for (const auto& e : d.outputs) {
        if (!Check(e, true, method == SequencedAssembly, error)) return false;
        if (!e.id.empty()) { if (e.kind == RecipeFluid) ++fo; else ++io; }
    }
    if (!io && !fo) { error = L"请至少选择一个产物。"; return false; }
    if (d.customTime && info.time && (d.time < 1 || d.time > 99999999)) {
        error = L"处理时间须为 1–99999999 tick。"; return false;
    }
    std::wstring call;
    if (method == MechanicalCrafting) {
        if (io != 1 || fo || d.rows < 1 || d.rows > 9 || d.columns < 1 || d.columns > 9 || d.grid.size() != 81) {
            error = L"动力合成须有一个物品产物，网格大小须为 1–9 行/列。"; return false;
        }
        for (const auto& e : d.outputs) if (!e.id.empty() && e.chance != 100) {
            error = L"动力合成产物不支持概率。"; return false;
        }
        std::map<std::wstring, wchar_t> keys;
        std::wstring pattern = L"[", mapping = L"{";
        // 81 different symbols, excluding space, quotes and backslash.
        const std::wstring symbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,-./:;<=>?@[]^_`{|}~";
        for (int row = 0; row < d.rows; ++row) {
            std::wstring line;
            for (int col = 0; col < d.columns; ++col) {
                const auto& e = d.grid[row * 9 + col];
                if (e.id.empty()) { line += L' '; continue; }
                if (e.kind != RecipeItem || e.count != 1) { error = L"合成网格每格只能放一个物品。"; return false; }
                auto it = keys.find(e.id);
                if (it == keys.end()) {
                    wchar_t key = symbols[keys.size()]; keys[e.id] = key;
                    if (keys.size() > 1) mapping += L", ";
                    mapping += Quote(std::wstring(1, key)) + L": Ingredient.of(" + Quote(e.id) + L")";
                    line += key;
                } else line += it->second;
            }
            if (row) pattern += L", "; pattern += Quote(line);
        }
        if (keys.empty()) { error = L"请在动力合成网格中选择原料。"; return false; }
        const Entry* output = nullptr;
        for (const auto& e : d.outputs) if (!e.id.empty()) output = &e;
        call = L"event.recipes.create.mechanical_crafting(" + Item(*output) + L", " + pattern + L"], " + mapping + L"})";
        call += d.mirrored ? L".acceptMirrored(true)" : L".acceptMirrored(false)";
    } else if (method == SequencedAssembly) {
        if (ii != 1 || fi || fo || d.transition.id.empty() || d.transition.kind != RecipeItem || d.transition.count != 1 ||
            d.steps.empty() || d.loops < 1 || d.loops > 99999999) {
            error = L"序列装配需要一个原料、过渡物品、至少一个步骤和大于 0 的循环次数；产物只能是物品。"; return false;
        }
        double largest = 0;
        for (const auto& e : d.outputs) if (!e.id.empty()) largest = (std::max)(largest, e.chance);
        call = L"event.recipes.create.sequenced_assembly(" + Array(d.outputs, true, largest) + L", " + Array(d.inputs, false).substr(1);
        // The assembly ingredient is singular, not an array.
        call.pop_back(); call += L", [\r\n";
        for (size_t i = 0; i < d.steps.size(); ++i) {
            const auto& step = d.steps[i];
            if (step.method != Pressing && step.method != Cutting && step.method != Deploying && step.method != Filling) {
                error = L"序列步骤只支持冲压、切削、使用和注液。"; return false;
            }
            if (!Check(step.secondary, false, false, error)) return false;
            std::vector<Entry> inputs = { d.transition };
            if (step.method == Deploying || step.method == Filling) {
                if (step.secondary.id.empty() || step.secondary.kind != (step.method == Filling ? RecipeFluid : RecipeItem) ||
                    (step.method == Deploying && step.secondary.count != 1)) {
                    error = L"请为机械手步骤选择一个工具/材料，为注液步骤选择流体。"; return false;
                }
                inputs.push_back(step.secondary);
            }
            call += L"    event.recipes.create." + std::wstring(GetInfo(step.method).function) + L"(" + Item(d.transition) + L", " + Array(inputs, false) + L")";
            if (step.method == Deploying && step.keepHeld) call += L".keepHeldItem()";
            if (step.method == Cutting && step.customTime) {
                if (step.time < 1 || step.time > 99999999) { error = L"步骤处理时间须大于 0。"; return false; }
                call += L".processingTime(" + std::to_wstring(step.time) + L")";
            }
            call += i + 1 < d.steps.size() ? L",\r\n" : L"\r\n";
        }
        call += L"  ]).transitionalItem(" + Quote(d.transition.id) + L").loops(" + std::to_wstring(d.loops) + L")";
    } else {
        if (!ii && !fi) { error = L"请至少选择一个原料。"; return false; }
        if (ii > info.itemsIn || io > info.itemsOut || fi > info.fluidsIn || fo > info.fluidsOut) {
            error = L"原料或产物数量超过此处理方法的限制。"; return false;
        }
        if ((method == Deploying && ii != 2) || (method == Filling && (ii != 1 || fi != 1)) ||
            (method == Emptying && (ii != 1 || io != 1 || fo != 1)) || (method == Compacting && io == 0)) {
            error = L"使用需要两个物品原料；注液需要物品和流体；分液需要物品和流体产物；塑形需要物品产物。"; return false;
        }
        call = L"event.recipes.create." + std::wstring(info.function) + L"(" + Array(d.outputs, true) + L", " + Array(d.inputs, false) + L")";
        if (info.heat) {
            if (d.heat < 0 || d.heat > 2) { error = L"请选择合法的加热条件。"; return false; }
            if (d.heat == 1) call += L".heated()";
            if (d.heat == 2) call += L".superheated()";
        }
        if (info.time && d.customTime) call += L".processingTime(" + std::to_wstring(d.time) + L")";
        if (method == Deploying && d.keepHeld) call += L".keepHeldItem()";
    }
    script = L"ServerEvents.recipes(event => {\r\n  // " + std::wstring(info.name) + L"\r\n  " + call + L"\r\n})\r\n";
    return true;
}
}
