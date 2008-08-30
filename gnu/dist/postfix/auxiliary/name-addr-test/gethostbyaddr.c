 /*
  * gethostbyaddr tester. compile with:
  * 
  * cc -o gethostbyaddr gethostbyaddr.c (SunOS 4.x)
  * 
  * cc -o gethostbyaddr gethostbyaddr.c -lnsl (SunOS 5.x)
  * 
  * run as: gethostbyaddr address
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

main(argc, argv)
int     argc;
char  **argv;
{
    struct hostent *hp;
    long    addr;

    if (argc != 2) {
	fprintf(stderr, "usage: %s i.p.addres\n", argv[0]);
	exit(1);
    }
    addr = inet_addr(argv[1]);
    if (hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET)) {
	printf("Hostname:\t%s\n", hp->h_name);
	printf("Aliases:\t");
	while (hp->h_aliases[0])
	    printf("%s ", *hp->h_aliases++);
	printf("\n");
	printf("Addresses:\t");
	while (hp->h_addr_list[0])
	    printf("%s ", inet_ntoa(*(struct in_addr *) * hp->h_addr_list++));
	printf("\n");
	exit(0);
    }
    fprintf(stderr, "host %s not found\n", argv[1]);
    exit(1);
}
