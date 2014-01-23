#include "mbed.h"
#include "constants.h"

#define MIN_BIN 12
#define ADJACENT_BINS 2
#define THRESHOLD_DB 20
#define WHISTLE_TICKS 5
#define HOLDOFF_TICKS 10

static bool holdoff;
static uint32_t tickCount;
static uint8_t peakBin;

static bool peakDetected(int8_t *spectrum)
{
  int8_t peakMax = -128;
  int8_t nextMax = -128;
  uint8_t i;
  
  /* Find the peak bin, exclude bins below MIN_BIN */
  peakBin = 0;
  for (i = MIN_BIN; i < MAGNITUDE_BINS; i++)
  {
    if (spectrum[i] > peakMax)
    {
      peakMax = spectrum[i];
      peakBin = i;
    }
  }
  
  /* Find the next highest peak, excluding peak bin and adjacent bins. */  
  if (peakBin > ADJACENT_BINS)
  {
    /* Lower frequency bins. Exclude bin 0 (DC). */
    for (i = 1; i < (peakBin - ADJACENT_BINS); i++)
    {
      if (spectrum[i] > nextMax)
      {
        nextMax = spectrum[i];
      }
    }       
  }
  
  if (peakBin < (MAGNITUDE_BINS - ADJACENT_BINS - 1))
  {
    /* Higher frequency bins */
    for (i = (peakBin + ADJACENT_BINS + 1); i < MAGNITUDE_BINS; i++)
    {
      if (spectrum[i] > nextMax)
      {
        nextMax = spectrum[i];
      }
    }       
  }   

  /* peakMax excludes bins < MIN_BIN, so nextMax could be higher. */
  if (peakMax > nextMax)
  {
    if ((peakMax - nextMax) > THRESHOLD_DB)
    {
      /* The peak level is at least THRESHOLD_DB above the next highest level. */
      return true;
    }
  }
  
  return false;
}

bool whistleDetected(int8_t *spectrum, uint16_t *whistleFrequency)
{
  bool result = false;
 
  if (peakDetected(spectrum))
  {
    if (holdoff)
    {
      /* Restart holdoff period */
      tickCount = 0;
    }
    else
    {
      tickCount++;
    
      if (tickCount > WHISTLE_TICKS)
      {
        /* Whistle detected. Start holdoff period. */
        result = true;
        holdoff = true;
        tickCount = 0;
      }
    }
  }
  else
  {
    if (holdoff)
    {
      tickCount++;
      
      if (tickCount > HOLDOFF_TICKS)
      {
        /* No whistle detected. Start detection. */
        holdoff = false;
        tickCount = 0;
      }
    }
    else
    {
      /* Restart whistle detection period */
      tickCount = 0;
    }
  }
  
  if (result)
  {
    if (whistleFrequency != NULL)
    {
      *whistleFrequency = peakBin * HZ_PER_BIN;
    }
  }
  
  return result;
}
