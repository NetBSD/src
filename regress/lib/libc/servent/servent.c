#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdio.h>

static void
pserv(const struct servent *svp)
{
	char **pp;

	printf("name=%s, port=%d, proto=%s, aliases=",
	    svp->s_name, ntohs((uint16_t)svp->s_port), svp->s_proto);
	for (pp = svp->s_aliases; *pp; pp++)
		printf("%s ", *pp);
	printf("\n");
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s\n"
	    "\t%s -p <port> [-P <proto>]\n"
	    "\t%s -n <name> [-P <proto>]\n", getprogname(), getprogname(),
	    getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct servent *svp;
	const char *port = NULL, *proto = NULL, *name = NULL;
	int c;

	while ((c = getopt(argc, argv, "p:n:P:")) != -1) {
		switch (c) {
		case 'n':
			name = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'P':
			proto = optarg;
			break;
		default:
			usage();
		}
	}

	if (port && name)
		usage();
	if (port) {
		if ((svp = getservbyport(htons(atoi(port)), proto)) != NULL)
			pserv(svp);
		return 0;
	}
	if (name) {
		if ((svp = getservbyname(name, proto)) != NULL)
			pserv(svp);
		return 0;
	}

	setservent(0);
	while ((svp = getservent()) != NULL)
		pserv(svp);
	endservent();
	return 0;
}
