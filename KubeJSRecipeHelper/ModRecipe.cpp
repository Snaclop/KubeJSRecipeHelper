#define NOMINMAX
#include <Windows.h>
#include "ModRecipe.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>

namespace ModRecipe {
std::wstring Wide(const std::string& strText) {
    if (strText.empty()) return {};
    const int nSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, strText.data(), (int)strText.size(), nullptr, 0);
    if (!nSize) throw std::runtime_error("Invalid UTF-8 text");
    std::wstring strValue(nSize, 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, strText.data(), (int)strText.size(), &strValue[0], nSize);
    return strValue;
}
std::string Utf8(const std::wstring& strText) {
    if (strText.empty()) return {};
    const int nSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, strText.data(), (int)strText.size(), nullptr, 0, nullptr, nullptr);
    if (!nSize) throw std::runtime_error("Invalid Unicode text");
    std::string strValue(nSize, 0);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, strText.data(), (int)strText.size(), &strValue[0], nSize, nullptr, nullptr);
    return strValue;
}
std::wstring Label(const Json& jsonLabel, const std::string& strLocale) {
    if (jsonLabel.is_string()) return Wide(jsonLabel.get<std::string>());
    if (jsonLabel.is_object()) {
        for (const auto& strKey : {strLocale, std::string("en"), std::string("zh-CN"), std::string("zh-TW")})
            if (jsonLabel.contains(strKey) && jsonLabel[strKey].is_string()) return Wide(jsonLabel[strKey].get<std::string>());
    }
    return {};
}
namespace {
void Need(bool bCondition, const std::string& strMessage) { if (!bCondition) throw std::runtime_error(strMessage); }
bool Identifier(const std::string& strId) {
    if (strId.empty() || strId.size() > 128) return false;
    for (char chCharacter : strId) if (!(chCharacter >= 'a' && chCharacter <= 'z') && !(chCharacter >= '0' && chCharacter <= '9') && chCharacter != '_' && chCharacter != '-' && chCharacter != '.') return false;
    return true;
}
bool Resource(const std::string& strId) { return RecipeValidId(strId); }
bool Has(const std::vector<std::string>& arrValues, const std::string& strKey) { return std::find(arrValues.begin(), arrValues.end(), strKey) != arrValues.end(); }
void CheckLabel(const Json& jsonValue) { Need(!Label(jsonValue, "en").empty(), "label needs a nonempty string or localized text"); }
const Group& FindGroup(const Recipe& recipe, const std::string& strId) {
    for (const auto& group : recipe.arrGroups) if (group.strId == strId) return group;
    throw std::runtime_error("Unknown group: " + strId);
}
const Field& FindField(const Recipe& recipe, const std::string& strId) {
    for (const auto& field : recipe.arrFields) if (field.strId == strId) return field;
    throw std::runtime_error("Unknown field: " + strId);
}
void CheckCondition(const Json& jsonCondition, const Recipe& recipe) {
    if (jsonCondition.is_null()) return;
    Need(jsonCondition.is_object() && jsonCondition.size() == 2 && jsonCondition.contains("field") && jsonCondition.contains("equals"),
        "when must contain field and equals");
    const auto& field = FindField(recipe, jsonCondition.at("field").get<std::string>());
    Need(field.strType == "choice" || field.strType == "boolean", "when needs a choice or boolean field");
    if (field.strType == "boolean") Need(jsonCondition["equals"].is_boolean(), "Boolean conditions need true or false");
    else {
        bool bFound = false;
        for (const auto& jsonChoice : field.jsonChoices) if (jsonChoice["value"] == jsonCondition["equals"]) bFound = true;
        Need(bFound, "Condition does not match a declared choice");
    }
}
void CheckTemplate(const Json& jsonNode, const Recipe& recipe, const std::string& strVersion, bool bEntryContext = false, int nDepth = 0) {
    Need(nDepth < 64, "Output template is too deeply nested");
    if (jsonNode.is_array()) { for (const auto& jsonValue : jsonNode) CheckTemplate(jsonValue, recipe, strVersion, bEntryContext, nDepth + 1); return; }
    if (!jsonNode.is_object()) return;
    const std::set<std::string> setOperators = {"$ref", "$each", "$first", "$concat", "$switch", "$version", "$value"};
    const std::set<std::string> setModifiers = {"$when", "$scale", "$omitIf", "$omitEmpty", "$repeatCount", "$cases"};
    int nOperatorCount = 0;
    for (auto iterator = jsonNode.begin(); iterator != jsonNode.end(); ++iterator) {
        if (setOperators.count(iterator.key())) ++nOperatorCount;
        else Need(iterator.key().empty() || iterator.key()[0] != '$' || setModifiers.count(iterator.key()), "Unknown template operator: " + iterator.key());
    }
    // $value is the body of $each/$first, rather than a second operation.
    if ((jsonNode.contains("$each") || jsonNode.contains("$first")) && jsonNode.contains("$value")) --nOperatorCount;
    Need(nOperatorCount <= 1, "A template node has multiple operations");
    if (nOperatorCount) for (auto iterator = jsonNode.begin(); iterator != jsonNode.end(); ++iterator)
        Need(!iterator.key().empty() && iterator.key()[0] == '$', "Template operations cannot contain literal recipe keys");
    if (jsonNode.contains("$repeatCount")) Need(jsonNode.contains("$each") && jsonNode["$repeatCount"].is_boolean(), "$repeatCount needs $each and a boolean");
    if (jsonNode.contains("$cases")) Need(jsonNode.contains("$switch"), "$cases needs $switch");
    if (jsonNode.contains("$omitEmpty")) Need(jsonNode["$omitEmpty"].is_boolean(), "$omitEmpty must be a boolean");
    if (jsonNode.contains("$when")) CheckCondition(jsonNode["$when"], recipe);
    if (jsonNode.contains("$scale")) Need(jsonNode["$scale"].is_number() && std::isfinite(jsonNode["$scale"].get<double>()), "$scale must be finite");
    if (jsonNode.contains("$ref")) {
        const auto strReference = jsonNode["$ref"].get<std::string>();
        if (strReference.compare(0, 7, "params.") == 0) FindField(recipe, strReference.substr(7));
        else Need(strReference == "version" || (bEntryContext && (strReference == "entry.id" || strReference == "entry.count" || strReference == "entry.chance")), "Unknown reference: " + strReference);
    } else if (jsonNode.contains("$each") || jsonNode.contains("$first")) {
        const auto strKey = jsonNode.contains("$each") ? "$each" : "$first";
        const auto& group = FindGroup(recipe, jsonNode[strKey].get<std::string>());
        Need(!jsonNode.value("$repeatCount", false) || !group.bFluid, "Cannot repeat fluid entries by volume");
        CheckTemplate(jsonNode.at("$value"), recipe, strVersion, true, nDepth + 1);
    } else if (jsonNode.contains("$switch")) {
        const auto& field = FindField(recipe, jsonNode["$switch"].get<std::string>());
        Need(field.strType == "choice" && !field.bAllowCustom, "$switch needs a fixed choice field");
        Need(jsonNode.at("$cases").is_object(), "$cases must be an object");
        for (const auto& jsonChoice : field.jsonChoices) {
            Need(jsonChoice.at("value").is_string(), "$switch choices must be strings");
            Need(jsonNode["$cases"].contains(jsonChoice["value"].get<std::string>()), "$switch is missing a choice");
        }
        for (const auto& jsonValue : jsonNode["$cases"]) CheckTemplate(jsonValue, recipe, strVersion, bEntryContext, nDepth + 1);
    } else if (jsonNode.contains("$version")) {
        Need(jsonNode["$version"].is_object() && jsonNode["$version"].contains(strVersion), "Missing template for version: " + strVersion);
        CheckTemplate(jsonNode["$version"][strVersion], recipe, strVersion, bEntryContext, nDepth + 1);
    } else if (jsonNode.contains("$concat")) {
        Need(jsonNode["$concat"].is_array(), "$concat must contain an array");
        for (const auto& jsonValue : jsonNode["$concat"]) CheckTemplate(jsonValue, recipe, strVersion, bEntryContext, nDepth + 1);
    } else if (jsonNode.contains("$value")) CheckTemplate(jsonNode["$value"], recipe, strVersion, bEntryContext, nDepth + 1);
    else {
        Need(nOperatorCount == 0, "Invalid output template");
        for (auto iterator = jsonNode.begin(); iterator != jsonNode.end(); ++iterator)
            if (iterator.key().empty() || iterator.key()[0] != '$') CheckTemplate(iterator.value(), recipe, strVersion, bEntryContext, nDepth + 1);
    }
}
bool TagIngredient(Json& jsonValue, const std::string& strId, bool bFluid) {
    bool bConverted = false;
    if (jsonValue.is_array()) for (auto& jsonChild : jsonValue) bConverted = TagIngredient(jsonChild, strId, bFluid) || bConverted;
    if (!jsonValue.is_object()) return bConverted;
    for (auto& jsonChild : jsonValue) bConverted = TagIngredient(jsonChild, strId, bFluid) || bConverted;
    const std::string strKey = bFluid ? "fluid" : "item";
    if (jsonValue.contains(strKey) && jsonValue[strKey] == strId) {
        const bool bModernFluid = bFluid && jsonValue.value("type", std::string()) == "fluid_stack";
        jsonValue.erase(strKey);
        jsonValue[bFluid ? (bModernFluid ? "fluid_tag" : "fluidTag") : "tag"] = strId.substr(1);
        if (bModernFluid) jsonValue["type"] = "fluid_tag";
        bConverted = true;
    } else if (!bFluid && jsonValue.contains("tag") && jsonValue["tag"] == strId) { jsonValue["tag"] = strId.substr(1); bConverted = true; }
    return bConverted;
}
Json Render(const Json& jsonNode, const Recipe& recipe, const Draft& draft, const std::string& strVersion, const Entry* pEntry = nullptr, int nDepth = 0) {
    Need(nDepth < 64, "Output template is too deeply nested");
    if (jsonNode.is_array()) {
        Json jsonResult = Json::array();
        for (const auto& jsonValue : jsonNode) { auto jsonRendered = Render(jsonValue, recipe, draft, strVersion, pEntry, nDepth + 1); if (!jsonRendered.is_null()) jsonResult.push_back(jsonRendered); }
        return jsonResult;
    }
    if (!jsonNode.is_object()) return jsonNode;
    if (jsonNode.contains("$when") && !Visible(jsonNode["$when"], draft)) return nullptr;
    Json jsonResult;
    if (jsonNode.contains("$ref")) {
        const auto strReference = jsonNode["$ref"].get<std::string>();
        if (strReference == "version") jsonResult = strVersion;
        else if (strReference.compare(0, 7, "params.") == 0) {
            const auto strId = strReference.substr(7); jsonResult = draft.jsonFields.at(strId);
            const auto& field = FindField(recipe, strId);
            if ((field.strType == "tag" || (field.bAllowCustom && field.strCustomType == "tag")) && jsonResult.is_string()) {
                auto strTag = jsonResult.get<std::string>();
                if (!strTag.empty() && strTag[0] == '#') jsonResult = strTag.substr(1);
            }
        }
        else {
            Need(pEntry != nullptr, "Entry reference outside a group");
            if (strReference == "entry.id") jsonResult = Utf8(pEntry->strId);
            else if (strReference == "entry.count") jsonResult = pEntry->nCount;
            else if (strReference == "entry.chance") jsonResult = pEntry->dChance;
            else throw std::runtime_error("Unknown reference: " + strReference);
        }
    } else if (jsonNode.contains("$version")) jsonResult = Render(jsonNode["$version"].at(strVersion), recipe, draft, strVersion, pEntry, nDepth + 1);
    else if (jsonNode.contains("$each") || jsonNode.contains("$first")) {
        const bool bFirst = jsonNode.contains("$first");
        const auto strId = jsonNode[bFirst ? "$first" : "$each"].get<std::string>();
        const auto& group = FindGroup(recipe, strId);
        jsonResult = bFirst ? Json(nullptr) : Json::array();
        if (Visible(group.jsonWhen, draft)) for (const auto& entry : draft.mapGroups.at(strId)) {
            if (entry.strId.empty()) continue;
            auto jsonValue = Render(jsonNode.at("$value"), recipe, draft, strVersion, &entry, nDepth + 1);
            if (RecipeIsTag(entry.strId) && !jsonValue.is_null()) Need(group.bAllowTags && TagIngredient(jsonValue, Utf8(entry.strId), group.bFluid),
                "Tag-enabled groups need an item/fluid ingredient object: " + group.strId);
            if (bFirst) { jsonResult = jsonValue; break; }
            const int nRepeat = jsonNode.value("$repeatCount", false) ? entry.nCount : 1;
            Need(jsonResult.size() + nRepeat <= 16384, "Too many expanded ingredients");
            for (int nIndex = 0; nIndex < nRepeat; ++nIndex) if (!jsonValue.is_null()) jsonResult.push_back(jsonValue);
        }
    } else if (jsonNode.contains("$concat")) {
        jsonResult = Json::array();
        for (const auto& jsonPart : jsonNode["$concat"]) {
            auto arrValues = Render(jsonPart, recipe, draft, strVersion, pEntry, nDepth + 1);
            if (arrValues.is_null()) continue;
            Need(arrValues.is_array(), "$concat requires arrays");
            for (const auto& jsonValue : arrValues) jsonResult.push_back(jsonValue);
        }
    } else if (jsonNode.contains("$switch")) {
        const auto strKey = draft.jsonFields.at(jsonNode["$switch"].get<std::string>()).get<std::string>();
        jsonResult = Render(jsonNode.at("$cases").at(strKey), recipe, draft, strVersion, pEntry, nDepth + 1);
    } else if (jsonNode.contains("$value")) jsonResult = Render(jsonNode["$value"], recipe, draft, strVersion, pEntry, nDepth + 1);
    else {
        jsonResult = Json::object();
        for (auto iterator = jsonNode.begin(); iterator != jsonNode.end(); ++iterator) if (iterator.key().empty() || iterator.key()[0] != '$') {
            auto jsonValue = Render(iterator.value(), recipe, draft, strVersion, pEntry, nDepth + 1);
            if (!jsonValue.is_null()) jsonResult[iterator.key()] = jsonValue;
        }
    }
    if (jsonNode.contains("$scale")) {
        Need(jsonResult.is_number(), "$scale needs a number");
        double dScaled = jsonResult.get<double>() * jsonNode["$scale"].get<double>();
        Need(std::isfinite(dScaled), "$scale produced a non-finite number"); jsonResult = dScaled;
    }
    if (jsonNode.contains("$omitIf") && jsonResult == jsonNode["$omitIf"]) return nullptr;
    if (jsonNode.value("$omitEmpty", false) && jsonResult.empty()) return nullptr;
    return jsonResult;
}
std::wstring Message(const std::string& strLocale, const wchar_t* pEnglish, const wchar_t* pChinese, const wchar_t* pTraditionalChinese) {
    return strLocale == "zh-CN" ? pChinese : strLocale == "zh-TW" ? pTraditionalChinese : pEnglish;
}
bool FieldValid(const Field& field, const Json& jsonValue) {
    if (field.strType == "boolean") return jsonValue.is_boolean();
    if (field.strType == "integer" || field.strType == "number") {
        if (!jsonValue.is_number()) return false;
        const double dNumber = jsonValue.get<double>();
        return std::isfinite(dNumber) && dNumber >= field.dMinimum && dNumber <= field.dMaximum && (field.strType != "integer" || std::floor(dNumber) == dNumber);
    }
    if (field.strType == "choice") {
        for (const auto& jsonChoice : field.jsonChoices) if (jsonChoice["value"] == jsonValue) return true;
        if (!field.bAllowCustom || !jsonValue.is_string()) return false;
    } else if (!jsonValue.is_string()) return false;
    const auto strText = jsonValue.get<std::string>();
    if (strText.empty()) return !field.bRequired;
    const auto strType = field.strType == "choice" ? field.strCustomType : field.strType;
    if (strType == "resource") return Resource(strText);
    if (strType == "tag") return Resource(strText[0] == '#' ? strText.substr(1) : strText);
    return strText.size() <= 4096;
}
}

