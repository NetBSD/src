/*	$NetBSD: tvctrl.c,v 1.3 1998/08/06 14:08:55 minoura Exp $	*/

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <machine/iteioctl.h>

main(argc, argv)
     int argc;
     char *argv[];
{
  unsigned char ctl;
  if (argc < 2) printf("usage: %s control_number [...]\n", argv[0]), exit(1);
  while (argv++, --argc != 0) {
    ctl = atoi(argv[0]);
    if (ioctl(0, ITETVCTRL, &ctl)) perror(errno);
  }
}
