/* netboot
 *
 * $Log: genprom.c,v $
 * Revision 1.1  1993/07/08 16:03:58  brezak
 * Diskless boot prom code from Jim McKim (mckim@lerc.nasa.gov)
 *
 * Revision 1.2  1993/06/30  20:15:38  mckim
 * Display code size.
 *
 * Revision 1.1.1.1  1993/05/28  11:41:06  mckim
 * Initial version.
 *
 *
 * Read a binary image of a bios extension, generate the
 * appropriate block count and checksum and write them
 * into the rom image (replacing 2nd and 5th bytes)
 * The binary image should be sized before being filtered
 * through this routine.
 */

#include <stdio.h>

#define USE_SIZE ROM_SIZE
#define PROM_SIZE 0x10000

main() {
  char w[PROM_SIZE], ck;
  int i, sum;
  memset(w, 0xff, PROM_SIZE);
  i = fread(w, 1, PROM_SIZE, stdin);
  fprintf(stderr, "bios extension size: %d (0x%x), read %d bytes\n",
	  USE_SIZE, USE_SIZE, i);
  w[2] = USE_SIZE / 512;
  for (sum=0, i=0; i<USE_SIZE; i++) {
    sum += w[i];
  }
  w[5] = -sum;
  for (ck=0, i=0; i<USE_SIZE; i++) {
    ck += w[i];
  }
  fwrite(w, 1, PROM_SIZE, stdout);
}
  
