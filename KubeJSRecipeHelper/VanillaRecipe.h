#pragma once
#include <array>
#include <string>

namespace VanillaRecipe {
enum class Method { Shaped, Shapeless, Smithing, Smelting, Blasting, Smoking, Stonecutting, Count };
constexpr int MethodCount = static_cast<int>(Method::Count);
struct Info { const wchar_t* name; const wchar_t* scriptMethod; int inputs; int defaultTime; };
struct Entry { std::wstring id; int count = 1; };
struct Draft { std::array<Entry, 9> inputs; Entry output; int time = 200; };
const Info& GetInfo(Method method);
bool IsCrafting(Method method);
bool IsCooking(Method method);
int InputCountLimit(Method method, const Draft& draft, int index);
bool Validate(Method method, const Draft& draft, std::wstring& error);
std::wstring BuildScript(Method method, const Draft& draft);

// 有序、无序共用材料；其他方式分别保存原料、产物和处理时间。
class DraftBank {
public:
    DraftBank();
    Draft& Get(Method method);
    const Draft& Get(Method method) const;
private:
    std::array<Draft, MethodCount> m_drafts;
};
}
