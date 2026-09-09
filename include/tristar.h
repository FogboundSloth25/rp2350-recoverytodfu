#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef TRISTAR_PIN
#define TRISTAR_PIN 3
#endif

#ifndef TRISTAR_OE_PIN
#define TRISTAR_OE_PIN 19
#endif

void tristar_init(void);
void tristar_task(void);
bool tristar_transition_active(void);
bool tristar_dfu_requested(void);
