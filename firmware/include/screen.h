#pragma once

#include <Arduino.h>

#include "input.h"

constexpr const char *SOUPBOY_BUILD_NAME = "SoupBoy OS v0.2";

void screenBegin();
void screenShowBoot();
void screenUpdate(InputEvent event);
const char *screenBuildName();
