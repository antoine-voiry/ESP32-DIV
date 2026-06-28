#pragma once

struct IDisplay;
struct IADC;
struct IEeprom;
struct IStorage;
struct ISubGhzRadio;
struct INrfRadio;

extern IDisplay*     gDisplay;
extern IADC*         gADC;
extern IEeprom*      gEeprom;
extern IStorage*     gStorage;
extern ISubGhzRadio* gSubGhz;
extern INrfRadio*    gNrf1;
extern INrfRadio*    gNrf2;
extern INrfRadio*    gNrf3;
