#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <ctype.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

struct ypalias {
	char *alias, *name;
} ypaliases[] = {
	{ "passwd", "passwd.byname" },
	{ "group", "group.byname" },
	{ "networks", "networks.byaddr" },
	{ "hosts", "hosts.byaddr" },
	{ "protocols", "protocols.bynumber" },
	{ "services", "services.byname" },
	{ "aliases", "mail.aliases" },
	{ "ethers", "ethers.byname" },
};

int key;

usage()
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\typcat [-k] [-d domainname] [-t] mapname\n");
	fprintf(stderr, "\typcat -x\n");
	exit(1);
}

printit(instatus, inkey, inkeylen, inval, invallen, indata)
int instatus;
char *inkey;
int inkeylen;
char *inval;
int invallen;
char *indata;
{
	if(instatus != YP_TRUE)
		return instatus;
	if(key)
		printf("%*.*s ", inkeylen, inkeylen, inkey);
	printf("%*.*s\n", invallen, invallen, inval);
	return 0;
}

int
main(argc, argv)
char **argv;
{
	char *domainname;
	struct ypall_callback ypcb;
	char *inmap;
	extern char *optarg;
	extern int optind;
	int notrans;
	int c, r, i;

	notrans = key = 0;
	yp_get_default_domain(&domainname);

	while( (c=getopt(argc, argv, "xd:kt")) != -1)
		switch(c) {
		case 'x':
			for(i=0; i<sizeof ypaliases/sizeof ypaliases[0]; i++)
				printf("Use \"%s\" for \"%s\"\n",
					ypaliases[i].alias,
					ypaliases[i].name);
			exit(0);
		case 'd':
			domainname = optarg;
			break;
		case 't':
			notrans++;
			break;
		case 'k':
			key++;
			break;
		default:
			usage();
		}

	if(optind + 1 != argc )
		usage();

	inmap = argv[optind];
	for(i=0; (!notrans) && i<sizeof ypaliases/sizeof ypaliases[0]; i++)
		if( strcmp(inmap, ypaliases[i].alias) == 0)
			inmap = ypaliases[i].name;
	ypcb.foreach = printit;
	ypcb.data = NULL;

	r = yp_all(domainname, inmap, &ypcb);
	switch(r) {
	case 0:
		break;
	case YPERR_YPBIND:
		fprintf(stderr, "ypcat: not running ypbind\n");
		exit(1);
	default:
		fprintf(stderr, "No such map %s. Reason: %s\n",
			inmap, yperr_string(r));
		exit(1);
	}
	exit(0);
}
