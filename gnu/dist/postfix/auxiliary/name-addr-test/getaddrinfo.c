 /*
  * getaddrinfo(3) (name->address lookup) tester.
  * 
  * Compile with:
  * 
  * cc -o getaddrinfo getaddrinfo.c (BSD, Linux)
  * 
  * cc -o getaddrinfo getaddrinfo.c -lsocket -lnsl (SunOS 5.x)
  * 
  * Run as: getaddrinfo hostname
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  * 
  * Author: Wietse Venema, IBM T.J. Watson Research, USA.
  */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int     main(int argc, char **argv)
{
    char    hostbuf[NI_MAXHOST];	/* XXX */
    struct addrinfo hints;
    struct addrinfo *res0;
    struct addrinfo *res;
    const char *addr;
    int     err;

#define NO_SERVICE ((char *) 0)

    if (argc != 2) {
	fprintf(stderr, "usage: %s hostname\n", argv[0]);
	exit(1);
    }
    memset((char *) &hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(argv[1], NO_SERVICE, &hints, &res0)) != 0) {
	fprintf(stderr, "host %s not found\n", argv[1]);
	exit(1);
    }
    printf("Hostname:\t%s\n", res0->ai_canonname);
    printf("Addresses:\t");
    for (res = res0; res != 0; res = res->ai_next) {
	addr = (res->ai_family == AF_INET ?
		(char *) &((struct sockaddr_in *) res->ai_addr)->sin_addr :
		(char *) &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr);
	if (inet_ntop(res->ai_family, addr, hostbuf, sizeof(hostbuf)) == 0) {
	    perror("inet_ntop:");
	    exit(1);
	}
	printf("%s ", hostbuf);
    }
    printf("\n");
    freeaddrinfo(res0);
    exit(0);
}
