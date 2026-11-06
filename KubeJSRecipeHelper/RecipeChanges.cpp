#include "RecipeChanges.h"

namespace RecipeChanges {
static std::wstring Q(const std::wstring& strText) { return L"'"+strText+L"'"; } // Only validated IDs are quoted.
bool Build(Method eMethod,const Draft& draft,std::wstring& strScript,std::wstring& strError) {
    strScript.clear(); strError.clear();
    if(eMethod<Method::RemoveInput || eMethod>Method::ReplaceOutput) { strError=L"请选择有效的操作方式。"; return false; }
    const bool bReplace=eMethod==Method::ReplaceInput || eMethod==Method::ReplaceOutput;
    const bool bInput=eMethod==Method::RemoveInput || eMethod==Method::ReplaceInput;
    std::wstring strFilter,strCall;
    if(eMethod==Method::RemoveMod) {
        if(!RecipeValidNamespace(draft.strMod)) { strError=L"请填写有效的模组 ID（仅命名空间，例如 farmersdelight）。"; return false; }
        strFilter=L"{ mod: "+Q(draft.strMod)+L" }";
    } else if(eMethod==Method::RemoveId) {
        if(!RecipeValidId(draft.strRecipeId)) { strError=L"请填写有效的配方 ID，例如 minecraft:iron_ingot_from_smelting_raw_iron。"; return false; }
        strFilter=L"{ id: "+Q(draft.strRecipeId)+L" }";
    } else {
        if(!RecipeValidId(draft.strItem,true)) { strError=L"请选择要匹配的物品。"; return false; }
        strFilter=L"{ "+std::wstring(bInput ? L"input" : L"output")+L": "+Q(draft.strItem);
        if(bReplace && !draft.strRecipeId.empty()) {
            if(!RecipeValidId(draft.strRecipeId)) { strError=L"限定配方 ID 须为有效的命名空间:名称，或留空。"; return false; }
            strFilter+=L", id: "+Q(draft.strRecipeId);
        }
        strFilter+=L" }";
    }
    if(bReplace) {
        if(!RecipeValidId(draft.strReplacement,bInput)) { strError=L"请选择替换后的物品。"; return false; }
        if(draft.strItem==draft.strReplacement) { strError=L"替换前后的物品相同，请选择不同的物品。"; return false; }
        strCall=std::wstring(bInput ? L"replaceInput" : L"replaceOutput")+L"("+strFilter+L", "+Q(draft.strItem)+L", ";
        strCall+=bInput ? L"Ingredient.of("+Q(draft.strReplacement)+L")" : Q(draft.strReplacement);
        strCall+=L")";
    } else strCall=L"remove("+strFilter+L")";
    strScript=L"ServerEvents.recipes(event => {\r\n  event."+strCall+L"\r\n})\r\n";
    return true;
}
}
