//==--- snowflake/util/portability.hppw -------------------- -*- C++ -*- ---==//
//
//                            Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  portability.hpp
/// \brief This file defines portability funcationality.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_UTIL_PORTABILITY_HPP
#define SNOWFLAKE_UTIL_PORTABILITY_HPP

#if __cplusplus == 201703L
  #define snowflake_nodiscard [[nodiscard]] // NOLINT
#else
  #define snowflake_nodiscard
#endif

#endif // SNOWFLAKE_UTIL_PORTABILITY_HPP