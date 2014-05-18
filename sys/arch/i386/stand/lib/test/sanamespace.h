/*	$NetBSD: sanamespace.h,v 1.1.206.1 2014/05/18 17:45:13 rmind Exp $	*/

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
