#include "mbed.h"
#include "arm_math.h"

#ifdef LPC1768

// Write data to the local filesystem
LocalFileSystem local("local");

void plot_q15(q15_t *data, uint32_t dataSize, const char *fileName)
{
  FILE *fp;
  float32_t data_f32;
  
  fp = fopen(fileName, "w");
  
  if (fp != NULL)
  {
    printf("Writing: %s\r\n", fileName);
    
    while (dataSize-- > 0)
    {
      /* Convert to float */
      arm_q15_to_float(data++, &data_f32, 1);
      fprintf(fp, "%f\r\n", data_f32);
    }

    fclose(fp);
  }
  else
  {
    printf("Failed to open: %s\r\n", fileName);
  }
}

void plot_int8(int8_t *data, uint32_t dataSize, const char *fileName)
{
  FILE *fp;
  
  fp = fopen(fileName, "w");
  
  if (fp != NULL)
  {
    printf("Writing: %s\r\n", fileName);
    
    while (dataSize-- > 0)
    {
      fprintf(fp, "%i\r\n", *data++);
    }

    fclose(fp);
  }
  else
  {
    printf("Failed to open: %s\r\n", fileName);
  }
}

#else

void plot_q15(q15_t *data, uint32_t dataSize, const char *fileName)
{
  float32_t data_f32;
  printf("Filename: %s\r\n", fileName);
    
  while (dataSize-- > 0)
  {
    /* Convert to float */
    arm_q15_to_float(data++, &data_f32, 1);
    printf("%f\r\n", data_f32);
  }
  
  printf("\r\n");
}

void plot_int8(int8_t *data, uint32_t dataSize, const char *fileName)
{
  printf("Filename: %s\r\n", fileName);
    
  while (dataSize-- > 0)
  {
    printf("%i\r\n", *data++);
  }
  
  printf("\r\n");
}

#endif
