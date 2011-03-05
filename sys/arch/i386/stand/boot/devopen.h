/*	$NetBSD: devopen.h,v 1.3.100.1 2011/03/05 20:50:43 rmind Exp $	*/

extern int boot_biosdev;

void bios2dev(int, daddr_t, char **, int *, int *);
