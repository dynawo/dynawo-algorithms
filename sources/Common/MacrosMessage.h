//
// Copyright (c) 2015-2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//
// This file is part of Dynawo, an hybrid C++/Modelica open source suite
// of simulation tools for power systems.
//

/**
 * @file MacrosMessage.h
 *
 * @brief definition of macro to add log/error....
 */

#ifndef COMMON_MACROSMESSAGE_H_
#define COMMON_MACROSMESSAGE_H_

#include "DYNMessage.hpp"
#include "DYNError.h"
#include "DYNAlgorithmsError_keys.h"
#include "DYNAlgorithmsLog_keys.h"

namespace DYNAlgorithms {

/**
 * @brief Macro description to have a shortcut.
 *  Thanks to this macro, user can only call an error with the type and the key to access
 *  to the message (+ optional arguments if the message need)
 *  File error localization and line error localization are added
 *
 * @param key  key of the message description
 *
 * @return an Error
 */
#define DYNAlgorithmsError(key, ...) \
          DYN::Error(DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::key, std::string(__FILE__), __LINE__, \
                     (DYN::Message("AlgorithmsERROR", DYNAlgorithms::KeyAlgorithmsError_t::names(DYNAlgorithms::KeyAlgorithmsError_t::key)), ##__VA_ARGS__))


/**
 * @brief Macro description to have a shortcut.
 *  Thanks to this macro, user can only call a log message with the key to access
 *  to the message (+ optional arguments if the message need)
 *
 * @param key  key of the message description
 *
 * @return a log message
 */
#define DYNAlgorithmsLog(key, ...) \
          (DYN::Message("AlgorithmsLOG", DYNAlgorithms::KeyAlgorithmsLog_t::names(DYNAlgorithms::KeyAlgorithmsLog_t::key)), ##__VA_ARGS__)

}  // namespace DYNAlgorithms

#endif  // COMMON_MACROSMESSAGE_H_
