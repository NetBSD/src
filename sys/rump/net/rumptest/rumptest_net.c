#include <sys/types.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <string.h>
#include <stdio.h>

#define DEST_ADDR "204.152.190.12"	/* www.NetBSD.org */
#define DEST_PORT 80			/* take a wild guess */

int
main()
{
	char buf[65536];
	struct sockaddr_in sin;
	struct timeval tv;
	ssize_t n;
	size_t off;
	int s, error;

	if (rump_init())
		errx(1, "rump_init failed");

	s = rump_sys___socket30(PF_INET, SOCK_STREAM, 0, &error);
	if (s == -1)
		errx(1, "can't open socket: %d (%s)", error, strerror(error));

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (rump_sys_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
	    &tv, sizeof(tv), &error) == -1)
		errx(1, "setsockopt %d (%s)", error, strerror(error));

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DEST_PORT);
	sin.sin_addr.s_addr = inet_addr(DEST_ADDR);

	if (rump_sys_connect(s, (struct sockaddr *)&sin, sizeof(sin),
	    &error) == -1) {
		errx(1, "connect failed: %d (%s)", error, strerror(error));
	}

	printf("connected\n");

	strcpy(buf, "GET / HTTP/1.0\n\n");
	n = rump_sys_write(s, buf, strlen(buf), &error);
	if (n != strlen(buf))
		errx(1, "wrote only %zd vs. %zu (%s)\n",
		    n, strlen(buf), strerror(error));
	
	memset(buf, 0, sizeof(buf));
	for (off = 0; off < sizeof(buf) && n > 0;) {
		n = rump_sys_read(s, buf+off, sizeof(buf)-off, &error);
		if (n > 0)
			off += n;
	}
	printf("read %zd (max %zu):\n", off, sizeof(buf));
	printf("%s", buf);

	return 0;
}
