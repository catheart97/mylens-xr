#pragma once

#include "pch.h"

namespace My::D3D11RenderSystem
{

inline DirectX::XMFLOAT3 cv(glm::vec3 v) { return {v.x, v.y, v.z}; }
inline DirectX::XMVECTOR cvx(glm::vec3 v) { return DirectX::XMVectorSet(v.x, v.y, v.z, 1.0f); }
inline DirectX::XMFLOAT2 cv(glm::vec2 v) { return {v.x, v.y}; }
inline DirectX::XMVECTOR cvx(glm::quat q) { return DirectX::XMVectorSet(q.x, q.y, q.z, q.w); }

} // namespace My::D3D11RenderSystem