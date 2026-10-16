#define NOMINMAX
#include <Windows.h>
#include "ModRecipe.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>

namespace ModRecipe {
std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0);
    if (!size) throw std::runtime_error("Invalid UTF-8 text");
    std::wstring value(size, 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), &value[0], size);
    return value;
}
std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("Invalid Unicode text");
    std::string value(size, 0);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), (int)text.size(), &value[0], size, nullptr, nullptr);
    return value;
}
std::wstring Label(const Json& label, const std::string& locale) {
    if (label.is_string()) return Wide(label.get<std::string>());
    if (label.is_object()) {
        for (const auto& key : {locale, std::string("en"), std::string("zh-CN"), std::string("zh-TW")})
            if (label.contains(key) && label[key].is_string()) return Wide(label[key].get<std::string>());
    }
    return {};
}
namespace {
void Need(bool condition, const std::string& message) { if (!condition) throw std::runtime_error(message); }
bool Identifier(const std::string& id) {
    if (id.empty() || id.size() > 128) return false;
    for (char c : id) if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '_' && c != '-' && c != '.') return false;
    return true;
}
bool Resource(const std::string& id) {
    auto colon = id.find(':');
    if (colon == std::string::npos || !Identifier(id.substr(0, colon)) || colon + 1 == id.size()) return false;
    for (size_t i = colon + 1; i < id.size(); ++i) {
        char c = id[i];
        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '_' && c != '-' && c != '.' && c != '/') return false;
    }
    return true;
}
bool Has(const std::vector<std::string>& values, const std::string& key) { return std::find(values.begin(), values.end(), key) != values.end(); }
void CheckLabel(const Json& value) { Need(!Label(value, "en").empty(), "label needs a nonempty string or localized text"); }
const Group& FindGroup(const Recipe& r, const std::string& id) {
    for (const auto& group : r.groups) if (group.id == id) return group;
    throw std::runtime_error("Unknown group: " + id);
}
const Field& FindField(const Recipe& r, const std::string& id) {
    for (const auto& field : r.fields) if (field.id == id) return field;
    throw std::runtime_error("Unknown field: " + id);
}
void CheckCondition(const Json& condition, const Recipe& r) {
    if (condition.is_null()) return;
    Need(condition.is_object() && condition.size() == 2 && condition.contains("field") && condition.contains("equals"),
        "when must contain field and equals");
    const auto& field = FindField(r, condition.at("field").get<std::string>());
    Need(field.type == "choice" || field.type == "boolean", "when needs a choice or boolean field");
    if (field.type == "boolean") Need(condition["equals"].is_boolean(), "Boolean conditions need true or false");
    else {
        bool found = false;
        for (const auto& choice : field.choices) if (choice["value"] == condition["equals"]) found = true;
        Need(found, "Condition does not match a declared choice");
    }
}
void CheckTemplate(const Json& node, const Recipe& r, const std::string& version, bool entry = false, int depth = 0) {
    Need(depth < 64, "Output template is too deeply nested");
    if (node.is_array()) { for (const auto& value : node) CheckTemplate(value, r, version, entry, depth + 1); return; }
    if (!node.is_object()) return;
    const std::set<std::string> operators = {"$ref", "$each", "$first", "$concat", "$switch", "$version", "$value"};
    const std::set<std::string> modifiers = {"$when", "$scale", "$omitIf", "$omitEmpty", "$repeatCount", "$cases"};
    int operatorCount = 0;
    for (auto it = node.begin(); it != node.end(); ++it) {
        if (operators.count(it.key())) ++operatorCount;
        else Need(it.key().empty() || it.key()[0] != '$' || modifiers.count(it.key()), "Unknown template operator: " + it.key());
    }
    // $value is the body of $each/$first, rather than a second operation.
    if ((node.contains("$each") || node.contains("$first")) && node.contains("$value")) --operatorCount;
    Need(operatorCount <= 1, "A template node has multiple operations");
    if (operatorCount) for (auto it = node.begin(); it != node.end(); ++it)
        Need(!it.key().empty() && it.key()[0] == '$', "Template operations cannot contain literal recipe keys");
    if (node.contains("$repeatCount")) Need(node.contains("$each") && node["$repeatCount"].is_boolean(), "$repeatCount needs $each and a boolean");
    if (node.contains("$cases")) Need(node.contains("$switch"), "$cases needs $switch");
    if (node.contains("$omitEmpty")) Need(node["$omitEmpty"].is_boolean(), "$omitEmpty must be a boolean");
    if (node.contains("$when")) CheckCondition(node["$when"], r);
    if (node.contains("$scale")) Need(node["$scale"].is_number() && std::isfinite(node["$scale"].get<double>()), "$scale must be finite");
    if (node.contains("$ref")) {
        const auto ref = node["$ref"].get<std::string>();
        if (ref.compare(0, 7, "params.") == 0) FindField(r, ref.substr(7));
        else Need(ref == "version" || (entry && (ref == "entry.id" || ref == "entry.count" || ref == "entry.chance")), "Unknown reference: " + ref);
    } else if (node.contains("$each") || node.contains("$first")) {
        const auto key = node.contains("$each") ? "$each" : "$first";
        const auto& group = FindGroup(r, node[key].get<std::string>());
        Need(!node.value("$repeatCount", false) || !group.fluid, "Cannot repeat fluid entries by volume");
        CheckTemplate(node.at("$value"), r, version, true, depth + 1);
    } else if (node.contains("$switch")) {
        const auto& field = FindField(r, node["$switch"].get<std::string>());
        Need(field.type == "choice" && !field.allowCustom, "$switch needs a fixed choice field");
        Need(node.at("$cases").is_object(), "$cases must be an object");
        for (const auto& choice : field.choices) {
            Need(choice.at("value").is_string(), "$switch choices must be strings");
            Need(node["$cases"].contains(choice["value"].get<std::string>()), "$switch is missing a choice");
        }
        for (const auto& value : node["$cases"]) CheckTemplate(value, r, version, entry, depth + 1);
    } else if (node.contains("$version")) {
        Need(node["$version"].is_object() && node["$version"].contains(version), "Missing template for version: " + version);
        CheckTemplate(node["$version"][version], r, version, entry, depth + 1);
    } else if (node.contains("$concat")) {
        Need(node["$concat"].is_array(), "$concat must contain an array");
        for (const auto& value : node["$concat"]) CheckTemplate(value, r, version, entry, depth + 1);
    } else if (node.contains("$value")) CheckTemplate(node["$value"], r, version, entry, depth + 1);
    else {
        Need(operatorCount == 0, "Invalid output template");
        for (auto it = node.begin(); it != node.end(); ++it)
            if (it.key().empty() || it.key()[0] != '$') CheckTemplate(it.value(), r, version, entry, depth + 1);
    }
}
Json Render(const Json& node, const Recipe& r, const Draft& d, const std::string& version, const Entry* entry = nullptr, int depth = 0) {
    Need(depth < 64, "Output template is too deeply nested");
    if (node.is_array()) {
        Json result = Json::array();
        for (const auto& value : node) { auto v = Render(value, r, d, version, entry, depth + 1); if (!v.is_null()) result.push_back(v); }
        return result;
    }
    if (!node.is_object()) return node;
    if (node.contains("$when") && !Visible(node["$when"], d, version)) return nullptr;
    Json result;
    if (node.contains("$ref")) {
        const auto ref = node["$ref"].get<std::string>();
        if (ref == "version") result = version;
        else if (ref.compare(0, 7, "params.") == 0) {
            const auto id = ref.substr(7); result = d.fields.at(id);
            const auto& field = FindField(r, id);
            if ((field.type == "tag" || (field.allowCustom && field.customType == "tag")) && result.is_string()) {
                auto tag = result.get<std::string>();
                if (!tag.empty() && tag[0] == '#') result = tag.substr(1);
            }
        }
        else {
            Need(entry != nullptr, "Entry reference outside a group");
            if (ref == "entry.id") result = Utf8(entry->id);
            else if (ref == "entry.count") result = entry->count;
            else if (ref == "entry.chance") result = entry->chance;
            else throw std::runtime_error("Unknown reference: " + ref);
        }
    } else if (node.contains("$version")) result = Render(node["$version"].at(version), r, d, version, entry, depth + 1);
    else if (node.contains("$each") || node.contains("$first")) {
        const bool first = node.contains("$first");
        const auto id = node[first ? "$first" : "$each"].get<std::string>();
        const auto& group = FindGroup(r, id);
        result = first ? Json(nullptr) : Json::array();
        if (Visible(group.when, d, version)) for (const auto& e : d.groups.at(id)) {
            if (e.id.empty()) continue;
            auto value = Render(node.at("$value"), r, d, version, &e, depth + 1);
            if (first) { result = value; break; }
            const int repeat = node.value("$repeatCount", false) ? e.count : 1;
            Need(result.size() + repeat <= 16384, "Too many expanded ingredients");
            for (int i = 0; i < repeat; ++i) if (!value.is_null()) result.push_back(value);
        }
    } else if (node.contains("$concat")) {
        result = Json::array();
        for (const auto& part : node["$concat"]) {
            auto values = Render(part, r, d, version, entry, depth + 1);
            Need(values.is_array(), "$concat requires arrays");
            for (const auto& value : values) result.push_back(value);
        }
    } else if (node.contains("$switch")) {
        const auto key = d.fields.at(node["$switch"].get<std::string>()).get<std::string>();
        result = Render(node.at("$cases").at(key), r, d, version, entry, depth + 1);
    } else if (node.contains("$value")) result = Render(node["$value"], r, d, version, entry, depth + 1);
    else {
        result = Json::object();
        for (auto it = node.begin(); it != node.end(); ++it) if (it.key().empty() || it.key()[0] != '$') {
            auto value = Render(it.value(), r, d, version, entry, depth + 1);
            if (!value.is_null()) result[it.key()] = value;
        }
    }
    if (node.contains("$scale")) {
        Need(result.is_number(), "$scale needs a number");
        double scaled = result.get<double>() * node["$scale"].get<double>();
        Need(std::isfinite(scaled), "$scale produced a non-finite number"); result = scaled;
    }
    if (node.contains("$omitIf") && result == node["$omitIf"]) return nullptr;
    if (node.value("$omitEmpty", false) && result.empty()) return nullptr;
    return result;
}
std::wstring Message(const std::string& locale, const wchar_t* en, const wchar_t* zh, const wchar_t* tw) {
    return locale == "zh-CN" ? zh : locale == "zh-TW" ? tw : en;
}
bool FieldValid(const Field& f, const Json& value) {
    if (f.type == "boolean") return value.is_boolean();
    if (f.type == "integer" || f.type == "number") {
        if (!value.is_number()) return false;
        const double n = value.get<double>();
        return std::isfinite(n) && n >= f.minimum && n <= f.maximum && (f.type != "integer" || std::floor(n) == n);
    }
    if (f.type == "choice") {
        for (const auto& choice : f.choices) if (choice["value"] == value) return true;
        if (!f.allowCustom || !value.is_string()) return false;
    } else if (!value.is_string()) return false;
    const auto text = value.get<std::string>();
    if (text.empty()) return !f.required;
    const auto type = f.type == "choice" ? f.customType : f.type;
    if (type == "resource") return Resource(text);
    if (type == "tag") return Resource(text[0] == '#' ? text.substr(1) : text);
    return text.size() <= 4096;
}
}

