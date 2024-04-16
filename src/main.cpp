#include <CashewLib/Application.h>
#include <CashewLib/EntryPoint.h>

#include <CashewLib/Image.h>
#include <iomanip>

#include <CashewLib/Input/Input.h>
#include <CashewLib/Input/KeyCodes.h>

class ExampleLayer : public Cashew::Layer {
public:
  virtual void onUIRender() override {
    ImGui::Begin("Example Layer");
    ImGui::SetWindowSize(ImVec2(720, 1040));
    ImGui::End();
  }
  // add input handling
  virtual void onUpdate(float dt) override {
    if (Cashew::Input::isKeyDown(Cashew::KeyCode::Tab)){}
    else if (Cashew::Input::isKeyDown(Cashew::KeyCode::Escape)) {
      Cashew::Application::Get().Close();
    }
  }
};

Cashew::Application *Cashew::CreateApplication(int argc, char **argv) {
  Cashew::ApplicationSpecification spec;
  spec.Name = "Cashew Example";
  spec.Width = 720;
  spec.Height = 1040;

  Cashew::Application *app = new Cashew::Application(spec);
  app->PushLayer<ExampleLayer>();
  return app;
}
