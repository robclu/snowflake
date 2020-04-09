#include <ripple/glow/vk/window/window.hpp>

auto run_application() -> void {
  ripple::glow::vk::Window window("glow", 512, 512);

  while (window.is_alive()) {
    window.poll_input();
  }
}

auto main(int argc, const char* argv[]) -> int {
  run_application();
}