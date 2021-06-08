/*	$NetBSD: oss_ioctl.c,v 1.1 2021/06/08 18:43:54 nia Exp $	*/

#include <stdarg.h>
#include "internal.h"

int
_oss_ioctl(int fd, unsigned long com, ...)
{
	va_list ap;
	void *argp;

	va_start(ap, com);
	argp = va_arg(ap, void *);
	va_end(ap);

	if (IOCGROUP(com) == 'P')
		return _oss_dsp_ioctl(fd, com, argp);
	else if (IOCGROUP(com) == 'M')
		return _oss3_mixer_ioctl(fd, com, argp);
	else if (IOCGROUP(com) == 'X')
		return _oss4_mixer_ioctl(fd, com, argp);
	else if (IOCGROUP(com) == 'Y')
		return _oss4_global_ioctl(fd, com, argp);
	else
		return ioctl(fd, com, argp);
}
