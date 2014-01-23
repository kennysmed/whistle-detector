/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 32000 Hz

fixed point precision: 15 bits

* 0 Hz - 3000 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 3.32 dB

* 4000 Hz - 16000 Hz
  gain = 0
  desired attenuation = -30 dB
  actual attenuation = -31.93 dB

*/

#define FILTER_TAP_NUM 29

static const q15_t filter_taps[FILTER_TAP_NUM] = {
  767,
  817,
  922,
  739,
  230,
  -506,
  -1243,
  -1677,
  -1524,
  -634,
  935,
  2901,
  4821,
  6218,
  6729,
  6218,
  4821,
  2901,
  935,
  -634,
  -1524,
  -1677,
  -1243,
  -506,
  230,
  739,
  922,
  817,
  767
};