Mod Parse(const Json& definition) {
    Need(definition.is_object() && definition.at("schemaVersion") == 1, "Unsupported schemaVersion (expected 1)");
    Mod mod; mod.id = definition.at("id").get<std::string>(); mod.name = definition.at("name").get<std::string>();
    Need(Identifier(mod.id), "Invalid mod id");
    Need(!mod.name.empty() && mod.name.size() <= 256 && mod.name.find_first_of("\r\n") == std::string::npos, "Invalid official mod name");
    Wide(mod.name);
    const auto& recipes = definition.at("recipes");
    Need(recipes.is_array() && !recipes.empty() && recipes.size() <= 128, "recipes needs 1-128 entries");
    std::set<std::string> recipeIds;
    for (const auto& source : recipes) {
        Recipe r; r.id = source.at("id").get<std::string>(); r.label = source.at("label");
        Need(Identifier(r.id) && recipeIds.insert(r.id).second, "Invalid or duplicate recipe id: " + r.id); CheckLabel(r.label);
        r.versions = source.at("versions").get<std::vector<std::string>>();
        Need(!r.versions.empty() && r.versions.size() <= 16, "versions needs 1-16 entries");
        std::set<std::string> versionIds;
        for (const auto& v : r.versions) Need(Identifier(v) && versionIds.insert(v).second, "Invalid or duplicate version: " + v);
        const auto& groups = source.at("groups");
        Need(groups.is_array() && !groups.empty() && groups.size() <= 8, "groups needs 1-8 entries");
        std::set<std::string> groupIds; int left = 0, right = 0;
        for (const auto& g : groups) {
            Group group; group.id = g.at("id").get<std::string>(); group.label = g.at("label"); CheckLabel(group.label);
            Need(Identifier(group.id) && groupIds.insert(group.id).second, "Invalid or duplicate group: " + group.id);
            const auto kind = g.value("kind", std::string("item")); Need(kind == "item" || kind == "fluid", "Unknown group kind: " + kind);
            group.fluid = kind == "fluid"; group.output = g.value("output", false); group.side = g.value("side", std::string(group.output ? "output" : "input"));
            Need(group.side == "input" || group.side == "output", "Unknown group side");
            group.side == "input" ? ++left : ++right;
            group.minimum = g.value("min", 0); group.maximum = g.value("max", 1); group.initial = g.value("initial", 1);
            group.maxCount = g.value("maxCount", group.fluid ? 99999999 : 64); group.totalByCount = g.value("totalByCount", false);
            group.maxTotal = g.value("maxTotal", group.maximum); group.chance = g.value("chance", false);
            group.positiveChanceVersions = g.value("positiveChanceVersions", std::vector<std::string>{});
            for (const auto& v : group.positiveChanceVersions) Need(Has(r.versions, v), "Unknown chance version: " + v);
            Need(group.minimum >= 0 && group.minimum <= group.maximum && group.maximum <= 128 && group.maximum > 0 &&
                group.initial >= 1 && group.initial <= group.maximum && group.maxCount >= 1 && group.maxCount <= (group.fluid ? 99999999 : 64) &&
                group.maxTotal >= group.minimum && group.maxTotal <= 99999999 && (!group.fluid || !group.chance), "Invalid group limits: " + group.id);
            group.when = g.value("when", Json()); r.groups.push_back(group);
        }
        Need(left <= 4 && right <= 4, "At most four groups per side are supported");
        const auto fields = source.value("fields", Json::array()); Need(fields.is_array() && fields.size() <= 32, "fields must be an array of at most 32 entries");
        std::set<std::string> fieldIds;
        for (const auto& f : fields) {
            Field field; field.id = f.at("id").get<std::string>(); field.type = f.at("type").get<std::string>();
            Need(Identifier(field.id) && fieldIds.insert(field.id).second, "Invalid or duplicate field: " + field.id);
            Need(Has({"integer", "number", "text", "resource", "tag", "boolean", "choice"}, field.type), "Unknown field type: " + field.type);
            field.label = f.at("label"); CheckLabel(field.label); field.initial = f.at("default");
            field.minimum = f.value("min", 0.0); field.maximum = f.value("max", 99999999.0);
            Need(std::isfinite(field.minimum) && std::isfinite(field.maximum) && field.minimum <= field.maximum, "Invalid field bounds");
            field.required = f.value("required", true); field.allowCustom = f.value("allowCustom", false); field.customType = f.value("customType", std::string("text"));
            Need(Has({"text", "resource", "tag"}, field.customType), "Unknown customType");
            field.choices = f.value("choices", Json::array());
            if (field.type == "choice") {
                Need(field.choices.is_array() && !field.choices.empty() && field.choices.size() <= 128, "choice needs 1-128 choices");
                std::vector<Json> values;
                for (const auto& choice : field.choices) {
                    CheckLabel(choice.at("label")); const auto& value = choice.at("value");
                    Need(!value.is_structured() && !value.is_null() && std::find(values.begin(), values.end(), value) == values.end(), "Invalid or duplicate choice value");
                    values.push_back(value);
                }
            }
            Need(FieldValid(field, field.initial) || ((field.type == "resource" || field.type == "tag" || field.type == "text") && field.initial.is_string() && field.initial.get<std::string>().empty()), "Invalid default for field: " + field.id);
            field.when = f.value("when", Json()); r.fields.push_back(field);
        }
        for (const auto& field : r.fields) CheckCondition(field.when, r);
        for (const auto& group : r.groups) CheckCondition(group.when, r);
        r.requireAny = source.value("requireAny", Json::array()); Need(r.requireAny.is_array(), "requireAny must be an array");
        for (const auto& rule : r.requireAny) {
            Need(rule.is_array() && !rule.empty(), "requireAny rules need group IDs");
            for (const auto& id : rule) FindGroup(r, id.get<std::string>());
        }
        r.output = source.at("output"); Need(r.output.is_object(), "output must be an object template");
        for (const auto& version : r.versions) CheckTemplate(r.output, r, version);
        mod.recipes.push_back(std::move(r));
    }
    return mod;
}

