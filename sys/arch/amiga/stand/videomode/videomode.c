/*
 *	$Id: videomode.c,v 1.2 1994/02/11 07:03:12 chopps Exp $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "../../dev/grfioctl.h"
#include "../../dev/grfvar.h"
#include <stdio.h>

void dump_mode __P((int mode));
void dump_vm   __P((struct grfvideo_mode *vm));

int
main(argc, argv)
     int argc;
     char *argv[];
{
  int m;

  if (argc == 1)
    dump_mode (0);
  else if (! strcmp (argv[1], "-a"))
    dump_mode (-1);
  else if (m = atoi (argv[1]))
    dump_mode (m);
  else if (argc == 3 && !strcmp (argv[1], "-s")
	   && (m = atoi (argv[2])))
    set_mode (m);

  exit (0);
}


int
get_grf ()
{
  struct stat stb;
  char grfname[80];
  int grffd;

  /* find out on which ite/grf we are */
  if (fstat (0, &stb) == -1)
    {
      perror ("fstat 0");
      exit (1);
    }
  if (((stb.st_mode & S_IFMT) != S_IFCHR) || !isatty(0))
    {
      fprintf (stderr, "stdin not a tty\n");
      exit (1);
    }
  if (major(stb.st_rdev) != 13)
    {
      fprintf (stderr, "stdin not an ite device\n");
      exit (1);
    }
  sprintf (grfname, "/dev/grf%d", minor(stb.st_rdev) & 0x7);
  if ((grffd = open (grfname, 2)) < 0)
    {
      perror (grfname);
      exit (1);
    }
  return grffd;
}

void
dump_mode (m)
     int m;
{
  struct grfvideo_mode vm;
  int num_vm;
  int grffd;

  grffd = get_grf ();

  if (ioctl (grffd, GRFGETNUMVM, &num_vm) < 0)
    {
      perror ("GRFGETNUMVM");
      exit (1);
    }
  if (m > 0 && m > num_vm)
    {
      fprintf (stderr, "no such mode.\n");
      exit (1);
    }

  if (m <= 0)
    {
      printf ("Current mode:\n");
      vm.mode_num = 0;
      if (ioctl (grffd, GRFGETVMODE, &vm) == 0)
	dump_vm (&vm);
      printf ("\n");
    }
  if (m < 0)
    {
      for (m = 1; m <= num_vm; m++)
	{
	  vm.mode_num = m;
	  if (ioctl (grffd, GRFGETVMODE, &vm) == -1)
	    break;
	  dump_vm (&vm);
	}
    }
}

void
set_mode (m)
     int m;
{
  int grffd;

  grffd = get_grf ();
  ioctl (grffd, GRFSETVMODE, &m);
}

void
dump_vm (vm)
     struct grfvideo_mode *vm;
{
  printf ("%d: %s\n", vm->mode_num, vm->mode_descr);
  printf ("pixel_clock = %u, width = %d, height = %d, depth = %d\n", 
	  vm->pixel_clock, vm->disp_width, vm->disp_height, vm->depth);
}

