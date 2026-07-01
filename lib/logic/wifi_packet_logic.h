// lib/logic/wifi_packet_logic.h
// Pure C++ — no Arduino or hardware headers.
#pragma once
#include <stdint.h>
#include <stddef.h>

// Returns the Y-axis scale factor for the packet monitor bar graph.
// Scans counts[0..len-1] for the maximum value.
// maxVal initialized to 1 (not 0): prevents divide-by-zero on all-zero input
// and keeps scale at 1.0 when all counts fit within displayHeight.
// Returns displayHeight/max when max > displayHeight, else 1.0.
double computePacketGraphScale(const uint32_t* counts, size_t len, double displayHeight);
