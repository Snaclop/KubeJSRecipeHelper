#pragma once
#include <string>

namespace RecipeChanges {
enum class Method { RemoveInput, RemoveOutput, RemoveMod, RemoveId, ReplaceInput, ReplaceOutput };
struct Draft {
    std::wstring item, replacement, mod, recipeId;
};
bool Build(Method method, const Draft& draft, std::wstring& script, std::wstring& error);
}
