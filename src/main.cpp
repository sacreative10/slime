#include <CashewLib/Application.h>
#include <tuple>
#include <CashewLib/EntryPoint.h>

#include <CashewLib/Image.h>
#include <iomanip>

#include <CashewLib/Input/Input.h>
#include <CashewLib/Input/KeyCodes.h>

#include "imgui_internal.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>

// helpers

uint32_t vec4tobyte(const glm::vec4 &v) {
  uint32_t r = (uint32_t) (v.r * 255.0f);
  uint32_t g = (uint32_t) (v.g * 255.0f);
  uint32_t b = (uint32_t) (v.b * 255.0f);
  uint32_t a = (uint32_t) (v.a * 255.0f);
  return (a << 24) | (b << 16) | (g << 8) | r;
}

glm::vec4 bytetovec4(uint32_t v) {
  float a = (float) (v & 0xff) / 255.0f;
  float r = (float) ((v >> 24) & 0xff) / 255.0f;
  float g = (float) ((v >> 16) & 0xff) / 255.0f;
  float b = (float) ((v >> 8) & 0xff) / 255.0f;


  return glm::vec4(r, g, b, a);
}

void testcolourconversion() {
  // test 1
  glm::vec4 v = glm::vec4(1.f, 0.f, 0.f, 1.f);
  uint32_t c = vec4tobyte(v);
  std::cout << "Test 1: " << std::hex << c << std::endl;

  uint32_t c2 = 0x00ff00ff;
  glm::vec4 v2 = bytetovec4(c2);
  std::cout << "Test 2: " << v2.r << " " << v2.g << " " << v2.b << " " << v2.a << std::endl;

  glm::vec4 v3 = glm::vec4(0.5f, 0.25f, 0.7f, 1.f);
  uint32_t c3 = vec4tobyte(v3);
  glm::vec4 v4 = bytetovec4(c3);
  std::cout << "Test 3: " << v4.r << " " << v4.g << " " << v4.b << " " << v4.a << std::endl;
}


class imageCreatorEntity {
public:
  imageCreatorEntity(uint32_t width, uint32_t height) : m_imageWidth(width), m_imageHeight(height) {
    m_Data = new uint32_t[width * height];
    time = 0.0f;
  }

  ~imageCreatorEntity() {
    delete[] m_Data;
  }

  virtual void requestImage() = 0;

  virtual void computeImage() = 0;

public:
  uint32_t *m_Data = nullptr;
  float time;
  uint32_t m_imageWidth;
  uint32_t m_imageHeight;
};

class basic : public imageCreatorEntity {
public:
  basic(uint32_t width, uint32_t height) : imageCreatorEntity(width, height) {
    time = 0.0f;
  }

public:
  void requestImage() override {
    computeImage();
  }

  void computeImage() override {
    time += 0.01f;
    for (int i = 0; i < m_imageWidth * m_imageHeight; i++) {
      m_Data[i] = 0x00ffffff;
    }
  }
};


class donut : public imageCreatorEntity {
public:
  donut(uint32_t width, uint32_t height) : imageCreatorEntity(width, height) {
    K1 = m_imageWidth * K2 * 3 / (8 * (R1 + R2));
    time = 0.0f;
  }

  void requestImage() override {
    computeImage();
  }

  void computeImage() override {
    // interpolate A and B as a function of time

    float *zbuffer = new float[m_imageWidth * m_imageHeight];

    m_Data = new uint32_t[m_imageWidth * m_imageHeight];

    float phi_spacing = 0.05f;
    float theta_spacing = 0.02f;

    for (float phi = 0; phi < glm::two_pi<float>(); phi += phi_spacing) {
      for (float theta = 0; theta < glm::two_pi<float>(); theta += theta_spacing) {
        glm::mat3 rot = glm::mat3(
                          glm::vec3(cos(phi), 0, sin(phi)),
                          glm::vec3(0, 1, 0), // y-axis
                          glm::vec3(-sin(phi), 0, cos(phi))
                        ) * glm::mat3(
                          glm::vec3(1, 0, 0),
                          glm::vec3(0, cos(A), sin(A)), // x-axis
                          glm::vec3(0, -sin(A), cos(A))
                        ) * glm::mat3(
                          glm::vec3(cos(B), sin(B), 0),
                          glm::vec3(-sin(B), cos(B), 0), // z-axis
                          glm::vec3(0, 0, 1)
                        );

        // technically we don't need to store the position, but it useful for debugging
        pos = glm::vec3(R2 + (R1 * cos(theta)), R1 * sin(theta), 0.0f) * rot;
        float ooz = 1.f / (pos.z + K2);

        // projecting x, y, to 2D
        int x = (m_imageWidth / 2) + K1 * ooz * pos.x;
        int y = (m_imageHeight / 2) - K1 * ooz * pos.y;

        // a simple check to see if the point is within the image, because we aren't guaranteed that the
        // donut will be within the image
        if (x < 0 || x >= m_imageWidth || y < 0 || y >= m_imageHeight) {
          continue;
        }

        // luminance is the dot product of the rotated normal and the direction of light, which defined as (0, 1, -1)
        // which is behind and above the camera
        float luminance = glm::dot((rot * glm::vec3(cos(theta), sin(theta), 0)),
                                   glm::normalize(glm::vec3(0, 1, -1)));

        int index = y * m_imageWidth + x;
        // check if the pixel is worth shading, and see if we are behind anything
        if (luminance > 0 && ooz > zbuffer[index]) {
          zbuffer[index] = ooz;
          // write the pixel
          m_Data[index] = vec4tobyte(glm::vec4(1.0f, 1.0f, 1.0f, luminance));
        }
      }
    }

    A = glm::two_pi<float>() * time * 0.3;
    B = glm::two_pi<float>() * time * 0.1;
  }

private:
  const float R1 = 1.0f; // radius of the donut
  const float R2 = 2.0f; // radius of the donut hole
  float K1;
  const float K2 = 5.0f; // constant scaling factor for the y-axis;


  glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
  float A = 0.0f;
  float B = 0.0f;
};


// subtraction will just be one of input's sign change
// otherwise adds two functions respecting the [0, 2pi] range
float angleAddition(float a, float b) {
  float result = a + b;
  if (result > glm::two_pi<float>()) {
    result -= glm::two_pi<float>();
  }
  if (result < 0) {
    result += glm::two_pi<float>();
  }
  return result;
}

int posWrap(int pos, int max) {
  if (pos < 0) {
    return max - 1;
  }
  if (pos > max) {
    return 0;
  }
  return pos;
}

void posWrap(glm::vec2 &pos, int width, int height) {
  // change the input position vector as well
  pos.x = posWrap((int) pos.x, width);
  pos.y = posWrap((int) pos.y, height);
}


class Agent {
public:
  void sensoryStage(std::tuple<float, float, float> sensoryInfo, float stepSize, float rotationAngle);

  void motorPhase(int imageWidth, int imageHeight, float stepSize);

public:
  glm::vec2 position;

  float direction; // in radians
};

void Agent::sensoryStage(std::tuple<float, float, float> sensoryInfo, float stepSize, float rotationAngle) {
  float leftSensorValue = std::get<1>(sensoryInfo);
  float rightSensorValue = std::get<0>(sensoryInfo);
  float frontSensorValue = std::get<2>(sensoryInfo);

  if (frontSensorValue > leftSensorValue && frontSensorValue > rightSensorValue) {
    direction = angleAddition(direction, 0);
  } else if (frontSensorValue < leftSensorValue && frontSensorValue < rightSensorValue) {
    if (rand() % 2 == 0) {
      direction = angleAddition(direction, rotationAngle);
    } else {
      direction = angleAddition(direction, -rotationAngle);
    }
  } else if (leftSensorValue < rightSensorValue) {
    direction = angleAddition(direction, rotationAngle);
  } else if (leftSensorValue > rightSensorValue) {
    direction = angleAddition(direction, -rotationAngle);
  }
}

void Agent::motorPhase(int imageWidth, int imageHeight, float stepSize) {
  glm::vec2 newPosition = position + (stepSize * glm::vec2(cos(direction), sin(direction)));
  posWrap(newPosition, imageWidth, imageHeight);
  position = newPosition;
}

// diffusion algorithm, takes in a 2D array and applies the diffusion algorithm
void diffusion(std::vector<std::vector<float> > &array, int width, int height, float decayRate) {
  std::vector<std::vector<float> > newArray = std::vector<std::vector<float> >(
    height + 1, std::vector<float>(width + 1, 0.0f));
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float value = array[y][x];
      float newValue = value * (1 - decayRate);
      float diffusion = value * decayRate / 8.0f;

      // apply diffusion
      if (y > 0) {
        newArray[y][x] += diffusion;
      }
      if (x < width - 1) {
        newArray[y + 1][x] += diffusion;
      }
      if (x > 0) {
        newArray[y][x - 1] += diffusion;
      }
      if (y < height - 2) {
        newArray[y][x + 1] += diffusion;
      }

      newArray[y][x] += newValue;
    }
  }

  array = newArray;
}


class slime : public imageCreatorEntity {
public:
  slime(int width, int height): imageCreatorEntity(width, height) {
    time = 0.0f;
    trailMap = std::vector<std::vector<float> >(height + 1, std::vector<float>(width + 1));
    agents = std::vector<Agent>(1000);
    for (Agent &agent: agents) {
      int randPosx = rand() % width;
      int randPosy = rand() % height;
      agent.position = glm::vec2(randPosx, randPosy);
      agent.direction = glm::linearRand(0.0f, glm::two_pi<float>());
    }
  }

