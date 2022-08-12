#pragma once

#include <algorithm>
#include <array>
#include <assert.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <WindowsNumerics.h>
#include <d2d1_2.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <dwrite_2.h>
#include <windows.h>

#pragma warning(disable : 26812)
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#pragma warning(disable : 26451)
#include <XrUtility/XrError.h>
#include <XrUtility/XrExtensions.h>
#include <XrUtility/XrHandle.h>
#include <XrUtility/XrMath.h>
#include <XrUtility/XrString.h>

#include <wincodec.h>

#include <Windows.Graphics.DirectX.Direct3D11.Interop.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Diagnostics.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Graphics.Holographic.h>
#include <winrt/Windows.Media.Capture.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Media.SpeechRecognition.h>
#include <winrt/Windows.Media.SpeechSynthesis.h>
#include <winrt/Windows.Perception.People.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.Spatial.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.Web.Syndication.h>

#include <pplawait.h>
#include <ppltasks.h>

#include <winrt/Microsoft.MixedReality.SceneUnderstanding.SpatialGraph.h>
#include <winrt/Microsoft.MixedReality.SceneUnderstanding.h>

#pragma warning(disable : 26812)

#include <DirectXTex.h>
#include <WICTextureLoader.h>