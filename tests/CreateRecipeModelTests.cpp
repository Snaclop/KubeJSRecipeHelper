#include "../KubeJSRecipeHelper/CreateRecipeModel.h"
#include <codecvt>
#include <fstream>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <limits>
using namespace CreateRecipe;
static int checks = 0;
static void Require(bool condition, const char* message) {
    ++checks; if (!condition) throw std::runtime_error(message);
}
static Entry ItemEntry(const wchar_t* id, int count = 1, double chance = 100) {
    Entry e; e.id=id; e.count=count; e.chance=chance; return e;
}
static Entry FluidEntry(const wchar_t* id, int amount = 1000) {
    Entry e=ItemEntry(id,amount); e.kind=RecipeFluid; return e;
}
int main(int argc, char** argv) {
    try {
        std::ofstream fixtures(argc > 1 ? argv[1] : "create-fixtures.js");
        std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8;
        std::wstring script,error;
        Draft simple; simple.inputs={ItemEntry(L"minecraft:iron_ingot")}; simple.outputs={ItemEntry(L"create:iron_sheet")};
        for(int i=0;i<MethodCount;++i) {
            Method method=static_cast<Method>(i); Draft d=simple;
            if(method==Deploying) { d.inputs.push_back(ItemEntry(L"minecraft:sandpaper")); d.keepHeld=true; }
            if(method==Filling) d.inputs.push_back(FluidEntry(L"minecraft:water"));
            if(method==Emptying) d.outputs.push_back(FluidEntry(L"minecraft:water"));
            if(method==MechanicalCrafting) { d.grid[0]=ItemEntry(L"minecraft:iron_ingot"); d.mirrored=false; }
            if(method==SequencedAssembly) {
                d.transition=ItemEntry(L"create:incomplete_precision_mechanism");
                Step press,cut,deploy,fill; cut.method=Cutting; cut.customTime=true; cut.time=250;
                deploy.method=Deploying; deploy.secondary=ItemEntry(L"create:small_cogwheel"); deploy.keepHeld=true;
                fill.method=Filling; fill.secondary=FluidEntry(L"minecraft:water",250);
                d.steps={press,cut,deploy,fill}; d.outputs={ItemEntry(L"create:precision_mechanism",1,120),ItemEntry(L"minecraft:iron_nugget",3,30)};
            }
            Require(Build(method,d,script,error),"valid recipe failed");
            Require(script.find(GetInfo(method).function)!=std::wstring::npos,"wrong method");
            fixtures<<utf8.to_bytes(script)<<"\n";
            if(method==SequencedAssembly) {
                Require(script.find(L"CreateItem.of(Item.of('minecraft:iron_nugget', 3), 0.25)")!=std::wstring::npos,"relative weights lost");
                Require(script.find(L".transitionalItem('create:incomplete_precision_mechanism').loops(5)")!=std::wstring::npos,"assembly defaults lost");
                Require(script.find(L".keepHeldItem()")!=std::wstring::npos,"assembly held item lost");
            }
        }
        Draft d=simple; d.outputs[0].count=4; d.outputs[0].chance=12.5; d.customTime=true; d.time=360;
        Require(Build(Crushing,d,script,error),"decimal chance failed");
        Require(script.find(L"CreateItem.of(Item.of('create:iron_sheet', 4), 0.125)")!=std::wstring::npos,"chance or count lost");
        Require(script.find(L".processingTime(360)")!=std::wstring::npos,"custom duration lost");
        fixtures<<utf8.to_bytes(script)<<"\n";
        d=simple; Require(Build(Milling,d,script,error),"default duration failed");
        Require(script.find(L"processingTime")==std::wstring::npos,"default duration should be omitted");
        d.inputs={FluidEntry(L"minecraft:water",500)}; d.outputs={FluidEntry(L"create:tea",500)}; d.heat=2;
        Require(Build(Mixing,d,script,error),"fluid only mixing failed");
        Require(script.find(L"Fluid.of('create:tea', 500)")!=std::wstring::npos,"fluid output lost");
        fixtures<<utf8.to_bytes(script)<<"\n";
        Require(!Build(Compacting,d,script,error),"compacting accepted only fluid output");
        d=simple; d.inputs[0].count=64; d.inputs.push_back(FluidEntry(L"minecraft:water"));
        d.inputs.push_back(FluidEntry(L"minecraft:lava"));
        Require(Build(Mixing,d,script,error),"64 item plus 2 fluid basin limit failed");
        fixtures<<utf8.to_bytes(script)<<"\n";
        d.inputs.push_back(ItemEntry(L"minecraft:dirt")); Require(!Build(Mixing,d,script,error),"basin accepted 65 items");
        d.inputs.pop_back(); d.inputs.push_back(FluidEntry(L"create:tea")); Require(!Build(Mixing,d,script,error),"basin accepted 3 fluids");
        d=simple; d.inputs.push_back(FluidEntry(L"minecraft:water")); Require(!Build(Crushing,d,script,error),"crusher accepted fluid");
        d=simple; Require(!Build(Deploying,d,script,error),"deployer accepted one input");
        Require(!Build(Filling,d,script,error),"filling accepted missing fluid");
        Require(!Build(Emptying,d,script,error),"emptying accepted missing fluid");
        d=simple; d.outputs.resize(8,ItemEntry(L"minecraft:dirt")); Require(!Build(Crushing,d,script,error),"crusher accepted 8 outputs");
        d=simple; d.outputs[0].chance=-1; Require(!Build(Pressing,d,script,error),"negative chance accepted");
        d.outputs[0].chance=101; Require(!Build(Pressing,d,script,error),"chance above 100 accepted");
        d.outputs[0].chance=std::numeric_limits<double>::quiet_NaN(); Require(!Build(Pressing,d,script,error),"NaN accepted");
        d=simple; d.rows=d.columns=9;
        for(int i=0;i<81;++i) d.grid[i]=ItemEntry((L"test:item_"+std::to_wstring(i)).c_str());
        Require(Build(MechanicalCrafting,d,script,error),"81 distinct keys failed"); fixtures<<utf8.to_bytes(script)<<"\n";
        d.outputs[0].chance=50; Require(!Build(MechanicalCrafting,d,script,error),"crafting accepted chance");
        d.outputs[0].chance=100; d.grid[80]=FluidEntry(L"minecraft:water"); Require(!Build(MechanicalCrafting,d,script,error),"crafting accepted fluid");
        d=simple; d.transition=ItemEntry(L"test:transition"); Step bad; bad.method=Mixing; d.steps={bad};
        Require(!Build(SequencedAssembly,d,script,error),"assembly accepted mixing step");
        bad.method=Filling; d.steps={bad}; Require(!Build(SequencedAssembly,d,script,error),"assembly accepted missing step fluid");
        d.steps[0].secondary=FluidEntry(L"minecraft:water"); d.outputs[0].chance=0;
        Require(!Build(SequencedAssembly,d,script,error),"assembly accepted zero weight");
        RecipeTypeMap catalog; catalog[L"test:both"]=RecipeItem|RecipeFluid; catalog[L"minecraft:water"]=RecipeFluid;
        Require(RecipeKinds(&catalog,L"test:both")==3,"same ID kind collision");
        Require(RecipeKinds(&catalog,L"minecraft:water")==RecipeFluid,"fluid metadata lost");
        Require(RecipeKinds(nullptr,L"test:item")==RecipeItem,"legacy selection fallback lost");
        std::cout<<checks<<" checks passed\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
