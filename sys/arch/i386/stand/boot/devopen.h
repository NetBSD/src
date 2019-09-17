/*	$NetBSD: devopen.h,v 1.4.52.1 2019/09/17 18:26:53 martin Exp $	*/

extern int boot_biosdev;

void bios2dev(int, daddr_t, char **, int *, int *, const char **);
