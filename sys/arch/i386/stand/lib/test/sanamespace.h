/*	$NetBSD: sanamespace.h,v 1.1.200.1 2014/08/20 00:03:07 tls Exp $	*/

/* take back the namespace mangling done by "Makefile.satest" */

#undef main
#undef exit
#undef free
#undef open
#undef close
#undef read
#undef write
#undef ioctl
#undef lseek
#undef printf
#undef vprintf
#undef putchar
#undef gets
#undef strerror
#undef errno
