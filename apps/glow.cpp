#include <ripple/glow/window/window.hpp>

auto run_application() -> void {
  auto*                engine = ripple::glow::Engine::create();
  ripple::glow::Window window(engine, "glow", 512, 512);

  // auto* renderer = window.renderer();
  // auto* renderer = engine.renderer();
  while (window.is_alive()) {
    window.poll_input();

    // Do some event handling

    engine->driver()->begin_frame(*(engine->platform()));
  }
}

auto main(int argc, const char* argv[]) -> int {
  run_application();
}