#pragma once

#include "pch.h"

namespace My::Utility
{

namespace _implementation
{

/**
 * @brief   Class containing various utilities and enumerations for winrt/C++ (DirectX)
 * environments.
 *
 * @ingroup Utility
 * @author  Ronja Schnur (rschnur@students.uni-mainz.de)
 */
class winrtUtility
{
public:
    static inline void LogMessage(winrt::hstring str)
    {
        winrt::hstring output(str);
        output = L"LOG: " + output + L"\n";

#ifdef _DEBUG
        winrt::Windows::Foundation::Diagnostics::LoggingChannel log_channel(
            L"MyLens Log"
#ifdef NATIVE_LOGGING_CHANNEL
            ,
            0,
            {0x4bd2826e,
             0x54a1,
             0x4ba9,
             {0xbf, 0x63, 0x92, 0xb7, 0x3e, 0xa1, 0xac, 0x4a}}); // GUID optional
#else
        );
#endif
        log_channel.LogMessage(output);
        OutputDebugString(output.c_str());
#endif
    }

    static std::wstring V2S(const DirectX::XMVECTOR & v)
    {
        using namespace DirectX;

        float x = XMVectorGetByIndex(v, 0);
        float y = XMVectorGetByIndex(v, 1);
        float z = XMVectorGetByIndex(v, 2);
        float w = XMVectorGetByIndex(v, 3);

        std::wstringstream ss;
        ss << L"vector( x: " << x << L" y: " << y << L" z: " << z << L" w: " << w << L" )";
        return ss.str();
    }
};

} // namespace _implementation

using _implementation::winrtUtility;

} // namespace My::Utility