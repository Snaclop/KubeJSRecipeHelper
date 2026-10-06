#include "CreateAddonRecipe.h"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace CreateAddonRecipe {
// Limits come from the addons' serializers and Create's inherited BasinRecipe.
static const Info Infos[]={
    {L"批量染色",L"create_dragons_plus:coloring",1,12,0,0,false,false,false},
    {L"批量冷冻",L"create_dragons_plus:freezing",1,12,0,0,false,false,false},
    {L"批量终结",L"create_dragons_plus:ending",1,12,0,0,false,false,false},
    {L"批量喷砂",L"create_dragons_plus:sanding",1,12,0,0,false,false,false},
    {L"发酵",L"createdieselgenerators:basin_fermenting",64,4,2,2,true,true,false},
    {L"批量发酵",L"createdieselgenerators:bulk_fermenting",9,4,2,2,true,true,false},
    {L"分馏",L"createdieselgenerators:distillation",0,0,1,6,true,true,false},
    {L"铸模冲压",L"createdieselgenerators:compression_molding",64,4,2,2,true,true,true},
    {L"铸造",L"createdieselgenerators:casting",0,1,1,0,false,false,true},
    {L"锤击",L"createdieselgenerators:hammering",1,1,0,0,false,false,false},
    {L"剪线",L"createdieselgenerators:wire_cutting",1,1,0,0,false,false,false}
};
const Info& GetInfo(Method method) { return Infos[method]; }
Draft MakeDraft(Method method) {
    const auto& info=GetInfo(method); Draft d;
    if(info.itemsIn) d.inputs.resize(1);
    if(info.itemsOut) d.outputs.resize(1);
    if(info.fluidsIn) { d.fluidInputs.resize(1); d.fluidInputs[0].kind=RecipeFluid; }
    if(info.fluidsOut) { d.fluidOutputs.resize(1); d.fluidOutputs[0].kind=RecipeFluid; }
    return d;
}
static bool Id(const std::wstring& id) {
    auto colon=id.find(L':');
    if(colon==std::wstring::npos || colon==0 || colon+1==id.size()) return false;
    for(size_t i=0;i<id.size();++i) {
        auto c=id[i]; if(i==colon) continue;
        if((c>=L'a' && c<=L'z') || (c>=L'0' && c<=L'9') || c==L'_' || c==L'-' || c==L'.' || (i>colon && c==L'/')) continue;
        return false;
    }
    return true;
}
static std::wstring Q(const std::wstring& id) { return L"\""+id+L"\""; } // Only validated resource IDs are quoted.
static std::wstring Number(double n) { std::wostringstream out; out.imbue(std::locale::classic()); out<<std::setprecision(15)<<n; return out.str(); }
bool Build(Method method, Version version, const Draft& d, std::wstring& script, std::wstring& error) {
    script.clear(); error.clear();
    if(method<0 || method>=MethodCount) { error=L"请选择配方类型。"; return false; }
    const auto& info=GetInfo(method); const bool modern=version==Version::Minecraft1211;
    int ii=0,io=0,fi=0,fo=0; std::wstring ingredients,results;
    auto append=[](std::wstring& list,const std::wstring& value) { if(!list.empty()) list+=L", "; list+=value; };
    auto entries=[&](const std::vector<Entry>& values,bool fluid,bool output,int& count) {
        for(const auto& e:values) {
            if(e.id.empty()) continue;
            if(!Id(e.id) || e.kind!=(fluid ? RecipeFluid : RecipeItem)) { error=L"请在物品格选择物品，在流体格选择流体；ID 须为有效的命名空间:名称。"; return false; }
            if(e.count<1 || e.count>(fluid ? 99999999 : 64)) { error=L"物品数量须为 1–64，流体数量须为 1–99999999 mB。"; return false; }
            if(output && (!std::isfinite(e.chance) || e.chance<0 || e.chance>100 || (modern && !fluid && e.chance==0))) {
                error=modern ? L"1.21.1 的物品产物概率须大于 0 且不超过 100%。" : L"产物概率须为 0–100%。"; return false;
            }
            if(fluid && e.chance!=100) { error=L"流体产物不支持概率。"; return false; }
            count+=output || fluid ? 1 : e.count;
            std::wstring value;
            if(fluid) {
                value=L"{ "+std::wstring(modern && !output ? L"\"type\": \"fluid_stack\", " : L"")+
                    (modern && output ? L"\"id\": " : L"\"fluid\": ")+Q(e.id)+L", \"amount\": "+std::to_wstring(e.count)+L" }";
            } else if(output) {
                value=L"{ "+std::wstring(modern ? L"\"id\": " : L"\"item\": ")+Q(e.id)+L", \"count\": "+std::to_wstring(e.count);
                if(e.chance!=100) value+=L", \"chance\": "+Number(e.chance/100);
                value+=L" }";
            } else value=L"{ \"item\": "+Q(e.id)+L" }";
            for(int n=0;n<(!output && !fluid ? e.count : 1);++n) append(output ? results : ingredients,value);
        }
        return true;
    };
    if(!entries(d.inputs,false,false,ii) || !entries(d.outputs,false,true,io) ||
       !entries(d.fluidInputs,true,false,fi) || !entries(d.fluidOutputs,true,true,fo)) return false;
    if(ii>info.itemsIn || io>info.itemsOut || fi>info.fluidsIn || fo>info.fluidsOut) { error=L"原料总数量或产物格数超过该配方类型的限制。"; return false; }
    if(ii+fi==0 || io+fo==0) { error=L"请至少选择一种原料和一种产物。"; return false; }
    std::wstring body=L"    \"type\": "+Q(info.type)+L",\r\n    \"ingredients\": ["+ingredients+L"],\r\n    \"results\": ["+results+L"]";
    if(info.time && d.customTime) {
        if(d.time<1 || d.time>99999999) { error=L"处理时间须为 1–99999999 tick。"; return false; }
        body+=L",\r\n    "+Q(modern ? L"processing_time" : L"processingTime")+L": "+std::to_wstring(d.time);
    }
    if(info.heat) {
        if(d.heat<0 || d.heat>2) { error=L"请选择有效的热量条件。"; return false; }
        if(d.heat) body+=L",\r\n    "+Q(modern ? L"heat_requirement" : L"heatRequirement")+L": "+Q(d.heat==1 ? L"heated" : L"superheated");
    }
    if(info.mold) {
        if(!Id(d.mold)) { error=L"模具 ID 须为有效的命名空间:名称。"; return false; }
        body+=L",\r\n    \"mold\": "+Q(d.mold);
    }
    if(method==Coloring) {
        if(!Id(d.color) || d.dyeAmount<1 || d.dyeAmount>1000) { error=L"请选择染料颜色；染料用量须为 1–1000 mB。"; return false; }
        body+=L",\r\n    \"color\": "+Q(d.color);
        if(d.dyeAmount!=32) body+=L",\r\n    \"dye_fluid_amount\": "+std::to_wstring(d.dyeAmount);
    }
    script=L"ServerEvents.recipes(event => {\r\n  // Minecraft "+std::wstring(modern ? L"1.21.1" : L"1.20.1")+L" · "+info.name+
        L"\r\n  event.custom({\r\n"+body+L"\r\n  })\r\n})\r\n";
    return true;
}
}
