/*	$NetBSD: sanamespace.h,v 1.1.190.1 2014/05/22 11:39:52 yamt Exp $	*/

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
