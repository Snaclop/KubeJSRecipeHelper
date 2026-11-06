#pragma once
#include <string>

// Append UTF-8 script text and a newline. True means writing and closing succeeded.
bool AppendRecipeScript(const std::wstring& strPath, const std::string& strScript);
