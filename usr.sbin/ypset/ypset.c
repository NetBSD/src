#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

extern bool_t xdr_domainname();

usage()
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\typset [-h host ] [-d domain] server\n");
	exit(1);
}

bind_tohost(sin, dom, server)
struct sockaddr_in *sin;
char *dom, *server;
{
	struct ypbind_setdom ypsd;
	struct timeval tv;
	CLIENT *client;
	int sock, port;
	int r;
	
	if( (port=htons(getrpcport(server, YPPROG, YPPROC_NULL, IPPROTO_UDP))) == 0) {
		fprintf(stderr, "%s not running ypserv.\n", server);
		exit(1);
	}

	bzero(&ypsd, sizeof ypsd);
	strncpy(ypsd.ypsetdom_domain, dom, sizeof ypsd.ypsetdom_domain);
	ypsd.ypsetdom_addr = sin->sin_addr;
	ypsd.ypsetdom_vers = YPVERS;
	ypsd.ypsetdom_port = port;
	
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	sock = RPC_ANYSOCK;
	client = clntudp_create(sin, YPBINDPROG, YPBINDVERS, &sock, 0, 0);
	if (client==NULL) {
		fprintf(stderr, "can't yp_bind: Reason: %s\n",
			yperr_string(YPERR_YPBIND));
		return YPERR_YPBIND;
	}
	client->cl_auth = authunix_create_default();

	r = clnt_call(client, YPBINDPROC_SETDOM,
		xdr_ypbind_setdom, &ypsd, xdr_void, NULL, tv);
	if(r) {
		fprintf(stderr, "Sorry, cannot ypset for domain %s on host.\n", dom);
		clnt_destroy(client);
		return YPERR_YPBIND;
	}
	clnt_destroy(client);
	return 0;
}

int
main(argc, argv)
char **argv;
{
	struct sockaddr_in sin;
	struct hostent *hent;
	extern char *optarg;
	extern int optind;
	char *domainname;
	int c;

	yp_get_default_domain(&domainname);

	bzero(&sin, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0x7f000001);

	while( (c=getopt(argc, argv, "h:d:")) != -1)
		switch(c) {
		case 'd':
			domainname = optarg;
			break;
		case 'h':
			if( (sin.sin_addr.s_addr=inet_addr(optarg)) == -1) {
				hent = gethostbyname(optarg);
				if(hent==NULL) {
					fprintf(stderr, "ypset: host %s unknown\n",
						optarg);
					exit(1);
				}
				bcopy(&hent->h_addr_list[0], &sin.sin_addr,
					sizeof sin.sin_addr);
			}
			break;
		default:
			usage();
		}

	if(optind + 1 != argc )
		usage();

	if (bind_tohost(&sin, domainname, argv[optind]))
		exit(1);
	exit(0);
}
