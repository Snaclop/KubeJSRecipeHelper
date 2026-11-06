#include "ScriptExport.h"
#include <fstream>

namespace {
bool WriteRecipeScript(std::ostream& stream, const std::string& strScript)
{
    stream << strScript << std::endl;
    return stream.good();
}
}

bool AppendRecipeScript(const std::wstring& strPath, const std::string& strScript)
{
    std::ofstream stream(strPath.c_str(), std::ios::app | std::ios::binary);
    if (!stream.is_open()) return false;
    const bool bWritten = WriteRecipeScript(stream, strScript);
    stream.close();
    return bWritten && !stream.fail();
}