Mod Parse(const Json& jsonDefinition) {
    Need(jsonDefinition.is_object() && jsonDefinition.at("schemaVersion") == 1, "Unsupported schemaVersion (expected 1)");
    Mod mod; mod.strId = jsonDefinition.at("id").get<std::string>(); mod.strName = jsonDefinition.at("name").get<std::string>();
    Need(Identifier(mod.strId), "Invalid mod id");
    Need(!mod.strName.empty() && mod.strName.size() <= 256 && mod.strName.find_first_of("\r\n") == std::string::npos, "Invalid official mod name");
    Wide(mod.strName);
    const auto& arrRecipes = jsonDefinition.at("recipes");
    Need(arrRecipes.is_array() && !arrRecipes.empty() && arrRecipes.size() <= 128, "recipes needs 1-128 entries");
    std::set<std::string> setRecipeIds;
    for (const auto& jsonSource : arrRecipes) {
        Recipe recipe; recipe.strId = jsonSource.at("id").get<std::string>(); recipe.jsonLabel = jsonSource.at("label");
        Need(Identifier(recipe.strId) && setRecipeIds.insert(recipe.strId).second, "Invalid or duplicate recipe id: " + recipe.strId); CheckLabel(recipe.jsonLabel);
        recipe.arrVersions = jsonSource.at("versions").get<std::vector<std::string>>();
        Need(!recipe.arrVersions.empty() && recipe.arrVersions.size() <= 16, "versions needs 1-16 entries");
        std::set<std::string> setVersionIds;
        for (const auto& strVersion : recipe.arrVersions) Need(Identifier(strVersion) && setVersionIds.insert(strVersion).second, "Invalid or duplicate version: " + strVersion);
        const auto& arrGroups = jsonSource.at("groups");
        Need(arrGroups.is_array() && !arrGroups.empty() && arrGroups.size() <= 8, "groups needs 1-8 entries");
        std::set<std::string> setGroupIds; int nLeft = 0, nRight = 0;
        for (const auto& jsonGroup : arrGroups) {
            Group group; group.strId = jsonGroup.at("id").get<std::string>(); group.jsonLabel = jsonGroup.at("label"); CheckLabel(group.jsonLabel);
            Need(Identifier(group.strId) && setGroupIds.insert(group.strId).second, "Invalid or duplicate group: " + group.strId);
            const auto strKind = jsonGroup.value("kind", std::string("item")); Need(strKind == "item" || strKind == "fluid", "Unknown group kind: " + strKind);
            group.bFluid = strKind == "fluid"; group.bOutput = jsonGroup.value("output", false); group.strSide = jsonGroup.value("side", std::string(group.bOutput ? "output" : "input"));
            group.bAllowTags = jsonGroup.value("allowTags", false);
            Need(!group.bAllowTags || !group.bOutput, "Output groups cannot allow tags: " + group.strId);
            Need(group.strSide == "input" || group.strSide == "output", "Unknown group side");
            group.strSide == "input" ? ++nLeft : ++nRight;
            group.nMinimum = jsonGroup.value("min", 0); group.nMaximum = jsonGroup.value("max", 1); group.nInitial = jsonGroup.value("initial", 1);
            group.nMaxCount = jsonGroup.value("maxCount", group.bFluid ? 99999999 : 64); group.bTotalByCount = jsonGroup.value("totalByCount", false);
            group.nMaxTotal = jsonGroup.value("maxTotal", group.nMaximum); group.bAllowChance = jsonGroup.value("chance", false);
            group.arrPositiveChanceVersions = jsonGroup.value("positiveChanceVersions", std::vector<std::string>{});
            for (const auto& strVersion : group.arrPositiveChanceVersions) Need(Has(recipe.arrVersions, strVersion), "Unknown chance version: " + strVersion);
            Need(group.nMinimum >= 0 && group.nMinimum <= group.nMaximum && group.nMaximum <= 128 && group.nMaximum > 0 &&
                group.nInitial >= 1 && group.nInitial <= group.nMaximum && group.nMaxCount >= 1 && group.nMaxCount <= (group.bFluid ? 99999999 : 64) &&
                group.nMaxTotal >= group.nMinimum && group.nMaxTotal <= 99999999 && (!group.bFluid || !group.bAllowChance), "Invalid group limits: " + group.strId);
            group.jsonWhen = jsonGroup.value("when", Json()); recipe.arrGroups.push_back(group);
        }
        Need(nLeft <= 4 && nRight <= 4, "At most four groups per side are supported");
        const auto arrFields = jsonSource.value("fields", Json::array()); Need(arrFields.is_array() && arrFields.size() <= 32, "fields must be an array of at most 32 entries");
        std::set<std::string> setFieldIds;
        for (const auto& jsonField : arrFields) {
            Field field; field.strId = jsonField.at("id").get<std::string>(); field.strType = jsonField.at("type").get<std::string>();
            Need(Identifier(field.strId) && setFieldIds.insert(field.strId).second, "Invalid or duplicate field: " + field.strId);
            Need(Has({"integer", "number", "text", "resource", "tag", "boolean", "choice"}, field.strType), "Unknown field type: " + field.strType);
            field.jsonLabel = jsonField.at("label"); CheckLabel(field.jsonLabel); field.jsonInitial = jsonField.at("default");
            field.dMinimum = jsonField.value("min", 0.0); field.dMaximum = jsonField.value("max", 99999999.0);
            Need(std::isfinite(field.dMinimum) && std::isfinite(field.dMaximum) && field.dMinimum <= field.dMaximum, "Invalid field bounds");
            field.bRequired = jsonField.value("required", true); field.bAllowCustom = jsonField.value("allowCustom", false); field.strCustomType = jsonField.value("customType", std::string("text"));
            Need(Has({"text", "resource", "tag"}, field.strCustomType), "Unknown customType");
            field.jsonChoices = jsonField.value("choices", Json::array());
            if (field.strType == "choice") {
                Need(field.jsonChoices.is_array() && !field.jsonChoices.empty() && field.jsonChoices.size() <= 128, "choice needs 1-128 choices");
                std::vector<Json> arrValues;
                for (const auto& jsonChoice : field.jsonChoices) {
                    CheckLabel(jsonChoice.at("label")); const auto& jsonValue = jsonChoice.at("value");
                    Need(!jsonValue.is_structured() && !jsonValue.is_null() && std::find(arrValues.begin(), arrValues.end(), jsonValue) == arrValues.end(), "Invalid or duplicate choice value");
                    arrValues.push_back(jsonValue);
                }
            }
            Need(FieldValid(field, field.jsonInitial) || ((field.strType == "resource" || field.strType == "tag" || field.strType == "text") && field.jsonInitial.is_string() && field.jsonInitial.get<std::string>().empty()), "Invalid default for field: " + field.strId);
            field.jsonWhen = jsonField.value("when", Json()); recipe.arrFields.push_back(field);
        }
        for (const auto& field : recipe.arrFields) CheckCondition(field.jsonWhen, recipe);
        for (const auto& group : recipe.arrGroups) CheckCondition(group.jsonWhen, recipe);
        recipe.jsonRequireAny = jsonSource.value("requireAny", Json::array()); Need(recipe.jsonRequireAny.is_array(), "requireAny must be an array");
        for (const auto& jsonRule : recipe.jsonRequireAny) {
            Need(jsonRule.is_array() && !jsonRule.empty(), "requireAny rules need group IDs");
            for (const auto& strId : jsonRule) FindGroup(recipe, strId.get<std::string>());
        }
        recipe.jsonOutput = jsonSource.at("output"); Need(recipe.jsonOutput.is_object(), "output must be an object template");
        for (const auto& strVersion : recipe.arrVersions) CheckTemplate(recipe.jsonOutput, recipe, strVersion);
        mod.arrRecipes.push_back(std::move(recipe));
    }
    return mod;
}

