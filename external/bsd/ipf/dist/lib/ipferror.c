/*	$NetBSD: ipferror.c,v 1.1.1.1 2012/03/23 21:20:08 christos Exp $	*/

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
