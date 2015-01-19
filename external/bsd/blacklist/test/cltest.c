#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s [-a <addr>] [-m <msg>]\n", getprogname());
	exit(1);
}

static void
getaddr(const char *a, in_port_t p, struct sockaddr_storage *ss)
{
	int c;

	memset(ss, 0, sizeof(*ss));
	p = htons(p);

	if (strchr(a, ':')) {
		struct sockaddr_in6 *s6 = (void *)ss;
		c = inet_pton(AF_INET6, a, &s6->sin6_addr);
		s6->sin6_family = AF_INET6;
		s6->sin6_len = sizeof(*s6);
		s6->sin6_port = p;
	} else {
		struct sockaddr_in *s = (void *)ss;
		c = inet_pton(AF_INET, a, &s->sin_addr);
		s->sin_family = AF_INET;
		s->sin_len = sizeof(*s);
		s->sin_port = p;
	}
	if (c == -1)
		err(EXIT_FAILURE, "Invalid address `%s'", a);
}

int
main(int argc, char *argv[])
{
	int sfd;
	int c;
	struct sockaddr_storage ss;
	const char *msg = "hello";
	const char *addr = "127.0.0.1";

	while ((c = getopt(argc, argv, "a:m:")) == -1) {
		switch (c) {
		case 'a':
			addr = optarg;
			break;
		case 'm':
			msg = optarg;
			break;
		default:
			usage();
		}
	}

	getaddr(addr, 6161, &ss);

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	if (connect(sfd, (const void *)&ss, ss.ss_len) == -1)
		err(1, "connect");

	size_t len = strlen(msg) + 1;
	if (write(sfd, msg, len) != (ssize_t)len)
		err(1, "write");
	return 0;
}
