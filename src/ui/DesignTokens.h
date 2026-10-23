// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QColor>
namespace reverie::design {
inline const QColor shell{"#242321"}, surface{"#33312D"}, control{"#4A453E"}, border{"#777065"};
inline const QColor text{"#EDE5D8"}, secondary{"#BEB2A4"}, paper{"#E8E1D5"}, ink{"#302D28"},
    mutedInk{"#6A6258"}, grid{"#CFC5B7"}, selection{"#D4A67A"}, signal{"#7EA89B"},
    record{"#BC655C"}, error{"#C7483F"};
inline constexpr int transportHeight = 56, headerWidth = 188, trackHeight = 82, controlHeight = 26,
                     radius = 2;
} // namespace reverie::design
