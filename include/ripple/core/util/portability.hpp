//==--- ripple/core/util/portability.hppw ------------------ -*- C++ -*- ---==//
//
//                            Ripple - Core
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  portability.hpp
/// \brief This file defines portability funcationality.
//
//==------------------------------------------------------------------------==//

#ifndef RIPPLE_CORE_UTIL_PORTABILITY_HPP
#define RIPPLE_CORE_UTIL_PORTABILITY_HPP

#if __cplusplus == 201703L
  #define ripple_no_discard [[nodiscard]]
#else
  #define ripple_no_discard
#endif

#endif // RIPPLE_CORE_UTIL_PORTABILITY_HPP