#pragma once
#include <array>
#include <string>
#include "RecipeCatalog.h"

namespace VanillaRecipe {
enum class Method { Shaped, Shapeless, Smithing, Smelting, Blasting, Smoking, Stonecutting, Count };
constexpr int MethodCount = static_cast<int>(Method::Count);
struct Info { const wchar_t* pName; const wchar_t* pScriptMethod; int nInputs; int nDefaultTime; };
struct Entry { std::wstring strId; int nCount = 1; };
struct Draft { std::array<Entry, 9> arrInputs; Entry output; int nTime = 200; };
const Info& GetInfo(Method eMethod);
bool IsCrafting(Method eMethod);
bool IsCooking(Method eMethod);
int InputCountLimit(Method eMethod, const Draft& draft, int nSlotIndex);
bool Validate(Method eMethod, const Draft& draft, std::wstring& strError);
std::wstring BuildScript(Method eMethod, const Draft& draft);

// 有序、无序共用材料；其他方式分别保存原料、产物和处理时间。
class DraftBank {
public:
    DraftBank();
    Draft& Get(Method eMethod);
private:
    std::array<Draft, MethodCount> m_arrDrafts;
};
}
