#include "mbed.h"
#include "spectrum.h"
#include "oled.h"
#include "whistle.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

int8_t spectrum[MAGNITUDE_BINS];

int main()
{
  led1 = 1;
  printf("--- start ---\r\n");
  
  oledInit();
  spectrumInit();
  
  while(1)
  {    
      if (spectrumAcquire(spectrum))
      {
        oledPlotSpectrum(spectrum);

        if (whistleDetected(spectrum))
        {
          /* Toggle LED */
          led4 = !led4;
        }
      }
      else
      {
        printf("spectrumAcquire() returned false.");
        return -1;
      }
  }
}
