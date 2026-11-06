#pragma once
#include <iomanip>
#include <locale>
#include <limits>
#include <sstream>
#include <string>

inline std::wstring FormatRecipeNumber(double dValue)
{
    std::wostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << dValue;
    return stream.str();
}
