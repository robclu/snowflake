//==--- snwoflake/renderer/renderer.hpp -------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  renderer.hpp
/// \brief This file defines a renderer.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RENDERER_RENDERER_HPP
#define SNOWFLAKE_RENDERER_RENDERER_HPP

namespace snowflake {

/// The Renderer represents an object which can be used to generates objects
/// which can be drawn to a window.
///
/// The typical usage of the renderer is:
///
/// ~~~{.cpp}
/// void render_loop(Renderer* renderer) {
///     renderer->begin_frame();
///     renderer->render();
///     renderer->end_frame();
///   }
/// }
/// ~~~
class Renderer {};

} // namespace snowflake

#endif // SNOWFLAKE_RENDERER_RENDERER_HPP
