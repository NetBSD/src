#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct addrinfo ai;

char host[NI_MAXHOST];
char serv[NI_MAXSERV];
int vflag = 0;

static void
usage()
{
	fprintf(stderr, "usage: test [-f family] [-s socktype] [-p proto] [-DPRSv46] host serv\n");
}

static void
print1(title, res, h, s)
	const char *title;
	const struct addrinfo *res;
	char *h;
	char *s;
{
	char *start, *end;
	int error;
	const int niflag = NI_NUMERICHOST;

	if (res->ai_addr) {
		error = getnameinfo(res->ai_addr, res->ai_addr->sa_len,
				    host, sizeof(host), serv, sizeof(serv),
				    niflag);
		h = host;
		s = serv;
	} else
		error = 0;

	if (vflag) {
		start = "\t";
		end = "\n";
	} else {
		start = " ";
		end = "";
	}
	printf("%s%s", title, end);
	printf("%sflags 0x%x%s", start, res->ai_flags, end);
	printf("%sfamily %d%s", start, res->ai_family, end);
	printf("%ssocktype %d%s", start, res->ai_socktype, end);
	printf("%sprotocol %d%s", start, res->ai_protocol, end);
	printf("%saddrlen %d%s", start, res->ai_addrlen, end);
	if (error)
		printf("%serror %d%s", start, error, end);
	else {
		printf("%shost %s%s", start, h, end);
		printf("%sserv %s%s", start, s, end);
	}
	if (res->ai_canonname)
		printf("%scname \"%s\"%s", start, res->ai_canonname, end);
	if (!vflag)
		printf("\n");

}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct addrinfo *res;
	int error, i;
	char *p, *q;
	extern int optind;
	extern char *optarg;
	int c;
	char nbuf[10];

	memset(&ai, 0, sizeof(ai));
	ai.ai_family = PF_UNSPEC;
	ai.ai_flags |= AI_CANONNAME;
	while ((c = getopt(argc, argv, "Df:p:PRs:Sv46")) != -1) {
		switch (c) {
		case 'D':
			ai.ai_socktype = SOCK_DGRAM;
			break;
		case 'f':
			ai.ai_family = atoi(optarg);
			break;
		case 'p':
			ai.ai_protocol = atoi(optarg);
			break;
		case 'P':
			ai.ai_flags |= AI_PASSIVE;
			break;
		case 'R':
			ai.ai_socktype = SOCK_RAW;
			break;
		case 's':
			ai.ai_socktype = atoi(optarg);
			break;
		case 'S':
			ai.ai_socktype = SOCK_STREAM;
			break;
		case 'v':
			vflag++;
			break;
		case '4':
			ai.ai_family = PF_INET;
			break;
		case '6':
			ai.ai_family = PF_INET6;
			break;
		default:
			usage();
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2){
		usage();
		exit(1);
	}

	p = *argv[0] ? argv[0] : NULL;
	q = *argv[1] ? argv[1] : NULL;

	print1("arg:", &ai, p ? p : "(empty)", q ? q : "(empty)");

	error = getaddrinfo(p, q, &ai, &res);
	if (error) {
		printf("%s\n", gai_strerror(error));
		exit(1);
	}

	i = 1;
	do {
		snprintf(nbuf, sizeof(nbuf), "ai%d:", i);
		print1(nbuf, res, NULL, NULL);

		i++;
	} while ((res = res->ai_next) != NULL);

	exit(0);
}
