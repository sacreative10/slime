#include <CashewLib/Application.h>
#include <CashewLib/EntryPoint.h>

#include <CashewLib/Image.h>
#include <iomanip>

#include <CashewLib/Input/Input.h>
#include <CashewLib/Input/KeyCodes.h>

#include "imgui_internal.h"


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
      m_Data[i] = 0xff0000ff;
    }
  }
};


class pathtracer : public imageCreatorEntity {
public:
  pathtracer(uint32_t width, uint32_t height) : imageCreatorEntity(width, height) {
    time = 0.0f;
  }

public:
  void requestImage() override {
    computeImage();
  }
  struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
  };

  Ray generateCameraRay(float x, float y) {
    Ray ray;

    float focalLength = 1.0f;
    float viewportHeight = 2.0f;
    float viewportWidth = viewportHeight * (float(m_imageWidth) / float(m_imageHeight));
    glm::vec3 cameraCenter = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 viewport_u = glm::vec3(viewportWidth, 0.0f, 0.0f);
    glm::vec3 viewport_v = glm::vec3(0.0f, -viewportHeight, 0.0f);

    glm::vec3 pixelDeltax = viewport_u / (float)m_imageWidth;
    glm::vec3 pixelDeltay = viewport_v / (float)m_imageHeight;

    glm::vec3 viewport_upper_left = cameraCenter - glm::vec3(0, 0, focalLength) - (viewport_u/2.0f) - (viewport_v/2.0f);

    glm::vec3 pixel00_loc = viewport_upper_left + 0.5f * (pixelDeltax + pixelDeltay);

    glm::vec3 pixel_loc = pixel00_loc + (y * pixelDeltax) + (x * pixelDeltay);

    glm::vec3 ray_direction = pixel_loc - cameraCenter;

    ray.origin = cameraCenter;
    ray.direction = ray_direction;

    return ray;
  }

  bool Scene(Ray& ray) {
    // interesection with sphere
    glm::vec3 sphere_center = glm::vec3(0.0f, 0.0f, -1.0f);
    float radius = 0.5f;


    glm::vec3 oc = sphere_center - ray.origin;
    float a = glm::dot(ray.direction, ray.direction);
    float b = -2.0f * glm::dot(ray.direction, oc);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    return (discriminant >= 0);
  }

  uint32_t color(glm::vec3 col) {
    uint32_t colour = 0xff000000;
    colour += (uint32_t)(col.x * 255.99f);
    colour += (uint32_t)(col.y * 255.99f) << 8;
    colour += (uint32_t)(col.z * 255.99f) << 16;

    return colour;

  }


  void computeImage() override {
    // do stuff here
    for (int i = 0; i < m_imageHeight; i++) {
      for (int j = 0; j < m_imageWidth; j++) {
        Ray ray = generateCameraRay(i, j);

        if (Scene(ray)) {
          m_Data[i * m_imageWidth + j] = 0xff0000ff;
          continue;
        }

        glm::vec3 unit_direction = glm::normalize(ray.direction);
        float a = 0.5f * (unit_direction.y + 1.0f);
        glm::vec3 col = (1.0f - a) * glm::vec3(1.0f, 1.0f, 1.0f) + a * glm::vec3(0.5f, 0.7f, 1.0f);

        m_Data[i * m_imageWidth + j] = color(col);

      }
    }

  }
};


class WindowLayer : public Cashew::Layer {
public:
  WindowLayer() : Layer() {
    m_basic = new pathtracer(1040, 720);
    m_Image = std::make_shared<Cashew::Image>("../cat.jpeg");
  }
  virtual void onUIRender() override {
    ImGui::Begin("Observation Window");
    ImGui::SetWindowSize(ImVec2(1040, 720));
    m_basic->requestImage();
    m_Image->setData(m_basic->m_Data);
    ImGui::Image((void *)m_Image->getDescriptorSet(), ImVec2(1040, 720));
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
  // add input handling
  virtual void onUpdate(float dt) override {
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
  pathtracer *m_basic;

  bool m_shouldPanelSeen = true;
};

Cashew::Application *Cashew::CreateApplication(int argc, char **argv) {
  Cashew::ApplicationSpecification spec;
  spec.Name = "Physarum";
  spec.Width = 1040;
  spec.Height = 720;

  Cashew::Application *app = new Cashew::Application(spec);
  app->PushLayer<WindowLayer>();
  return app;
}
