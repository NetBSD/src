/*	$NetBSD: genprom.c,v 1.1.1.1 1997/03/14 02:40:30 perry Exp $	*/
/*
 * mainly from netbsd:sys/arch/i386/netboot/genprom.c
 */

/*
 * Read a binary image of a bios extension, generate the
 * appropriate block count and checksum and write them
 * into the rom image (replacing 2nd and 5th bytes)
 * The binary image should be sized before being filtered
 * through this routine.
 */

#include <stdio.h>
#include <err.h>
#include <string.h>

#define PROM_SIZE 0x10000 /* max */

int
main(argc,argv)
int argc;
char **argv;
{
  char w[PROM_SIZE], ck;
  int i, sum;
  int romsize;

  if(argc>1){
    if(sscanf(argv[1], "%d", &romsize)!=1){
      errx(1, "bad arg");
    }
  }else{
    errx(1, "arg: romsize / bytes\n");
  }

  memset(w, 0x0, PROM_SIZE);
  i = fread(w, 1, PROM_SIZE, stdin);

  fprintf(stderr, "bios extension size: %d (0x%x), read %d bytes\n",
	  romsize, romsize, i);

  if(i > romsize)
	errx(1, "read longer than expected");

  w[2] = romsize / 512;
  for (sum = 0, i = 0; i < romsize; i++) {
    sum += w[i];
  }
  w[5] = -sum;
  for (ck = 0, i = 0; i < romsize; i++) {
    ck += w[i];
  }
  if(ck){
    errx(1, "crc???\n");
  }
  fwrite(w, 1, romsize , stdout);
  return(0);
}
