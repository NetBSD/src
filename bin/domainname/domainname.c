#include <sys/types.h>
#include <sys/param.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>

main(argc, argv)
char **argv;
{
	char dom[MAXHOSTNAMELEN];

	if( argc>2 ) {
		fprintf(stderr, "usage: domainname [name]\n");
		exit(1);
	}
	if( argc==2 ) {
		if( setdomainname(argv[1], strlen(argv[1])+1) == -1) {
			perror("setdomainname");
			exit(1);
		}
		exit(0);
	}
	if( getdomainname(dom, sizeof dom) == -1) {
		perror("getdomainname");
		exit(1);
	}
	printf("%s\n", dom);
	exit(0);
}

