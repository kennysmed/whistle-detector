#include "mbed.h"

SPI spi(p11, p12, p13); // mosi, miso, sclk
DigitalOut cs(p30);
DigitalOut rs(p9);
DigitalOut rst(p10);

const uint8_t oledInitData[] = {
  0xae,  /* turn off oled panel */
  0xd5,  /* set display clock divide ratio/oscillator frequency */
  0x80,  /* set divide ratio */
  0xa8,  /* set multiplex ratio(1 to 64) */
  0x1f,  /* 1/32 duty */
  0xd3,  /* set display offset */
  0x00,  /* not offset */
  0x8d,  /* set Charge Pump enable/disable */
  0x14,  /* set(0x10) disable */
  0x40,  /* set start line address */
  0xa6,  /* set normal display */
  0xa4,  /* disable Entire Display On */
  0xa0,  /* set segment re-map */
  0xc0,  /* set COM Output Scan Direction */
  0xda,  /* set com pins hardware configuration */
  0x42,
  0x81,  /* set contrast control register */
  0x80,  /* contrast */
  0xd9,  /* set pre-charge period */
  0xf1,
  0xdb,
  0x40,  /* set vcomh */
};

static void writeInstruction(uint8_t txData)
{
  rs = 0;
  cs = 0;
  spi.write(txData);
  cs = 1;
}

static void writeData(uint8_t txData)
{
  rs = 1;
  cs = 0;
  spi.write(txData);
  cs = 1;
}

static void setPageAddress(uint8_t add)
{
  // Set page address 0~15
  add = 0xb0|add;
  writeInstruction(add);
}

static void setColumnAddress(uint8_t add)
{
  writeInstruction((0x10|(add>>4)));
  writeInstruction((0x0f&add));
}

static void enableDisplay(void)
{
  writeInstruction(0xaf);
}

static void displayClear(void)
{
  uint8_t i,j;
  for(i=0; i<4; i++)
  {
    setPageAddress(i);
    setColumnAddress(0x00);
    for(j=0; j<128; j++)
    {
      writeData(0x00);
    }
  }
}

void oledInit(void)
{
  uint32_t i;
  
  cs = 1;
  rst = 0;   
  spi.format(8,3);
  spi.frequency(1000000);
  
  wait(0.01);
  
  /* Out of reset */
  rst = 1;
  wait(0.1);
    
  /* Send initialisation commands */
  for (i=0; i<sizeof(oledInitData); i++)
  {
    writeInstruction(oledInitData[i]);
  }
  
  displayClear();
  enableDisplay();    
}

void oledPlotSpectrum(int8_t *spectrum)
{
  uint8_t i,j,d,t,b;
  int8_t n;
   
  t = 0; /* Value the current oled line represents */
    
  for (i = 0; i < 4; i++)
  {
    setPageAddress(i);
    setColumnAddress(0);
        
    for (j = 0; j < 128; j+=2)  
    {  
      n = spectrum[j/2]; /* value in dbFs */
      d = 0; /* byte to write to oled */
        
      /* Change the range to be 0 to 31 */
        
      n = -n/2;
        
      if (n > 31)
      {
        n = 31;
      }
        
      for (b = 0; b < 8; b++)
      {
        /* For each bit in d */
        d <<= 1;
        if ((t+(7-b)) >= n)
        {
          /* Level is as high or higher than this line */
          d |= 1;
        }
      }
           
      /* Two columns per spectrum bin */
      writeData(d);
      writeData(d);
    }
    
    /* Next eight lines */
    t += 8;
  } 
}   
    
