#pragma once
#include <string>
#include "RecipeCatalog.h"

namespace RecipeChanges {
enum class Method { RemoveInput, RemoveOutput, RemoveMod, RemoveId, ReplaceInput, ReplaceOutput };
struct Draft { std::wstring strItem, strReplacement, strMod, strRecipeId; };
bool Build(Method eMethod,const Draft& draft,std::wstring& strScript,std::wstring& strError);
}
