#include "FarmersDelightRecipe.h"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace FarmersDelightRecipe {
static std::wstring Quote(const std::wstring& text) {
    std::wstring result=L"\"";
    for(wchar_t c:text) {
        switch(c) {
        case L'"': result+=L"\\\""; break;
        case L'\\': result+=L"\\\\"; break;
        case L'\n': result+=L"\\n"; break;
        case L'\r': result+=L"\\r"; break;
        case L'\t': result+=L"\\t"; break;
        default:
            if(c<32) { std::wostringstream hex; hex<<L"\\u"<<std::hex<<std::setw(4)<<std::setfill(L'0')<<(unsigned)c; result+=hex.str(); }
            else result+=c;
        }
    }
    return result+L"\"";
}
static std::wstring Number(double value) {
    std::wostringstream out; out.imbue(std::locale::classic()); out<<std::setprecision(15)<<value; return out.str();
}
static bool ResourceId(const std::wstring& id) {
    const auto colon=id.find(L':');
    if(colon==std::wstring::npos || !colon || colon+1==id.size() || id.find(L':',colon+1)!=std::wstring::npos) return false;
    for(size_t i=0;i<id.size();++i) {
        wchar_t c=id[i]; if(i==colon) continue;
        if((c>=L'a' && c<=L'z') || (c>=L'0' && c<=L'9') || c==L'_' || c==L'-' || c==L'.' || (i>colon && c==L'/')) continue;
        return false;
    }
    return true;
}
static bool Check(const Entry& e, bool output, std::wstring& error) {
    if(e.id.empty()) return true;
    if(e.kind!=RecipeItem || !ResourceId(e.id)) { error=L"农夫乐事配方只能选择物品，物品 ID 须为有效的命名空间:名称。"; return false; }
    if(e.count<1 || e.count>(output ? 64 : 1)) { error=output ? L"产物数量须为 1–64。" : L"每个原料、工具或容器格只能放一个物品。"; return false; }
    if(output && (!std::isfinite(e.chance) || e.chance<0 || e.chance>100)) { error=L"产物概率须为 0–100%。"; return false; }
    return true;
}
static std::wstring Stack(const Entry& e, Version v) {
    return L"{ " + std::wstring(v==Version::Minecraft1201 ? L"\"item\": " : L"\"id\": ") + Quote(e.id) + L", \"count\": " + std::to_wstring(e.count) + L" }";
}
bool Build(Method method, Version version, const Draft& d, std::wstring& script, std::wstring& error) {
    script.clear(); error.clear();
    int inputs=0, outputs=0;
    std::wstring ingredients=L"[", results=L"[";
    for(const auto& e:d.inputs) {
        if(!Check(e,false,error)) return false;
        if(e.id.empty()) continue;
        if(inputs++) ingredients+=L", "; ingredients+=L"{ \"item\": "+Quote(e.id)+L" }";
    }
    for(const auto& e:d.outputs) {
        if(!Check(e,true,error)) return false;
        if(e.id.empty()) continue;
        if(outputs++) results+=L", ";
        if(method==Method::Cutting) {
            if(version==Version::Minecraft1201) {
                results+=L"{ \"item\": "+Quote(e.id)+L", \"count\": "+std::to_wstring(e.count)+L", \"chance\": "+Number(e.chance/100)+L" }";
            } else results+=L"{ \"item\": "+Stack(e,version)+L", \"chance\": "+Number(e.chance/100)+L" }";
        } else {
            if(e.chance!=100) { error=L"厨锅产物不支持概率。"; return false; }
            results+=Stack(e,version);
        }
    }
    ingredients+=L"]"; results+=L"]";
    if(inputs<1 || inputs>(method==Method::Cooking ? 6 : 1)) { error=method==Method::Cooking ? L"厨锅需要 1–6 个原料，每格消耗一个。" : L"砧板需要一个原料。"; return false; }
    if(outputs<1 || outputs>(method==Method::Cooking ? 1 : 4)) { error=method==Method::Cooking ? L"厨锅需要一个产物。" : L"砧板需要 1–4 个产物。"; return false; }
    const bool cooking=method==Method::Cooking;
    std::wstring body=L"    \"type\": "+Quote(cooking ? L"farmersdelight:cooking" : L"farmersdelight:cutting")+L",\r\n    \"ingredients\": "+ingredients;
    if(!d.group.empty()) body+=L",\r\n    \"group\": "+Quote(d.group);
    if(cooking) {
        if(d.cookingTime<1 || d.cookingTime>99999999 || !std::isfinite(d.experience) || d.experience<0 || d.experience>1000000) { error=L"处理时间须为 1–99999999 tick，经验须为 0–1000000，支持小数。"; return false; }
        if(!d.tab.empty() && d.tab!=L"meals" && d.tab!=L"drinks" && d.tab!=L"misc") { error=L"请选择有效的配方书分类。"; return false; }
        if(!Check(d.container,false,error)) return false;
        // Cooking uses a single stack; cutting uses an array of chance results.
        body+=L",\r\n    \"result\": "+results.substr(1,results.size()-2);
        if(!d.container.id.empty()) body+=L",\r\n    \"container\": "+Stack(d.container,version);
        body+=L",\r\n    \"cookingtime\": "+std::to_wstring(d.cookingTime)+L",\r\n    \"experience\": "+Number(d.experience);
        if(!d.tab.empty()) body+=L",\r\n    \"recipe_book_tab\": "+Quote(d.tab);
    } else {
        std::wstring tool;
        switch(d.tool) {
        case Tool::Knives: tool=L"{ \"tag\": "+Quote(version==Version::Minecraft1201 ? L"forge:tools/knives" : L"c:tools/knife")+L" }"; break;
        case Tool::Axes:
            tool=L"[{ \"type\": "+Quote(version==Version::Minecraft1201 ? L"farmersdelight:tool_action" : L"farmersdelight:item_ability")+L", \"action\": \"axe_strip\" }, { \"tag\": \"minecraft:axes\" }]"; break;
        case Tool::Item:
            if(d.toolItem.id.empty() || !Check(d.toolItem,false,error)) { if(error.empty()) error=L"请选择砧板工具。"; return false; }
            tool=L"{ \"item\": "+Quote(d.toolItem.id)+L" }"; break;
        case Tool::Tag:
        {
            const std::wstring tag=!d.toolTag.empty() && d.toolTag[0]==L'#' ? d.toolTag.substr(1) : d.toolTag;
            if(!ResourceId(tag)) { error=L"工具标签须为命名空间:名称，可带开头的 #。"; return false; }
            tool=L"{ \"tag\": "+Quote(tag)+L" }"; break;
        }
        default: error=L"请选择砧板工具类型。"; return false;
        }
        body+=L",\r\n    \"tool\": "+tool+L",\r\n    \"result\": "+results;
        if(!d.sound.empty()) {
            if(!ResourceId(d.sound)) { error=L"声音 ID 须为命名空间:名称。"; return false; }
            body+=L",\r\n    \"sound\": ";
            body+=version==Version::Minecraft1201 ? Quote(d.sound) : L"{ \"sound_id\": "+Quote(d.sound)+L" }";
        }
    }
    script=L"ServerEvents.recipes(event => {\r\n  // Minecraft "+std::wstring(version==Version::Minecraft1201 ? L"1.20.1" : L"1.21.1")+L" · 农夫乐事 · "+(cooking ? L"厨锅烹饪" : L"砧板切割")+L"\r\n  event.custom({\r\n"+body+L"\r\n  })\r\n})\r\n";
    return true;
}
}
