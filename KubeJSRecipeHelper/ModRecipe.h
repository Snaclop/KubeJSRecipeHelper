#pragma once
#include "third_party/nlohmann/json.hpp"
#include <map>
#include <string>
#include <vector>

namespace ModRecipe {
using Json = nlohmann::ordered_json;
struct Entry { std::wstring id; int count = 1; double chance = 100; };
struct Group {
    std::string id;
    Json label, when;
    bool fluid = false, output = false, chance = false, totalByCount = false;
    std::string side;
    int minimum = 0, maximum = 1, initial = 1, maxCount = 64, maxTotal = 1;
    std::vector<std::string> positiveChanceVersions;
};
struct Field {
    std::string id, type;
    Json label, initial, choices, when;
    bool required = true, allowCustom = false;
    std::string customType;
    double minimum = 0, maximum = 99999999;
};
struct Recipe {
    std::string id;
    Json label, output, requireAny;
    std::vector<std::string> versions;
    std::vector<Group> groups;
    std::vector<Field> fields;
};
struct Mod { std::string id, name; std::vector<Recipe> recipes; };
struct Draft { std::map<std::string, std::vector<Entry>> groups; Json fields = Json::object(); };
struct Catalog { std::vector<Mod> mods; std::vector<std::wstring> errors; };

std::wstring Wide(const std::string& text);
std::string Utf8(const std::wstring& text);
std::wstring Label(const Json& label, const std::string& locale);
Mod Parse(const Json& definition); // Throws with a configuration location on invalid definitions.
Catalog LoadDirectory(const std::wstring& directory);
std::wstring Directory(); // addon beside the executable, independent of the working directory.
Draft MakeDraft(const Recipe& recipe);
bool Visible(const Json& condition, const Draft& draft);
bool Build(const Recipe& recipe, const std::string& version, const Draft& draft,
    Json& body, std::wstring& error, const std::string& locale = "en");
std::wstring Script(const Json& body);
}
