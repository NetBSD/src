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
process(bl_t bl, int sfd, int afd)
{
	ssize_t n;
	char buffer[256];

	memset(buffer, 0, sizeof(buffer));

	if ((n = read(afd, buffer, sizeof(buffer))) == -1)
		err(1, "read");
	buffer[sizeof(buffer) - 1] = '\0';
	printf("%s: sending %d %d %s\n", getprogname(), sfd, afd, buffer);
	bl_send(bl, BL_ADD, sfd, afd, buffer);
	exit(0);
}

static int
cr(int af, in_port_t p)
{
	int sfd;
	struct sockaddr_storage ss;
	sfd = socket(af == AF_INET ? PF_INET : PF_INET6, SOCK_STREAM, 0);
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
handle(bl_t bl, int sfd)
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
		process(bl, sfd, afd);
		break;
	default:
		close(afd);
		break;
	}
}

int
main(int argc, char *argv[])
{
	bl_t bl;
	struct pollfd pfd[2];

	signal(SIGCHLD, SIG_IGN);

	pfd[0].fd = cr(AF_INET, 6161);
	pfd[1].fd = cr(AF_INET6, 6161);
	pfd[0].events = pfd[1].events = POLLIN;

	bl = bl_create();

	for (;;) {
		if (poll(pfd, __arraycount(pfd), INFTIM) == -1)
			err(1, "poll");
		for (size_t i = 0; i < __arraycount(pfd); i++) {
			if ((pfd[i].revents & POLLIN) == 0)
				continue;
			handle(bl, pfd[i].fd);
		}
	}
}
