#include <ripple/glow/window/window.hpp>

auto run_application() -> void {
  auto*                engine = ripple::glow::Engine::create();
  ripple::glow::Window window(engine, "glow", 512, 512);

  // auto* renderer = window.renderer();
  // auto* renderer = engine.renderer();
  while (window.is_alive()) {
    window.poll_input();

    // Do some event handling

    auto* driver = engine->driver();
    driver->begin_frame(*(engine->platform()));

    auto cmd =
      driver
        ->request_command_buffer<ripple::glow::CommandBufferKind::graphics>();

    driver->submit(cmd);

    driver->end_frame(*(engine->platform()));
  }
}

auto main(int argc, const char* argv[]) -> int {
  run_application();
}