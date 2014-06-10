#include "mbed.h"
#include "arm_math.h"
#include "constants.h"
#include "adc.h"
#include "plot.h"
#include "window_128_q15.h"
#include "decibel.h"
#include "filter_lowpass.h"

//#define DEBUG_PLOT

#ifdef DEBUG_PLOT
#define PLOT_Q15(...) plot_q15(__VA_ARGS__)
#define PLOT_INT8(...) plot_int8(__VA_ARGS__)
#else
#define PLOT_Q15(...)
#define PLOT_INT8(...)
#endif

/* Check math lib will build correctly */
#if !defined(ARM_MATH_CM0) && !defined(ARM_MATH_CM3) && !defined(ARM_MATH_CM4)
#error
#endif

static volatile uint32_t samples_adc[ACQ_SIZE_SAMPLES];
static q15_t samples_q15[ACQ_SIZE_SAMPLES];
static q15_t data_q15[DATA_SIZE_SAMPLES];
static q15_t cplx_fft_q15[DATA_SIZE_SAMPLES * 2];
static q15_t magnitude_q15[MAGNITUDE_BINS];
static uint8_t _gainBits;

/* Decimator state */
arm_fir_decimate_instance_q15 decimatorInstance;
q15_t decimatorState[FILTER_TAP_NUM + ACQ_SIZE_SAMPLES -1];

/* FFT state */
arm_rfft_instance_q15 RFFTInstance;
arm_cfft_radix4_instance_q15 CFFTInstance;

void spectrumInit(void)
{
  _gainBits = 0;

  /* Init ADC and DMA */
  adc_init();
}

void spectrumSetGain(uint8_t gainBits)
{
  _gainBits = gainBits;
}
    
bool spectrumAcquire(int8_t *magnitude_dBfs)
{
  /* Acquire samples from the ADC */
  if (!adc_acquire(samples_adc, ACQ_SIZE_SAMPLES))
  {
    printf("adc_acquire() failed.\r\n");
    return false;
  }

  /* Convert to 1.15 */
  adc_to_q15(samples_adc, samples_q15, ACQ_SIZE_SAMPLES);

  /* Plot samples */
  PLOT_Q15(samples_q15, ACQ_SIZE_SAMPLES, "/local/adc.dat");

  /* Init decimator */
  if (arm_fir_decimate_init_q15(&decimatorInstance,
                                FILTER_TAP_NUM,
                                DECIMATION_FACTOR,
                                (q15_t *)filter_taps,
                                decimatorState,
                                ACQ_SIZE_SAMPLES) != ARM_MATH_SUCCESS)
  {
    printf("arm_fir_decimate_init_q15() failed.\r\n");
    return false;
  }
  
  /* Low-pass filter and decimate */
  arm_fir_decimate_q15( &decimatorInstance,
                        samples_q15,
                        data_q15,
                        ACQ_SIZE_SAMPLES);
 
  /* Plot downsampled data */
  PLOT_Q15(data_q15, DATA_SIZE_SAMPLES, "/local/data.dat");
  
  /* Apply gain */
  if (_gainBits > 0)
  {
    arm_shift_q15(data_q15, _gainBits, data_q15, DATA_SIZE_SAMPLES);
  }

  /* Apply window */
  arm_mult_q15(data_q15, window_q15(), data_q15, DATA_SIZE_SAMPLES);

  /* Plot window and windowed data */
  PLOT_Q15(window_q15(), DATA_SIZE_SAMPLES, "/local/window.dat");
  PLOT_Q15(data_q15, DATA_SIZE_SAMPLES, "/local/datawin.dat");
  
  /* Init FFT */
  if (arm_rfft_init_q15(&RFFTInstance,
                        &CFFTInstance,
                        DATA_SIZE_SAMPLES,
                        0 /* Forward */,
                        1 /* Bit reverse */) != ARM_MATH_SUCCESS)
  {
    printf("arm_rfft_init_q15() failed.\r\n");
    return false;
  }

  /* FFT (real input, complex output) */
  arm_rfft_q15(&RFFTInstance, data_q15, cplx_fft_q15);

  /*
    The FFT output is a vector of complex values in 7.9 format.

    a) Scale the FFT output so that for a real, sinewave input the magnitude in the
       frequency domain will be the same as the time domain input signal.
       
       Scale by 2/N. Here N = 128, so shift right by 6 bits.
       
    b) Convert to 1.15 format. Shift left by 6 bits.
    
    The bit shifts in a) and b) cancel out so there is nothing to do.
  */
    
  /* Plot the scaled FFT - note they are complex numbers */
  PLOT_Q15(cplx_fft_q15, DATA_SIZE_SAMPLES * 2, "/local/cplxfft.dat");
 
  /* 
     Get the magnitude of the complex values. Discard the second half of the FFT
     result, keeping the 'positive frequency' part. This is accounted for by the
     factor of 2 in the scaling above.
  */

  arm_cmplx_mag_q15(cplx_fft_q15, magnitude_q15, MAGNITUDE_BINS);
 
  /*
    The magnitude values are in 2.14 format. Shift left one bit to get 1.15
  */

  arm_shift_q15(magnitude_q15, 1, magnitude_q15, MAGNITUDE_BINS);
  
  /* Plot the magnitudes */
  PLOT_Q15(magnitude_q15, MAGNITUDE_BINS, "/local/mag.dat");

  /* Convert to dBfs */
  q15_to_dBfs(magnitude_q15, magnitude_dBfs, MAGNITUDE_BINS);
  
  /* Plot magnitude in dB fs */
  PLOT_INT8(magnitude_dBfs, MAGNITUDE_BINS, "/local/magdbfs.dat");

  /* Success */
  return true;
}
