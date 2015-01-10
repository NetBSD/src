#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

static __dead void
usage(const char *msg)
{
	fprintf(stderr, "Usage: %s %s\n", getprogname(), msg);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int sfd;
	struct sockaddr_in ssin;

	if (argc == 1)
		usage("<message>");
	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	memset(&ssin, 0, sizeof(ssin));
	ssin.sin_family = AF_INET;
	ssin.sin_len = sizeof(ssin);
	ssin.sin_addr.s_addr = htonl(INADDR_ANY);
	ssin.sin_port = htons(6161);
     
	if (connect(sfd, (const void *)&ssin, sizeof(ssin)) == -1)
		err(1, "connect");

	size_t len = strlen(argv[1]) + 1;
	if (write(sfd, argv[1], len) != (ssize_t)len)
		err(1, "write");
	return 0;
}
