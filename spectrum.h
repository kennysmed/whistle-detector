#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "constants.h"

void spectrumInit(void);
void spectrumSetGain(uint8_t gainBits);
bool spectrumAcquire(int8_t *magnitude_dBfs);

#endif
