#include "RecipeChanges.h"

namespace RecipeChanges {
static bool Namespace(const std::wstring& text) {
    if(text.empty()) return false;
    for(auto c:text) if(!((c>=L'a' && c<=L'z') || (c>=L'0' && c<=L'9') || c==L'_' || c==L'-' || c==L'.')) return false;
    return true;
}
static bool Id(const std::wstring& text) {
    auto colon=text.find(L':');
    if(colon==std::wstring::npos || !Namespace(text.substr(0,colon)) || colon+1==text.size()) return false;
    for(size_t i=colon+1;i<text.size();++i) {
        auto c=text[i]; if(!((c>=L'a' && c<=L'z') || (c>=L'0' && c<=L'9') || c==L'_' || c==L'-' || c==L'.' || c==L'/')) return false;
    }
    return true;
}
static std::wstring Q(const std::wstring& text) { return L"'"+text+L"'"; } // Only validated IDs are quoted.
bool Build(Method method,const Draft& d,std::wstring& script,std::wstring& error) {
    script.clear(); error.clear();
    if(method<Method::RemoveInput || method>Method::ReplaceOutput) { error=L"请选择有效的操作方式。"; return false; }
    const bool replace=method==Method::ReplaceInput || method==Method::ReplaceOutput;
    const bool input=method==Method::RemoveInput || method==Method::ReplaceInput;
    std::wstring filter,call;
    if(method==Method::RemoveMod) {
        if(!Namespace(d.mod)) { error=L"请填写有效的模组 ID（仅命名空间，例如 farmersdelight）。"; return false; }
        filter=L"{ mod: "+Q(d.mod)+L" }";
    } else if(method==Method::RemoveId) {
        if(!Id(d.recipeId)) { error=L"请填写有效的配方 ID，例如 minecraft:iron_ingot_from_smelting_raw_iron。"; return false; }
        filter=L"{ id: "+Q(d.recipeId)+L" }";
    } else {
        if(!Id(d.item)) { error=L"请选择要匹配的物品。"; return false; }
        filter=L"{ "+std::wstring(input ? L"input" : L"output")+L": "+Q(d.item);
        if(replace && !d.recipeId.empty()) {
            if(!Id(d.recipeId)) { error=L"限定配方 ID 须为有效的命名空间:名称，或留空。"; return false; }
            filter+=L", id: "+Q(d.recipeId);
        }
        filter+=L" }";
    }
    if(replace) {
        if(!Id(d.replacement)) { error=L"请选择替换后的物品。"; return false; }
        if(d.item==d.replacement) { error=L"替换前后的物品相同，请选择不同的物品。"; return false; }
        call=std::wstring(input ? L"replaceInput" : L"replaceOutput")+L"("+filter+L", "+Q(d.item)+L", ";
        call+=input ? L"Ingredient.of("+Q(d.replacement)+L")" : Q(d.replacement);
        call+=L")";
    } else call=L"remove("+filter+L")";
    script=L"ServerEvents.recipes(event => {\r\n  event."+call+L"\r\n})\r\n";
    return true;
}
}
