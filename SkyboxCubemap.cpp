#include "SkyboxCubemap.h"

#include <array>
#include <iostream>
#include <string>

#include "helper/stb/stb_image.h"

namespace
{
    GLuint LoadCubemapFromFaces(const std::array<std::string, 6>& facePaths)
    {
        stbi_set_flip_vertically_on_load(false);

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

        int loadedFaces = 0;

        for (unsigned int i = 0; i < facePaths.size(); ++i)
        {
            int width = 0;
            int height = 0;
            int channels = 0;
            unsigned char* data = stbi_load(facePaths[i].c_str(), &width, &height, &channels, 0);

            if (!data)
            {
                std::cerr << "Failed to load skybox face: " << facePaths[i] << std::endl;
                continue;
            }

            GLenum format = GL_RGB;
            if (channels == 1) format = GL_RED;
            else if (channels == 3) format = GL_RGB;
            else if (channels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            ++loadedFaces;
        }

        if (loadedFaces != 6)
        {
            glDeleteTextures(1, &textureId);
            return 0;
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        std::cout << "Loaded skybox cubemap from folder." << std::endl;
        return textureId;
    }
}

GLuint CreateFallbackSkyboxCubemap()
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    const unsigned char faces[6][3] = {
        {118, 164, 214},
        {110, 156, 206},
        {76,  112, 170},
        {150, 170, 190},
        {124, 170, 220},
        {110, 150, 200}
    };

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, faces[i]);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tex;
}

GLuint LoadSkyboxCubemapOrFallback(const std::string& folderPath)
{
    const std::array<std::string, 6> jpgFaces = {
        folderPath + "right.jpg",
        folderPath + "left.jpg",
        folderPath + "top.jpg",
        folderPath + "bottom.jpg",
        folderPath + "front.jpg",
        folderPath + "back.jpg"
    };

    GLuint cubemap = LoadCubemapFromFaces(jpgFaces);
    if (cubemap != 0)
        return cubemap;

    const std::array<std::string, 6> pngFaces = {
        folderPath + "right.png",
        folderPath + "left.png",
        folderPath + "top.png",
        folderPath + "bottom.png",
        folderPath + "front.png",
        folderPath + "back.png"
    };

    cubemap = LoadCubemapFromFaces(pngFaces);
    if (cubemap != 0)
        return cubemap;

    std::cout << "Using fallback skybox cubemap instead." << std::endl;
    return CreateFallbackSkyboxCubemap();
}