std::wstring Directory() {
    std::wstring path(32768, 0); DWORD length = GetModuleFileNameW(nullptr, &path[0], (DWORD)path.size());
    if (!length || length >= path.size()) throw std::runtime_error("Cannot locate the executable directory");
    path.resize(length); return path.substr(0, path.find_last_of(L"\\/")) + L"\\addon";
}
Catalog LoadDirectory(const std::wstring& directory) {
    Catalog catalog; std::vector<std::wstring> paths; WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW((directory + L"\\*.json").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        catalog.errors.push_back(directory + L": " + std::to_wstring(GetLastError())); return catalog;
    }
    do { if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) paths.push_back(directory + L"\\" + data.cFileName); } while (FindNextFileW(find, &data));
    const DWORD scanError = GetLastError(); FindClose(find);
    if (scanError != ERROR_NO_MORE_FILES) catalog.errors.push_back(directory + L": " + std::to_wstring(scanError));
    std::sort(paths.begin(), paths.end()); std::set<std::string> ids;
    for (const auto& path : paths) {
        try {
            std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
            Need(file.good(), "Cannot open the file"); const auto size = file.tellg();
            Need(size > 0 && size <= 2 * 1024 * 1024, "File must contain 1 byte to 2 MiB");
            file.seekg(0); std::string text((size_t)size, '\0'); file.read(&text[0], (std::streamsize)size); Need(file.good(), "Cannot read the file");
            auto json = Json::parse(text, [](int depth, Json::parse_event_t, Json&) { Need(depth < 64, "JSON is too deeply nested"); return true; });
            Mod mod = Parse(json); Need(ids.insert(mod.id).second, "Duplicate mod id: " + mod.id);
            catalog.mods.push_back(std::move(mod));
        } catch (const std::exception& e) {
            std::wstring detail;
            try { detail = Wide(e.what()); }
            catch (const std::exception&) { detail = L"Invalid UTF-8 text in the JSON file."; }
            catalog.errors.push_back(path + L"\r\n" + detail);
        }
    }
    return catalog;
}
Draft MakeDraft(const Recipe& r) {
    Draft d; for (const auto& group : r.groups) d.groups[group.id].resize(group.initial);
    for (const auto& field : r.fields) d.fields[field.id] = field.initial;
    return d;
}
bool Visible(const Json& condition, const Draft& draft, const std::string&) {
    if (condition.is_null()) return true;
    const auto key = condition.at("field").get<std::string>();
    return draft.fields.contains(key) && draft.fields[key] == condition.at("equals");
}
bool Build(const Recipe& r, const std::string& version, const Draft& d, Json& body, std::wstring& error, const std::string& locale) {
    body = nullptr; error.clear();
    try {
        Need(Has(r.versions, version), "Unsupported recipe version: " + version);
        for (const auto& f : r.fields) if (Visible(f.when, d, version)) {
            if (!d.fields.contains(f.id) || !FieldValid(f, d.fields[f.id])) {
                error = Label(f.label, locale) + L": " + Message(locale, L"Invalid value or outside the allowed range.", L"数值无效或超出允许范围。", L"數值無效或超出允許範圍。"); return false;
            }
        }
        std::map<std::string, int> counts;
        for (const auto& g : r.groups) {
            counts[g.id] = 0; if (!Visible(g.when, d, version)) continue;
            const auto& values = d.groups.at(g.id); int64_t total = 0;
            Need(values.size() <= (size_t)g.maximum, "Too many slots: " + g.id);
            for (const auto& e : values) {
                if (e.id.empty()) continue;
                if (!Resource(Utf8(e.id)) || e.count < 1 || e.count > g.maxCount || !std::isfinite(e.chance) ||
                    e.chance < 0 || e.chance > 100 || (!g.chance && e.chance != 100) || (Has(g.positiveChanceVersions, version) && e.chance == 0)) {
                    error = Label(g.label, locale) + L": " + Message(locale, L"Check the ID, count and chance.", L"请检查 ID、数量和概率。", L"請檢查 ID、數量和機率。"); return false;
                }
                ++counts[g.id]; total += g.totalByCount ? e.count : 1;
            }
            if (counts[g.id] < g.minimum || total > g.maxTotal) {
                error = Label(g.label, locale) + L": " + Message(locale, L"The input/output count does not meet the recipe limits.", L"原料或产物数量不符合配方限制。", L"原料或產物數量不符合配方限制。"); return false;
            }
        }
        for (const auto& rule : r.requireAny) {
            int count = 0; for (const auto& id : rule) count += counts[id.get<std::string>()];
            if (!count) { error = Message(locale, L"Select at least one input and one output.", L"请至少选择一种原料和一种产物。", L"請至少選擇一種原料和一種產物。"); return false; }
        }
        body = Render(r.output, r, d, version);
        Need(body.is_object() && body.contains("type") && body["type"].is_string() && Resource(body["type"].get<std::string>()), "Output needs a valid recipe type");
        return true;
    } catch (const std::exception& e) { error = Wide(e.what()); body = nullptr; return false; }
}
std::wstring Script(const Json& body) {
    // JSON serialization quotes data; definitions never execute code in the helper.
    const auto json = body.dump(2, ' ', true, Json::error_handler_t::strict);
    std::string text = "ServerEvents.recipes(event => {\n  event.custom(";
    for (char c : json) { text += c; if (c == '\n') text += "  "; }
    text += ")\n})\n";
    std::wstring result;
    for (wchar_t c : Wide(text)) { if (c == L'\n') result += L'\r'; result += c; }
    return result;
}
}
