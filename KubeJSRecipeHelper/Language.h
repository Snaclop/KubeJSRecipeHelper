#pragma once

class CWnd;

namespace UiLanguage {
// The order matches the three entries in the main window's language selector.
enum class Language { English, SimplifiedChinese, TraditionalChinese };
Language Get();
void Set(Language eLanguage);
Language FromWindowsLanguage(unsigned short uLanguageId);
Language SystemDefault();
const wchar_t* Text(const wchar_t* pszSource);
// Translate resource captions and labels, leaving editable values and icons intact.
void ApplyDialog(CWnd& dialog);
}
