/*	$NetBSD: ipferror.c,v 1.1.1.1.2.2 2012/04/17 00:03:17 yamt Exp $	*/

#include "ipf.h"

#include <sys/ioctl.h>

void ipferror(fd, msg)
	int fd;
	char *msg;
{
	int err, save;

	save = errno;

	err = 0;

	if (fd >= 0) {
		(void) ioctl(fd, SIOCIPFINTERROR, &err);

		fprintf(stderr, "%d:", err);
		ipf_perror(err, msg);
	} else {
		fprintf(stderr, "0:");
		perror(msg);
	}
}
