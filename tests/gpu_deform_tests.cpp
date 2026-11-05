// Requires a desktop GL context. Compares real shader output with the existing CPU effects.
#include "GLUtil.hpp"
#include "Effects.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool value, const std::string& message) {
  if (!value) throw std::runtime_error(message);
}
int main(int argc, char** argv) {
  GLFWwindow* window = nullptr;
  try {
    require(glfwInit(), "GLFW initialization failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    window = glfwCreateWindow(64, 64, "GPU deformation validation", nullptr, nullptr);
    require(window != nullptr, "GL context unavailable");
    glfwMakeContextCurrent(window);
#ifdef _WIN32
    require(de::loadGL(reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress)), "GL loader failed");
#endif
    const std::string dir = argc > 1 ? argv[1] : "engine/shaders";
    const auto library = glutil::readText(dir + "/lib/gpu_deform.glsl");
    require(!library.empty(), "GPU deformation library missing");
    const std::string vertex = "#version 410 core\n" + library + R"GLSL(
layout(location=0) in vec3 aPos;
uniform int uVertices;
out vec3 result;
void main(){
  result=gpuDeform(aPos,uint(gl_VertexID),gl_InstanceID);
  gl_Position=vec4((float(gl_VertexID+gl_InstanceID*uVertices)+.5)/8192.*2.-1.,0.,0.,1.);
  gl_PointSize=1.;
})GLSL";
    const std::string fragment = "#version 410 core\nin vec3 result;out vec4 color;void main(){color=vec4(result,1.);}";
    std::string error;
    const auto program = glutil::linkProgram(vertex, fragment, error);
    require(program != 0, error);
    GLuint vao = 0, buffer = 0, texture = 0, fbo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &buffer); glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V3), nullptr);
    glEnableVertexAttribArray(0);
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 8192, 1, 0, GL_RGBA, GL_FLOAT, nullptr);
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Float target incomplete");
    glViewport(0, 0, 8192, 1);
    glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST); glDisable(GL_DITHER);
    glUseProgram(program);
    int cases = 0;
    float maximum = 0;
    for (int fixture = 0; fixture < 6; ++fixture) {
      auto base = fixture % 2 ? geo::uvSphere(8, 16) : geo::torus(24, 12);
      if (fixture >= 4) {
        const auto edges = base.e;
        base.e.resize(fixture == 4 ? 30000 : 60000);
        for (size_t i = 0; i < base.e.size(); ++i) base.e[i] = edges[i % edges.size()];
      }
      base.v.push_back({0, 0, 0}); base.v.push_back({0, 1, 0});
      if (fixture % 2) for (auto& p : base.v) { p.x += 2.3f; p.y -= .7f; }
      for (int id = 1; id < effectCount(); ++id) if (gpuDeformationSupported(id))
        for (float amount : {-4.f, -.3f, 0.f, 1.6f, 4.f})
          for (float time : {0.f, 2.345f}) {
            EffectContext context;
            context.amount = amount; context.time = time;
            context.bass = .73f; context.seed = 0xfffffff0u;
            EffectRecipe recipe;
            recipe.stageCount = fixture < 2 ? 1 : 2;
            if (fixture >= 2) recipe.stages[0] = EffectStage{13, .3f};
            auto& last = recipe.stages[recipe.stageCount - 1];
            last.effect = id;
            if (fixture >= 2) {
              last.amount = -.7f; last.timeScale = 1.25f; last.timeOffset = .4f;
              last.seedOffset = 4294967296.f;
            }
            GpuDeformation deferred;
            auto mesh = base, reference = base;
            require(applyRecipeGpu(mesh, recipe, context, deferred), "GPU planning failed");
            require(deferred.effect == id, "Supported stage was not deferred");
            require(applyRecipe(reference, recipe, context), "CPU reference failed");
            glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(mesh.v.size() * sizeof(V3)), mesh.v.data(), GL_STREAM_DRAW);
            glUniform4f(glGetUniformLocation(program, "uGpuEffect"), float(id), deferred.context.amount,
                        deferred.context.time, deferred.context.bass);
            glUniform3f(glGetUniformLocation(program, "uGpuCenter"), deferred.center.x, deferred.center.y, deferred.center.z);
            glUniform1ui(glGetUniformLocation(program, "uGpuSeed"), deferred.context.seed);
            glUniform1i(glGetUniformLocation(program, "uVertices"), int(mesh.v.size()));
            glDrawArraysInstanced(GL_POINTS, 0, GLsizei(mesh.v.size()), deferred.instances);
            require(reference.v.size() == mesh.v.size() * size_t(deferred.instances), "GPU copy count mismatch");
            std::vector<float> actual(reference.v.size() * 4);
            glReadPixels(0, 0, GLsizei(reference.v.size()), 1, GL_RGBA, GL_FLOAT, actual.data());
            require(glGetError() == GL_NO_ERROR, "GL error in deformation validation");
            for (size_t i = 0; i < reference.v.size(); ++i) {
              const auto& p = reference.v[i];
              const float expected[] = {p.x, p.y, p.z};
              for (int axis = 0; axis < 3; ++axis) {
                const float difference = std::abs(actual[i * 4 + size_t(axis)] - expected[axis]);
                maximum = std::max(maximum, difference);
                require(std::isfinite(actual[i * 4 + size_t(axis)]) &&
                        difference < 1e-4f * std::max(1.f, std::abs(expected[axis])),
                        "CPU/GPU mismatch in effect " + std::to_string(id));
              }
              require(std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z) <= deferred.radius + 1e-4f,
                      "Underestimated GPU bound in effect " + std::to_string(id));
            }
            ++cases;
          }
    }
    glDeleteFramebuffers(1, &fbo); glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &buffer); glDeleteVertexArrays(1, &vao); glDeleteProgram(program);
    glfwDestroyWindow(window); glfwTerminate();
    std::cout << "GPU deformation validation: " << cases << " cases passed, maximum error " << maximum << "\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }
}
