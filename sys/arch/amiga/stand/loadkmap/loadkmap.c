/*	$NetBSD: loadkmap.c,v 1.4.50.1 2002/02/28 04:07:10 nathanw Exp $	*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "../../dev/iteioctl.h"
#include "../../dev/kbdmap.h"
#include <stdio.h>


void load_kmap(const char *);
void dump_kmap(void);

int
main(int argc, char *argv[])
{
  if (argc > 2)
    {
      fprintf (stderr, "%s keymap\n", argv[0]);
      exit (1);
    }

  if (argc == 1)
    dump_kmap ();
  else
    load_kmap (argv[1]);

  exit (0);
}


void
load_kmap(const char *file)
{
  int fd;
  char buf[sizeof (struct kbdmap)];

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
	perror ("read kmap");

      close (fd);
    }
  else
    perror ("open kmap");
}

void
dump_kmap(void)
{
  char buf[sizeof (struct kbdmap)];
  if (ioctl (0, ITEIOCGKMAP, buf) == 0)
    write (1, buf, sizeof (buf));
  else
    perror ("ITEIOCGKMAP");
}
