#include "pch.h"

#include "My/Utility/Utility.h"

std::wstring My::Utility::ReadFileWide(std::wstring filename)
{
    std::wifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<wchar_t> buffer(size);
    if (file.read(buffer.data(), size))
    {
        return std::wstring(buffer.begin(), buffer.end());
    }
    else
    {
        throw std::runtime_error("Could not read file.");
    }
}

std::string My::Utility::ReadFile(std::string filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
    {
        return std::string(buffer.begin(), buffer.end());
    }
    else
    {
        throw std::runtime_error("Could not read file.");
    }
}

std::vector<std::wstring> My::Utility::Split(const std::wstring & s, wchar_t seperator)
{
    std::vector<std::wstring> output;

    std::wstring::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find(seperator, pos)) != std::wstring::npos)
    {
        std::wstring substring(s.substr(prev_pos, pos - prev_pos));

        output.push_back(substring);

        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word

    return output;
}

std::wstring My::Utility::Join(std::vector<std::wstring> & tjoin, wchar_t sep)
{
    std::wstring res;
    for (auto & s : tjoin) res += s + sep;
    return res;
}
