#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#include "bl.h"

static void
process(bl_t bl, int sfd, int afd)
{
	ssize_t n;
	char buffer[256];

	memset(buffer, 0, sizeof(buffer));

	if ((n = read(afd, buffer, sizeof(buffer))) == -1)
		err(1, "read");
	buffer[sizeof(buffer) - 1] = '\0';
	bl_send(bl, BL_ADD, sfd, afd, buffer);
	printf("received %s\n", buffer);
	exit(0);
}

int
main(int argc, char *argv[])
{
	int sfd;
	bl_t bl;
	struct sockaddr_in ssin;

	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	signal(SIGCHLD, SIG_IGN);

	memset(&ssin, 0, sizeof(ssin));
	ssin.sin_family = AF_INET;
	ssin.sin_len = sizeof(ssin);
	ssin.sin_addr.s_addr = htonl(INADDR_ANY);
	ssin.sin_port = htons(6161);
     
	if (bind(sfd, (const void *)&ssin, sizeof(ssin)) == -1)
		err(1, "bind");

	if (listen(sfd, 5) == -1)
		err(1, "listen");

	bl = bl_create(false);

	for (;;) {
		struct sockaddr_in asin;
		socklen_t alen = sizeof(asin);
		int afd;
		if ((afd = accept(sfd, (void *)&asin, &alen)) == -1)
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
}
