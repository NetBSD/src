/*	$NetBSD: tvctrl.c,v 1.2 1998/01/05 20:52:34 perry Exp $	*/

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include "../../dev/iteioctl.h" /* XXX */

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
