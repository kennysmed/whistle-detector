#include <sstream> // for std::stringstream
#include "BERGCloudMbed.h"

#include "mbed.h"
#include "spectrum.h"
#include "oled.h"
#include "whistle.h"

// BERG Cloud Devshield comms defines
#define MOSI_PIN p5
#define MISO_PIN p6
#define SCLK_PIN p7
#define nSSEL_PIN p8

// BERG Cloud Project definitions
#define VERSION 0x0001
const uint8_t PROJECT_KEY[BC_KEY_SIZE_BYTES] =
  {0x96,0xCC,0xE7,0x73,0x75,0xD3,0x78,0x16,0x00,0x18,0xEF,0xE0,0xB5,0x0E,0x3F,0xA0};
  
// Only one event to emit so far
#define EVENT_WHISTLE_DETECTED 0x01

// Our instance to communicate with BERG Cloud
BERGCloudMbed BERGCloud;

// Use the leds to indicate state
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

int8_t spectrum[MAGNITUDE_BINS];

int main()
{
  stringstream display_text; // Used to format content for the OLED
  whistle_count = 0;
  led1 = 1;
  printf("--- start ---\r\n");

  // The usual initiation of SPI comms
  BERGCloud.begin(MOSI_PIN, MISO_PIN, SCLK_PIN, nSSEL_PIN);
  
  // The event we'll be sending up when a whistle is detected
  BERGCloudMessage event;
  
  if (BERGCloud.connect(PROJECT_KEY, VERSION)) {
    printf("Connected to network");
    led1 = 2;
  } else {
    printf("BERGCloud.connect() returned false.");
  }
  
  // Initialise our OLED driver and ADC pipeline
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
          whistle_count++;
          
          event.pack("COUNT"); // Just to indicate what we've captured
          event.pack(whistle_count);
          event.pack("STATE"); // Just to indicate what we've captured
          event.pack(led4);
          
          // Send the event object
          if (BERGCloud.sendEvent(EVENT_WHISTLE_DETECTED, event))  {
            display_text << "Whistle count ";
            display_text << whistle_count;
            BERGCloud.display(display_text.str().c_str());
            // Reset our sstr
            display_text.str("");
          }
        }
      }
      else
      {
        printf("spectrumAcquire() returned false.");
        return -1;
      }
  }
}
