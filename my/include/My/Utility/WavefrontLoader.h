#pragma once

#include "pch.h"

#include "My/Engine/ComponentManager.h"
#include "My/Engine/EntityManager.h"
#include "My/Engine/MaterialStructures.h"
#include "My/Engine/MeshComponent.h"

#include "My/Utility/Utility.h"


namespace My::Utility
{

namespace _implementation
{

using namespace My::Engine;

class MTLLoader
{
public:
    static std::unordered_map<std::wstring, MaterialPointer> Load(std::wstring filename)
    {
        using namespace std;

        unordered_map<wstring, MaterialPointer> result;

        wstring s_str = ReadFileWide(filename);
        wstringstream file_ss(s_str);
        wstring line{L""};

        wstring mat_name = L"";
        float Ns, Ka, Ks, Ni, d, illum; // Ke for emmission
        My::Color Kd;

        auto push_material = [&]() {
            PBRMaterialData data;
            data.Albedo = Kd;
            data.Roughness = 1 - sqrt(Ns) / 30;
            data.Metalness = illum == 3 || illum == 6 ? Ka : 0.f;
            data.AmbientOcclusion = 1.f;
            data.IOR = Ni;
            data.Alpha = d;

            auto rs = make_pair(mat_name, MaterialPointer(data));
            result.insert(rs);
        };

        while (getline(file_ss, line))
        {
            if (line[0] == '#') continue;

            wstringstream ss(line);
            wstring command;
            ss >> command;

            if (command == L"Ka")
                ss >> Ka;
            else if (command == L"Kd")
                ss >> Kd.r >> Kd.g >> Kd.b;
            else if (command == L"Ks")
                ss >> Ks;
            else if (command == L"Ns")
                ss >> Ns;
            else if (command == L"Ni")
                ss >> Ni;
            else if (command == L"d")
                ss >> d;
            else if (command == L"illum")
                ss >> illum;
            else if (command == L"newmtl")
            {
                if (mat_name != L"") push_material();
                ss >> mat_name;
            }
        }
        if (mat_name != L"") push_material();

        return result;
    }
};

class OBJLoader
{
public:
    static EntityReference Load(std::wstring filename,
                     std::unordered_map<std::wstring, MaterialPointer> materials,
                     ComponentManager & component_manager, EntityManager & entity_manager)
    {
        using namespace std;
        using namespace winrt;
        using namespace My::Engine;

        wstring line{L""};

        wstring s_str = ReadFileWide(filename);
        wstringstream file_ss(s_str);

        vector<glm::vec3> read_vertices;
        vector<glm::vec3> read_normals;
        vector<glm::vec2> read_uv;

        vector<glm::vec3> vertices;
        vector<glm::vec3> normals;
        vector<glm::vec2> UV;

        EntityReference entity = entity_manager.CreateEntity();
        MaterialPointer material;

        auto push_object = [&]() {
            vector<UINT> indices(vertices.size());
            iota(indices.begin(), indices.end(), 0);

            MeshComponent comp;
            comp.Vertices = vertices;
            comp.Normals = normals;
            comp.UVs = UV;
            comp.Indices = indices;

            comp.MPointer = material;

            component_manager.RegisterComponent(entity_manager, entity, comp);

            vertices.clear();
            normals.clear();
            UV.clear();
        };

        bool reading = false;

        while (getline(file_ss, line))
        {
            if (line[0] == '#') continue; // dump comment line

            wstringstream ss(line);
            wstring command;
            ss >> command;

            if (command == L"mtllib")
            {
            }
            else if (command == L"o")
            {
                // read_uv.clear(); // blender obj exporter continues counting
                if (reading) push_object();
                reading = true;
            }
            else if (command == L"v")
            {
                float x, y, z;
                ss >> x >> y >> z;
                read_vertices.push_back(glm::vec3{x, y, z});
            }
            else if (command == L"vt")
            {
                float u, v;
                ss >> u >> v;
                read_uv.push_back(glm::vec2{u, v});
            }
            else if (command == L"vn")
            {
                float x, y, z;
                ss >> x >> y >> z;
                read_normals.push_back(glm::vec3{x, y, z});
            }
            else if (command == L"usemtl")
            {
                wstring materialid;
                ss >> materialid;
                material = materials[materialid];
            }
            else if (command == L"s")
            {} // TODO: smooth shading
            else if (command == L"f")
            {
                wstring vtxs;
                struct INDICES
                {
                    size_t v, u, n;
                };
                vector<INDICES> vtx_l;
                getline(ss, vtxs);

                wstring vtx;
                wstringstream vtx_ss(vtxs);
                while (getline(vtx_ss, vtx, L' '))
                {
                    if (vtx == L"") continue;
                    wstringstream vtx_ss(vtx);
                    wstring tmp;
                    INDICES idc;
                    getline(vtx_ss, tmp, L'/');
                    idc.v = stoi(tmp);
                    getline(vtx_ss, tmp, L'/');
                    idc.u = stoi(tmp);
                    getline(vtx_ss, tmp);
                    idc.n = stoi(tmp);
                    vtx_l.push_back(idc);
                }

                if (vtx_l.size() == 3)
                {
                    for (size_t q = 0; q < vtx_l.size(); ++q)
                    {
                        auto idx_s = vtx_l.size() - 1 - q;
                        auto & idx = vtx_l[idx_s];
                        vertices.push_back(read_vertices[idx.v - 1]);
                        UV.push_back(read_uv[idx.u - 1]);
                        normals.push_back(read_normals[idx.n - 1]);
                    }
                }
                else
                    throw std::runtime_error("Utility: Could not parse face.");
            }
        }
        if (vertices.size()) push_object();

        return entity;
    }
};

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;

class WavefrontLoader
{
public:
    static EntityReference Load(std::wstring filename,          //
                     EntityManager & entity_manager, //
                     ComponentManager & component_manager)
    {
        auto s = Split(filename, L'/');
        auto fname = s.back();
        auto name = fname.substr(0, fname.size() - 4);
        s.resize(s.size() - 1);
        auto path = Join(s, L'/');
        auto materials = MTLLoader::Load(path + L'/' + name + L".mtl");

        return OBJLoader::Load(path + L'/' + name + L".obj", materials, component_manager, entity_manager);
    }
};

} // namespace _implementation

using _implementation::WavefrontLoader;

} // namespace My::Utility