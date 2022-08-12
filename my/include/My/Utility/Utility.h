#pragma once

#include "pch.h"

namespace My::Utility
{

std::wstring ReadFileWide(std::wstring filename);

std::string ReadFile(std::string filename);

std::vector<std::wstring> Split(const std::wstring & s, wchar_t seperator);

std::wstring Join(std::vector<std::wstring> & tjoin, wchar_t sep = L'/');

} // namespace My::Utility
