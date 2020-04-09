#include <iostream>
#include <ripple/glow/vk/window/window.hpp>

auto run_application(ripple::glow::vk::Window& window) -> void {
  while (window.is_alive()) {
    window.poll_input();
  }
}

auto main(int argc, const char* argv[]) -> int {
  ripple::glow::vk::Window window("glow", 512, 512);

  run_application(window);
}