  void requestImage() override {
    computeImage();
  }

  void computeImage() override;

  std::tuple<float, float, float> getSensor(glm::vec2 pos, float direction);


  float sensoryAngle = glm::radians(45.f);
  float rotationAngle = glm::radians(45.0f);
  float sensoryOffsetDistance = 9.0f;
  float stepSize = 1.f;
  float depositionRate = 5.f;
  float decayRate = 0.1f;

private:
  std::vector<std::vector<float> > trailMap;
  std::vector<Agent> agents;
};

void slime::computeImage() {
  m_Data = new uint32_t[m_imageWidth * m_imageHeight];

  // sensory phase

  // Get sensor information for each agent
  for (Agent &agent: agents) {
    auto sensoryInfo = getSensor(agent.position, agent.direction);
    agent.sensoryStage(sensoryInfo, 1.0f, rotationAngle);
  }


  // motor phase
  for (Agent &agent: agents) {
    agent.motorPhase(m_imageWidth, m_imageHeight, stepSize);
    trailMap[agent.position.y][agent.position.x] += depositionRate;
  }

  diffusion(trailMap, m_imageWidth, m_imageHeight, decayRate);

  // finally convert the trail map to an image
  for (int y = 0; y < m_imageHeight; y++) {
    for (int x = 0; x < m_imageWidth; x++) {
      float value = trailMap[y][x];
      m_Data[y * m_imageWidth + x] = vec4tobyte(glm::vec4(value, value, value, 1.0f));
    }
  }
}

std::tuple<float, float, float> slime::getSensor(glm::vec2 pos, float direction) {
  float rightAngle = angleAddition(direction, sensoryAngle);
  float leftAngle = angleAddition(direction, -sensoryAngle);
  float frontAngle = direction;

  glm::vec2 rightSensorPos = pos + glm::vec2(cos(rightAngle), sin(rightAngle));
  glm::vec2 leftSensorPos = pos + glm::vec2(cos(leftAngle), sin(leftAngle));
  glm::vec2 frontSensorPos = pos + glm::vec2(cos(frontAngle), sin(frontAngle));

  posWrap(rightSensorPos, m_imageWidth, m_imageHeight);
  posWrap(leftSensorPos, m_imageWidth, m_imageHeight);
  posWrap(frontSensorPos, m_imageWidth, m_imageHeight);

  float rightSensorValue = trailMap[(int) rightSensorPos.y][(int) rightSensorPos.x];
  float leftSensorValue = trailMap[(int) leftSensorPos.y][(int) leftSensorPos.x];
  float frontSensorValue = trailMap[(int) frontSensorPos.y][(int) frontSensorPos.x];


  return std::make_tuple(rightSensorValue, leftSensorValue, frontSensorValue);
}


class WindowLayer : public Cashew::Layer {
public:
  WindowLayer() : Layer() {
    m_basic = new slime(1040, 720);
    m_Image = std::make_shared<Cashew::Image>(1040, 720, Cashew::ImageFormat::RGBA);
  }

  virtual void onUIRender() override {
    ImGui::Begin("Observation Window");
    ImGui::SetWindowSize(ImVec2(1040, 720));
    m_basic->requestImage();
    m_Data = m_basic->m_Data;
    m_Image->setData(m_Data);
    ImGui::Image((void *) m_Image->getDescriptorSet(), ImVec2(1040, 720));
    ImGui::End();
    // panel
    if (m_shouldPanelSeen) {
      ImGui::Begin("Panel");
      ImGui::SetWindowSize(ImVec2(320, 720));
      if (ImGui::Button("Hide")) {
        m_shouldPanelSeen = false;
      }
      ImGui::End();
    }
  }

  virtual void onUpdate(float dt) override {
    m_basic->time += dt;


    // input handling
    if (Cashew::Input::isKeyDown(Cashew::KeyCode::Escape)) {
      Cashew::Application::Get().Close();
    } else if (Cashew::Input::isKeyPressed(Cashew::KeyCode::Tab)) {
      float currentTime = Cashew::Application::Get().GetTime();
      static float lastTime = 0.0f;
      if (currentTime - lastTime > 0.3f) {
        m_shouldPanelSeen = !m_shouldPanelSeen;
        lastTime = currentTime;
      }
    }
  }

private:
  std::shared_ptr<Cashew::Image> m_Image;
  uint32_t *m_Data = nullptr;
  imageCreatorEntity *m_basic;

  bool m_shouldPanelSeen = true;
};

Cashew::Application *Cashew::CreateApplication(int argc, char **argv) {
  Cashew::ApplicationSpecification spec;
  spec.Name = "Physarum";
  spec.Width = 1040;
  spec.Height = 760;

  Cashew::Application *app = new Cashew::Application(spec);
  app->PushLayer<WindowLayer>();
  return app;
}
