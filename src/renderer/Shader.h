#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include "core/Base.h"
#include "Texture.h"

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertFile, const std::string& fragFile);
    void Bind() const;
    void SetInt(const std::string& name, i32 num);
    void SetFloat(const std::string& name, f32 num);
    void SetVec3(const std::string& name, const glm::vec3& vec);
    void SetVec4(const std::string& name, const glm::vec4& vec);
    void SetMat4(const std::string& name, const glm::mat4& mat4);
private:
    inline i32 GetUniformLocation(const std::string& name);
    void CreateShader(const std::string& vertFile, const std::string& fragFile);
    std::string LoadShaderFile(const std::string& filePath) const;
    void CheckCompileErrors(u32 shader, const std::string& type) const;

    i32 shaderId; 
    std::unordered_map<std::string, i32> m_UniformLocations;
};