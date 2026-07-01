#pragma once
#include <stdbool.h>

// Returns 0=OK when chip responds to register readback, 1=NOT_DETECTED otherwise.
int computeCC1101Status(bool chipDetected);
