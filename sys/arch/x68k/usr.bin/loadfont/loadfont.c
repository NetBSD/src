/*
 * loadfont - load ascii font (for NetBSD/X680x0)
 * from: amiga/stand/loadkmap/loadkmap.c
 * Copyright 1993 by Masaru Oki
 *
 *	$NetBSD: loadfont.c,v 1.2 1997/10/13 14:23:08 lukem Exp $
 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#define ITEKANJI 1 /* XXX */
#include "../../dev/iteioctl.h" /* XXX */

void load_font __P((const char *file));

int
main(argc, argv)
     int argc;
     char *argv[];
{
  if (argc != 2)
    fprintf (stderr, "Usage: %s fontfile\n", argv[0]), exit (1);

  load_font (argv[1]);
  exit (0);
}

void
load_font (file)
     const char *file;
{
  int fd;
  unsigned char buf[4096];

  if ((fd = open(file, O_RDONLY)) >= 0) {
      if (read (fd, buf, sizeof(buf)) == sizeof (buf)) {
	  if (ioctl(0, ITELOADFONT, buf) == 0)
	    return;
	  else
	    perror("ITELOADFONT");
      }
      else
	  perror("read font");

      close(fd);
  }
  else
      perror("open font");
}
