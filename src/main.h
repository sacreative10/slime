#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>

uint32_t vec4tobyte(const glm::vec4 &v) {
  uint32_t r = (uint32_t)(v.r * 255.0f);
  uint32_t g = (uint32_t)(v.g * 255.0f);
  uint32_t b = (uint32_t)(v.b * 255.0f);
  uint32_t a = (uint32_t)(v.a * 255.0f);
  return (a << 24) | (b << 16) | (g << 8) | r;
}

glm::vec4 bytetovec4(uint32_t v) {
  float a = (float)(v & 0xff) / 255.0f;
  float r = (float)((v >> 8) & 0xff) / 255.0f;
  float g = (float)((v >> 16) & 0xff) / 255.0f;
  float b = (float)((v >> 24) & 0xff) / 255.0f;

  return glm::vec4(r, g, b, a);
}

void testcolourconversion() {
  // test 1
  glm::vec4 v = glm::vec4(1.f, 0.f, 0.f, 1.f);
  uint32_t c = vec4tobyte(v);
  std::cout << "Test 1: " << std::hex << c << std::endl;

  uint32_t c2 = 0x00ff00ff;
  glm::vec4 v2 = bytetovec4(c2);
  std::cout << "Test 2: " << v2.r << " " << v2.g << " " << v2.b << " " << v2.a
            << std::endl;

  glm::vec4 v3 = glm::vec4(0.5f, 0.25f, 0.7f, 1.f);
  uint32_t c3 = vec4tobyte(v3);
  glm::vec4 v4 = bytetovec4(c3);
  std::cout << "Test 3: " << v4.r << " " << v4.g << " " << v4.b << " " << v4.a
            << std::endl;
}
