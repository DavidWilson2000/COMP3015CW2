#ifndef SIMPLE_OBJ_LOADER_H
#define SIMPLE_OBJ_LOADER_H

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace SimpleOBJ
{
    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Index
    {
        int v = 0;
        int vt = 0;
        int vn = 0;
    };

    inline std::string trim(const std::string& s)
    {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
            ++start;

        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
            --end;

        return s.substr(start, end - start);
    }

    inline int resolveIndex(int idx, int size)
    {
        if (idx > 0) return idx - 1;
        if (idx < 0) return size + idx;
        return -1;
    }

    inline bool parseFaceVertex(const std::string& token, Index& out)
    {
        out = {};

        size_t firstSlash = token.find('/');
        if (firstSlash == std::string::npos)
        {
            out.v = std::stoi(token);
            return true;
        }

        size_t secondSlash = token.find('/', firstSlash + 1);

        std::string vStr = token.substr(0, firstSlash);
        std::string vtStr;
        std::string vnStr;

        if (secondSlash == std::string::npos)
        {
            vtStr = token.substr(firstSlash + 1);
        }
        else
        {
            vtStr = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
            vnStr = token.substr(secondSlash + 1);
        }

        if (!vStr.empty()) out.v = std::stoi(vStr);
        if (!vtStr.empty()) out.vt = std::stoi(vtStr);
        if (!vnStr.empty()) out.vn = std::stoi(vnStr);

        return true;
    }

    inline void appendVertex(
        const Index& idx,
        const std::vector<Vec3>& positions,
        const std::vector<Vec2>& texcoords,
        const std::vector<Vec3>& normals,
        std::vector<float>& outVertices)
    {
        Vec3 p{};
        Vec2 t{};
        Vec3 n{0.0f, 1.0f, 0.0f};

        int vi = resolveIndex(idx.v, static_cast<int>(positions.size()));
        if (vi >= 0 && vi < static_cast<int>(positions.size()))
            p = positions[vi];

        int ti = resolveIndex(idx.vt, static_cast<int>(texcoords.size()));
        if (ti >= 0 && ti < static_cast<int>(texcoords.size()))
            t = texcoords[ti];

        int ni = resolveIndex(idx.vn, static_cast<int>(normals.size()));
        if (ni >= 0 && ni < static_cast<int>(normals.size()))
            n = normals[ni];

        outVertices.push_back(p.x);
        outVertices.push_back(p.y);
        outVertices.push_back(p.z);

        outVertices.push_back(n.x);
        outVertices.push_back(n.y);
        outVertices.push_back(n.z);

        outVertices.push_back(t.x);
        outVertices.push_back(t.y);
    }

    inline bool loadOBJInterleaved(
        const std::string& path,
        std::vector<float>& outVertices,
        std::string* warning = nullptr,
        std::string* error = nullptr)
    {
        outVertices.clear();

        std::ifstream file(path);
        if (!file)
        {
            if (error) *error = "Failed to open OBJ file: " + path;
            return false;
        }

        std::vector<Vec3> positions;
        std::vector<Vec2> texcoords;
        std::vector<Vec3> normals;

        std::string line;
        int lineNumber = 0;

        while (std::getline(file, line))
        {
            ++lineNumber;
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v")
            {
                Vec3 v;
                iss >> v.x >> v.y >> v.z;
                positions.push_back(v);
            }
            else if (prefix == "vt")
            {
                Vec2 vt;
                iss >> vt.x >> vt.y;
                texcoords.push_back(vt);
            }
            else if (prefix == "vn")
            {
                Vec3 vn;
                iss >> vn.x >> vn.y >> vn.z;
                normals.push_back(vn);
            }
            else if (prefix == "f")
            {
                std::vector<Index> face;
                std::string token;
                while (iss >> token)
                {
                    Index idx;
                    try
                    {
                        if (parseFaceVertex(token, idx))
                            face.push_back(idx);
                    }
                    catch (...)
                    {
                        if (warning)
                        {
                            *warning += "Skipped malformed face token on line " + std::to_string(lineNumber) + ": " + token + "\n";
                        }
                    }
                }

                if (face.size() < 3)
                    continue;

                for (size_t i = 1; i + 1 < face.size(); ++i)
                {
                    appendVertex(face[0], positions, texcoords, normals, outVertices);
                    appendVertex(face[i], positions, texcoords, normals, outVertices);
                    appendVertex(face[i + 1], positions, texcoords, normals, outVertices);
                }
            }
        }

        if (positions.empty())
        {
            if (error) *error = "OBJ contained no positions: " + path;
            return false;
        }

        if (outVertices.empty())
        {
            if (error) *error = "OBJ contained no drawable face data: " + path;
            return false;
        }

        return true;
    }
}

#endif
