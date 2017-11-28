/* If we're being compiled as a .c file, rather than being included in
   d10v_sim.h, then ENDIAN_INLINE won't be defined yet.  */

#ifndef ENDIAN_INLINE
#define NO_ENDIAN_INLINE
#include "sim-main.h"
#define ENDIAN_INLINE
#endif

ENDIAN_INLINE uint16
get_word (uint8 *x)
{
  return ((uint16)x[0]<<8) + x[1];
}

ENDIAN_INLINE uint32
get_longword (uint8 *x)
{
  return ((uint32)x[0]<<24) + ((uint32)x[1]<<16) + ((uint32)x[2]<<8) + ((uint32)x[3]);
}

ENDIAN_INLINE int64
get_longlong (uint8 *x)
{
  uint32 top = get_longword (x);
  uint32 bottom = get_longword (x+4);
  return (((int64)top)<<32) | (int64)bottom;
}

ENDIAN_INLINE void
write_word (uint8 *addr, uint16 data)
{
  addr[0] = (data >> 8) & 0xff;
  addr[1] = data & 0xff;
}

ENDIAN_INLINE void
write_longword (uint8 *addr, uint32 data)
{
  addr[0] = (data >> 24) & 0xff;
  addr[1] = (data >> 16) & 0xff;
  addr[2] = (data >> 8) & 0xff;
  addr[3] = data & 0xff;
}

ENDIAN_INLINE void
write_longlong (uint8 *addr, int64 data)
{
  write_longword (addr, (uint32)(data >> 32));
  write_longword (addr+4, (uint32)data);
}
