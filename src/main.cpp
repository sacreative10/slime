#include <CashewLib/Application.h>
#include <CashewLib/EntryPoint.h>

#include <CashewLib/Image.h>
#include <iomanip>

#include <CashewLib/Input/Input.h>
#include <CashewLib/Input/KeyCodes.h>

#include "imgui_internal.h"

class WindowLayer : public Cashew::Layer {
public:
  WindowLayer() : Layer() {
    m_Data = new uint32_t[1040 * 720];
    for (int i = 0; i < 1040 * 720; i++) {
      m_Data[i] = 0xff0000ff;
    }
    m_Image = std::make_shared<Cashew::Image>("../cat.jpeg");
  }
  virtual void onUIRender() override {
    ImGui::Begin("Observation Window");
    ImGui::SetWindowSize(ImVec2(1040, 720));
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
  uint32_t* m_Data = nullptr;

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
