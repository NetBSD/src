/*	$NetBSD$	*/

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
