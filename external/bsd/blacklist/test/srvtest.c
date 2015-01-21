#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <err.h>

#include "blacklist.h"

static void
process(int afd)
{
	ssize_t n;
	char buffer[256];

	memset(buffer, 0, sizeof(buffer));

	if ((n = read(afd, buffer, sizeof(buffer))) == -1)
		err(1, "read");
	buffer[sizeof(buffer) - 1] = '\0';
	printf("%s: sending %d %s\n", getprogname(), afd, buffer);
	blacklist(1, afd, buffer);
	exit(0);
}

static int
cr(int af, int type, in_port_t p)
{
	int sfd;
	struct sockaddr_storage ss;
	sfd = socket(af == AF_INET ? PF_INET : PF_INET6, type, 0);
	if (sfd == -1)
		err(1, "socket");

	p = htons(p);
	memset(&ss, 0, sizeof(ss));
	if (af == AF_INET) {
		struct sockaddr_in *s = (void *)&ss;
		s->sin_family = AF_INET;
		s->sin_len = sizeof(*s);
		s->sin_port = p;
	} else {
		struct sockaddr_in6 *s6 = (void *)&ss;
		s6->sin6_family = AF_INET6;
		s6->sin6_len = sizeof(*s6);
		s6->sin6_port = p;
	}
     
	if (bind(sfd, (const void *)&ss, ss.ss_len) == -1)
		err(1, "bind");

	if (listen(sfd, 5) == -1)
		err(1, "listen");
	return sfd;
}

static void
handle(int sfd)
{
	struct sockaddr_storage ss;
	socklen_t alen = sizeof(ss);
	int afd;
	if ((afd = accept(sfd, (void *)&ss, &alen)) == -1)
		err(1, "accept");

	/* Create child process */
	switch (fork()) {
	case -1:
		err(1, "fork");
	case 0:
		process(afd);
		break;
	default:
		close(afd);
		break;
	}
}

static __dead void
usage(int c)
{
	warnx("Unknown option `%c'", (char)c);
	fprintf(stderr, "Usage: %s [-u] [-p <num>]\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct pollfd pfd[2];
	int type = SOCK_STREAM, c;
	in_port_t port = 6161;

	signal(SIGCHLD, SIG_IGN);

	while ((c = getopt(argc, argv, "up:")) != -1)
		switch (c) {
		case 'u':
			type = SOCK_DGRAM;
			break;
		case 'p':
			port = (in_port_t)atoi(optarg);
			break;
		default:
			usage(c);
		}

	pfd[0].fd = cr(AF_INET, type, port);
	pfd[1].fd = cr(AF_INET6, type, port);
	pfd[0].events = pfd[1].events = POLLIN;

	for (;;) {
		if (poll(pfd, __arraycount(pfd), INFTIM) == -1)
			err(1, "poll");
		for (size_t i = 0; i < __arraycount(pfd); i++) {
			if ((pfd[i].revents & POLLIN) == 0)
				continue;
			handle(pfd[i].fd);
		}
	}
}
