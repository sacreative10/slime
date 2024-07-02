#include <CashewLib/Application.h>
#include <tuple>
#include <CashewLib/EntryPoint.h>

#include <CashewLib/Image.h>
#include <iomanip>

#include <CashewLib/Input/Input.h>
#include <CashewLib/Input/KeyCodes.h>

#include "imgui_internal.h"
#include <glm/gtc/constants.hpp>

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

class Agent {
public:
  void sensoryStage(std::tuple<float, float, float> sensoryInfo, float stepSize, float rotationAngle);

  void move(float stepSize, glm::vec2 bounds) {
    float newPosx = position.x + stepSize * cos(glm::radians(direction));
    float newPosy = position.y + stepSize * sin(glm::radians(direction));

    // if (newPosx <= 0 || newPosx >= bounds.x) {
    //   // we flip the direction if we hit the bounds
    //   if (direction <= 90) {
    //     direction = 180 - direction;
    //   } else if (direction <= 180) {
    //     direction = 180 + (180 - direction);
    //   } else if (direction <= 270) {
    //     direction = 360 - direction;
    //   } else {
    //     direction = 360 - (360 - direction);
    //   }
    // }
    // if (newPosy <= 0 || newPosy >= bounds.y) {
    //   // bruteforce method to flip the direction
    //   if (direction <= 90) {
    //     direction = 270 + (90 - direction);
    //   } else if (direction <= 180) {
    //     direction = 270 - (180 - direction);
    //   } else if (direction <= 270) {
    //     direction = 90 + (270 - direction);
    //   } else {
    //     direction = 90 - (360 - direction);
    //   }
    // }

    // wrap around
    if (newPosx < 0) {
      newPosx = bounds.x - 1;
    } else if (newPosx >= bounds.x) {
      newPosx = 0;
    }
    if (newPosy < 0) {
      newPosy = bounds.y - 1;
    } else if (newPosy >= bounds.y) {
      newPosy = 0;
    }

    position = glm::vec2(newPosx, newPosy);
  }

public:
  glm::vec2 position;

  float direction; // in radians
};


class slime : public imageCreatorEntity {
public:
  slime(int width, int height): imageCreatorEntity(width, height) {
    time = 0.0f;
    trailMap = std::vector<float>(width * height, 0.0f);
    agents = std::vector<Agent>(100);
    for (Agent &agent: agents) {
      agent.position = glm::vec2(width / 2, height / 2);
      agent.direction = (rand() % 360);
    }
  }

  void requestImage() override {
    computeImage();
  }

  void computeImage() override;

  std::tuple<float, float, float> getSensor(glm::vec2 pos, float direction);


  float sensoryAngle = 22.5f;
  float rotationAngle = 45.0f;
  float sensoryOffsetDistance = 9.0f;
  float depositionRate = 1.f;
  float decayRate = 0.1f;

private:
  std::vector<float> trailMap;
  std::vector<Agent> agents;
};

void slime::computeImage() {
  m_Data = new uint32_t[m_imageWidth * m_imageHeight];

  // sensory phase
  for (Agent agent: agents) {
    auto sensorInfo = getSensor(agent.position, agent.direction);
    agent.sensoryStage(sensorInfo, 9.0f, rotationAngle);
  }

  // movement phase

  for (Agent &agent: agents) {
    agent.move(1.0f, glm::vec2(m_imageWidth - 1, m_imageHeight - 1));
    trailMap[(int) agent.position.y * m_imageWidth + (int) agent.position.x] += depositionRate;
  }
  //
  // diffusion phase
  // we start from one to avoid edge handling
  for (int y = 1; y < m_imageHeight - 1; y++) {
    for (int x = 1; x < m_imageWidth - 1; x++) {
      float sum = 0;
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          sum += trailMap[(y + dy) * m_imageWidth + (x + dx)];
        }
      }
      trailMap[y * m_imageWidth + x] = sum / 9;
    }
  }

  // decay phase
  for (int i = 0; i < m_imageWidth * m_imageHeight; i++) {
    trailMap[i] *= 1; // rather aggressive decay
  }

  // convert trail map to image
  for (int i = 0; i < m_imageWidth * m_imageHeight; i++) {
    m_Data[i] = vec4tobyte(glm::vec4(1.0f, 1.0f, 1.0f, std::min(1.f, trailMap[i])));
  }
}


std::tuple<float, float, float> slime::getSensor(glm::vec2 pos, float direction) {
  float leftAngle = direction - sensoryAngle;
  float rightAngle = direction + sensoryAngle;

  int leftDY = (int)
      (sensoryOffsetDistance * sin(glm::radians(leftAngle)));
  int leftDX = (int) (sensoryOffsetDistance * cos(glm::radians(leftAngle)));

  int rightDY = (int) (sensoryOffsetDistance * sin(glm::radians(rightAngle)));
  int rightDX = (int) (sensoryOffsetDistance * cos(glm::radians(rightAngle)));

  int centerDY = (int) (sensoryOffsetDistance * sin(glm::radians(direction)));
  int centerDX = (int) (sensoryOffsetDistance * cos(glm::radians(direction)));

  // TODO: talk about
  if (pos.x + leftDX < 0 || pos.x + leftDX >= m_imageWidth || pos.y + leftDY < 0 || pos.y + leftDY >= m_imageHeight) {
    leftDX = 0;
    leftDY = 0;
  }
  if (pos.x + rightDX < 0 || pos.x + rightDX >= m_imageWidth || pos.y + rightDY < 0 || pos.y + rightDY >=
      m_imageHeight) {
    rightDX = 0;
    rightDY = 0;
  }
  if (pos.x + centerDX < 0 || pos.x + centerDX >= m_imageWidth || pos.y + centerDY < 0 || pos.y + centerDY >=
      m_imageHeight) {
    centerDX = 0;
    centerDY = 0;
  }

  float leftSensor = trailMap[(int) (pos.y + leftDY) * m_imageWidth + (int) (pos.x + leftDX)];
  float rightSensor = trailMap[(int) (pos.y + rightDY) * m_imageWidth + (int) (pos.x + rightDX)];
  float centerSensor = trailMap[(int) (pos.y + centerDY) * m_imageWidth + (int) (pos.x + centerDX)];

  return std::make_tuple(leftSensor, centerSensor, rightSensor);
}


void Agent::sensoryStage(std::tuple<float, float, float> sensorInfo, float stepSize, float rotationAngle) {
  // from the trail map, we get the sensor information
  float leftSensor = std::get<0>(sensorInfo);
  float centerSensor = std::get<1>(sensorInfo);
  float rightSensor = std::get<2>(sensorInfo);

  if (centerSensor > leftSensor &&
      centerSensor > rightSensor) {
    // we stay on the same path
    direction += 0.f;
  } else if (leftSensor < centerSensor && rightSensor < centerSensor) {
    // we are at a crossroads, where either path is suitable but not the center
    // we choose a random direction
    if (rand() % 2 == 0) {
      direction += rotationAngle;
    } else {
      direction -= rotationAngle;
    }
  } else if (leftSensor < rightSensor) {
    // rotate right
    direction += rotationAngle;
  } else if (rightSensor < leftSensor) {
    // rotate left
    direction -= rotationAngle;
  } else
    direction += 0.f;
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