std::wstring Directory() {
    std::wstring strPath(32768, 0); DWORD uLength = GetModuleFileNameW(nullptr, &strPath[0], (DWORD)strPath.size());
    if (!uLength || uLength >= strPath.size()) throw std::runtime_error("Cannot locate the executable directory");
    strPath.resize(uLength); return strPath.substr(0, strPath.find_last_of(L"\\/")) + L"\\addon";
}
Catalog LoadDirectory(const std::wstring& strDirectory) {
    Catalog catalog; std::vector<std::wstring> arrPaths; WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((strDirectory + L"\\*.json").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        catalog.arrErrors.push_back(strDirectory + L": " + std::to_wstring(GetLastError())); return catalog;
    }
    do { if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) arrPaths.push_back(strDirectory + L"\\" + findData.cFileName); } while (FindNextFileW(hFind, &findData));
    const DWORD uScanError = GetLastError(); FindClose(hFind);
    if (uScanError != ERROR_NO_MORE_FILES) catalog.arrErrors.push_back(strDirectory + L": " + std::to_wstring(uScanError));
    std::sort(arrPaths.begin(), arrPaths.end()); std::set<std::string> setIds;
    for (const auto& strPath : arrPaths) {
        try {
            std::ifstream inputFile(strPath.c_str(), std::ios::binary | std::ios::ate);
            Need(inputFile.good(), "Cannot open the file"); const auto nSize = inputFile.tellg();
            Need(nSize > 0 && nSize <= 2 * 1024 * 1024, "File must contain 1 byte to 2 MiB");
            inputFile.seekg(0); std::string strText((size_t)nSize, '\0'); inputFile.read(&strText[0], (std::streamsize)nSize); Need(inputFile.good(), "Cannot read the file");
            auto jsonDefinition = Json::parse(strText, [](int nDepth, Json::parse_event_t, Json&) { Need(nDepth < 64, "JSON is too deeply nested"); return true; });
            Mod mod = Parse(jsonDefinition); Need(setIds.insert(mod.strId).second, "Duplicate mod id: " + mod.strId);
            catalog.arrMods.push_back(std::move(mod));
        } catch (const std::exception& exception) {
            std::wstring strDetail;
            try { strDetail = Wide(exception.what()); }
            catch (const std::exception&) { strDetail = L"Invalid UTF-8 text in the JSON file."; }
            catalog.arrErrors.push_back(strPath + L"\r\n" + strDetail);
        }
    }
    return catalog;
}
Draft MakeDraft(const Recipe& recipe) {
    Draft draft; for (const auto& group : recipe.arrGroups) draft.mapGroups[group.strId].resize(group.nInitial);
    for (const auto& field : recipe.arrFields) draft.jsonFields[field.strId] = field.jsonInitial;
    return draft;
}
bool Visible(const Json& jsonCondition, const Draft& draft) {
    if (jsonCondition.is_null()) return true;
    const auto strKey = jsonCondition.at("field").get<std::string>();
    return draft.jsonFields.contains(strKey) && draft.jsonFields[strKey] == jsonCondition.at("equals");
}
bool Build(const Recipe& recipe, const std::string& strVersion, const Draft& draft, Json& jsonBody, std::wstring& strError, const std::string& strLocale) {
    jsonBody = nullptr; strError.clear();
    try {
        Need(Has(recipe.arrVersions, strVersion), "Unsupported recipe version: " + strVersion);
        for (const auto& field : recipe.arrFields) if (Visible(field.jsonWhen, draft)) {
            if (!draft.jsonFields.contains(field.strId) || !FieldValid(field, draft.jsonFields[field.strId])) {
                strError = Label(field.jsonLabel, strLocale) + L": " + Message(strLocale, L"Invalid value or outside the allowed range.", L"数值无效或超出允许范围。", L"數值無效或超出允許範圍。"); return false;
            }
        }
        std::map<std::string, int> mapCounts;
        for (const auto& group : recipe.arrGroups) {
            mapCounts[group.strId] = 0; if (!Visible(group.jsonWhen, draft)) continue;
            const auto& arrValues = draft.mapGroups.at(group.strId); int64_t nTotal = 0;
            Need(arrValues.size() <= (size_t)group.nMaximum, "Too many slots: " + group.strId);
            for (const auto& entry : arrValues) {
                if (entry.strId.empty()) continue;
                const bool bTag = RecipeIsTag(entry.strId);
                if ((bTag && !group.bAllowTags) || !Resource(Utf8(bTag ? entry.strId.substr(1) : entry.strId)) || entry.nCount < 1 || entry.nCount > group.nMaxCount || !std::isfinite(entry.dChance) ||
                    entry.dChance < 0 || entry.dChance > 100 || (!group.bAllowChance && entry.dChance != 100) || (Has(group.arrPositiveChanceVersions, strVersion) && entry.dChance == 0)) {
                    strError = Label(group.jsonLabel, strLocale) + L": " + Message(strLocale, L"Check the ID, count and chance.", L"请检查 ID、数量和概率。", L"請檢查 ID、數量和機率。"); return false;
                }
                ++mapCounts[group.strId]; nTotal += group.bTotalByCount ? entry.nCount : 1;
            }
            if (mapCounts[group.strId] < group.nMinimum || nTotal > group.nMaxTotal) {
                strError = Label(group.jsonLabel, strLocale) + L": " + Message(strLocale, L"The input/output count does not meet the recipe limits.", L"原料或产物数量不符合配方限制。", L"原料或產物數量不符合配方限制。"); return false;
            }
        }
        for (const auto& jsonRule : recipe.jsonRequireAny) {
            int nCount = 0; for (const auto& strId : jsonRule) nCount += mapCounts[strId.get<std::string>()];
            if (!nCount) { strError = Message(strLocale, L"Select at least one input and one output.", L"请至少选择一种原料和一种产物。", L"請至少選擇一種原料和一種產物。"); return false; }
        }
        jsonBody = Render(recipe.jsonOutput, recipe, draft, strVersion);
        Need(jsonBody.is_object() && jsonBody.contains("type") && jsonBody["type"].is_string() && Resource(jsonBody["type"].get<std::string>()), "Output needs a valid recipe type");
        return true;
    } catch (const std::exception& exception) { strError = Wide(exception.what()); jsonBody = nullptr; return false; }
}
std::wstring Script(const Json& jsonBody) {
    // JSON serialization quotes data; definitions never execute code in the helper.
    const auto strJson = jsonBody.dump(2, ' ', true, Json::error_handler_t::strict);
    std::string strText = "ServerEvents.recipes(event => {\n  event.custom(";
    for (char chCharacter : strJson) { strText += chCharacter; if (chCharacter == '\n') strText += "  "; }
    strText += ")\n})\n";
    std::wstring strResult;
    for (wchar_t chCharacter : Wide(strText)) { if (chCharacter == L'\n') strResult += L'\r'; strResult += chCharacter; }
    return strResult;
}
}
