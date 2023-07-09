#include <stdint.h>
#include "XYMatrix.h"

uint16_t XY( uint8_t col, uint8_t row)
{
  uint16_t i;
  
  i = (kMatrixWidth - 1) - col;
  i *= kMatrixHeight;
  if (col & 0x01)
  {
    i += (kMatrixHeight - 1) - row;
  } else {
    i += row;
  }
  return i;
}