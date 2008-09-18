 /*
  * getnameinfo(3) (address->name lookup) tester.
  * 
  * Compile with:
  * 
  * cc -o getnameinfo getnameinfo.c (BSD, Linux)
  * 
  * cc -o getnameinfo getnameinfo.c -lsocket -lnsl (SunOS 5.x)
  * 
  * Run as: getnameinfo address
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
    const char *host;
    const char *addr;
    int     err;

#define NO_SERVICE ((char *) 0)

    if (argc != 2) {
	fprintf(stderr, "usage: %s ipaddres\n", argv[0]);
	exit(1);
    }

    /*
     * Convert address to internal form.
     */
    host = argv[1];
    memset((char *) &hints, 0, sizeof(hints));
    hints.ai_family = (strchr(host, ':') ? AF_INET6 : AF_INET);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_NUMERICHOST;
    if ((err = getaddrinfo(host, NO_SERVICE, &hints, &res0)) != 0) {
	fprintf(stderr, "getaddrinfo %s: %s\n", host, gai_strerror(err));
	exit(1);
    }

    /*
     * Convert host address to name.
     */
    for (res = res0; res != 0; res = res->ai_next) {
	err = getnameinfo(res->ai_addr, res->ai_addrlen,
			  hostbuf, sizeof(hostbuf),
			  NO_SERVICE, 0, NI_NAMEREQD);
	if (err) {
	    fprintf(stderr, "getnameinfo %s: %s\n", host, gai_strerror(err));
	    exit(1);
	}
	printf("Hostname:\t%s\n", hostbuf);
	addr = (res->ai_family == AF_INET ?
		(char *) &((struct sockaddr_in *) res->ai_addr)->sin_addr :
		(char *) &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr);
	if (inet_ntop(res->ai_family, addr, hostbuf, sizeof(hostbuf)) == 0) {
	    perror("inet_ntop:");
	    exit(1);
	}
	printf("Address:\t%s\n", hostbuf);
    }
    freeaddrinfo(res0);
    exit(0);
}
