#pragma once

class CWnd;

namespace UiLanguage {
// The order matches the three entries in the main window's language selector.
enum class Language { English, SimplifiedChinese, TraditionalChinese };
Language Get();
void Set(Language language);
Language FromWindowsLanguage(unsigned short languageId);
Language SystemDefault();
const wchar_t* Text(const wchar_t* source);
// Translate resource captions and labels, leaving editable values and icons intact.
void ApplyDialog(CWnd& dialog);
}
