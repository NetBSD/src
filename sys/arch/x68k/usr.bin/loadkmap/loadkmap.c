/*	$NetBSD: loadkmap.c,v 1.4 2000/07/31 23:40:02 minoura Exp $	*/
/*
 * loadkmap - load keyboard map (for NetBSD/X680x0)
 * from: amiga/stand/loadkmap/loadkmap.c
 * Copyright 1994 by Masaru Oki
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#define ITEKANJI 1 /* XXX */
#include <machine/iteioctl.h>
#include "kbdmap.h"

void load_kmap __P((const char *file));

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main()
     int argc;
     char *argv[];
#endif
{
  if (argc != 2)
    fprintf (stderr, "Usage: %s kmapfile\n", argv[0]), exit (1);

  load_kmap (argv[1]);
  exit (0);
}

void
#ifdef __STDC__
load_kmap (const char *file)
#else
load_kmap (file)
     const char *file;
#endif
{
  int fd;
  unsigned char buf[sizeof(struct kbdmap)];

  if ((fd = open (file, 0)) >= 0)
    {
      if (read (fd, buf, sizeof (buf)) == sizeof (buf))
	{
	  if (ioctl (0, ITEIOCSKMAP, buf) == 0)
	    return;
	  else
	    perror ("ITEIOCSKMAP");
	}
      else
	perror ("read kbdmap");

      close (fd);
    }
  else
    perror ("open kbdmap");
}
