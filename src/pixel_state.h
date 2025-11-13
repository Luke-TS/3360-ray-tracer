#pragma once
#include "color.h"

struct PixelState {
    color sum = color(0,0,0);
    int   samples = 0;
};
