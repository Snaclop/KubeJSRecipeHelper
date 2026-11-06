#pragma once
#include "third_party/nlohmann/json.hpp"
#include <map>
#include "RecipeCatalog.h"
#include <string>
#include <vector>

namespace ModRecipe {
using Json = nlohmann::ordered_json;
struct Entry { std::wstring strId; int nCount = 1; double dChance = 100; };
struct Group {
    std::string strId;
    Json jsonLabel, jsonWhen;
    bool bFluid = false, bOutput = false, bAllowChance = false, bTotalByCount = false, bAllowTags = false;
    std::string strSide;
    int nMinimum = 0, nMaximum = 1, nInitial = 1, nMaxCount = 64, nMaxTotal = 1;
    std::vector<std::string> arrPositiveChanceVersions;
};
struct Field {
    std::string strId, strType;
    Json jsonLabel, jsonInitial, jsonChoices, jsonWhen;
    bool bRequired = true, bAllowCustom = false;
    std::string strCustomType;
    double dMinimum = 0, dMaximum = 99999999;
};
struct Recipe {
    std::string strId;
    Json jsonLabel, jsonOutput, jsonRequireAny;
    std::vector<std::string> arrVersions;
    std::vector<Group> arrGroups;
    std::vector<Field> arrFields;
};
struct Mod { std::string strId, strName; std::vector<Recipe> arrRecipes; };
struct Draft { std::map<std::string, std::vector<Entry>> mapGroups; Json jsonFields = Json::object(); };
struct Catalog { std::vector<Mod> arrMods; std::vector<std::wstring> arrErrors; };

std::wstring Wide(const std::string& strText);
std::string Utf8(const std::wstring& strText);
std::wstring Label(const Json& jsonLabel, const std::string& strLocale);
Mod Parse(const Json& jsonDefinition); // Throws with a configuration location on invalid definitions.
Catalog LoadDirectory(const std::wstring& strDirectory);
std::wstring Directory(); // addon beside the executable, independent of the working directory.
Draft MakeDraft(const Recipe& recipe);
bool Visible(const Json& jsonCondition, const Draft& draft);
bool Build(const Recipe& recipe, const std::string& strVersion, const Draft& draft,
    Json& jsonBody, std::wstring& strError, const std::string& strLocale = "en");
std::wstring Script(const Json& jsonBody);
}
