/*
 * This code is not copyright, and is placed in the public domain.
 * Feel free to use and modify. Please send modifications and/or
 * suggestions + bug fixes to Klas Heggemann <klas@nada.kth.se>
 * 
 * Various small changes by Theo de Raadt <deraadt@fsa.ca>
 *
 * $Id: bootparamd.c,v 1.1 1994/01/08 13:22:04 deraadt Exp $
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpcsvc/bootparam_prot.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <syslog.h>

#define MAXLEN 800

struct hostent *he;
static char buffer[MAXLEN];
static char hostname[MAX_MACHINE_NAME];
static char askname[MAX_MACHINE_NAME];
static char path[MAX_PATH_LEN];
static char domain_name[MAX_MACHINE_NAME];

extern void bootparamprog_1();

int     debug = 0;
int     dolog = 0;
unsigned long route_addr, inet_addr();
struct sockaddr_in my_addr;
char   *progname;
char   *bootpfile = "/etc/bootparams";

extern char *optarg;
extern int optind;

void
usage()
{
	fprintf(stderr,
		"usage: rpc.bootparamd [-d] [-s] [-r router] [-f bootparmsfile]\n");
}


/*
 * ever familiar
 */
int
main(argc, argv)
	int     argc;
	char  **argv;
{
	SVCXPRT *transp;
	int     i, s, pid;
	char   *rindex();
	struct hostent *he;
	struct stat buf;
	char   *optstring;
	char    c;

	progname = rindex(argv[0], '/');
	if (progname)
		progname++;
	else
		progname = argv[0];

	while ((c = getopt(argc, argv, "dsr:f:")) != EOF)
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 'r':
			if (isdigit(*optarg)) {
				route_addr = inet_addr(optarg);
				break;
			}
			he = gethostbyname(optarg);
			if (!he) {
				fprintf(stderr, "%s: No such host %s\n",
				    progname, optarg);
				usage();
				exit(1);
			}
			bcopy(he->h_addr, (char *) &route_addr, sizeof(route_addr));
			break;
		case 'f':
			bootpfile = optarg;
			break;
		case 's':
			dolog = 1;
#ifndef LOG_DAEMON
			openlog(progname, 0, 0);
#else
			openlog(progname, 0, LOG_DAEMON);
			setlogmask(LOG_UPTO(LOG_NOTICE));
#endif
			break;
		default:
			usage();
			exit(1);
		}

	if (stat(bootpfile, &buf)) {
		fprintf(stderr, "%s: ", progname);
		perror(bootpfile);
		exit(1);
	}
	if (!route_addr) {
		get_myaddress(&my_addr);
		bcopy(&my_addr.sin_addr.s_addr, &route_addr, sizeof(route_addr));
	}
	if (!debug)
		daemon();

	(void) pmap_unset(BOOTPARAMPROG, BOOTPARAMVERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf(stderr, "cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(transp, BOOTPARAMPROG, BOOTPARAMVERS,
		bootparamprog_1, IPPROTO_UDP)) {
		fprintf(stderr,
		    "bootparamd: unable to register BOOTPARAMPROG version %d, udp)\n",
		    BOOTPARAMVERS);
		exit(1);
	}
	svc_run();
	fprintf(stderr, "svc_run returned\n");
	exit(1);
}

bp_whoami_res *
bootparamproc_whoami_1(whoami)
	bp_whoami_arg *whoami;
{
	long    haddr;
	static bp_whoami_res res;
	if (debug)
		fprintf(stderr, "whoami got question for %d.%d.%d.%d\n",
		    255 & whoami->client_address.bp_address_u.ip_addr.net,
		    255 & whoami->client_address.bp_address_u.ip_addr.host,
		    255 & whoami->client_address.bp_address_u.ip_addr.lh,
		    255 & whoami->client_address.bp_address_u.ip_addr.impno);
	if (dolog)
		syslog(LOG_NOTICE, "whoami got question for %d.%d.%d.%d\n",
		    255 & whoami->client_address.bp_address_u.ip_addr.net,
		    255 & whoami->client_address.bp_address_u.ip_addr.host,
		    255 & whoami->client_address.bp_address_u.ip_addr.lh,
		    255 & whoami->client_address.bp_address_u.ip_addr.impno);

	bcopy((char *) &whoami->client_address.bp_address_u.ip_addr, (char *) &haddr,
	    sizeof(haddr));
	he = gethostbyaddr((char *) &haddr, sizeof(haddr), AF_INET);
	if (!he)
		goto failed;

	if (debug)
		fprintf(stderr, "This is host %s\n", he->h_name);
	if (dolog)
		syslog(LOG_NOTICE, "This is host %s\n", he->h_name);

	strcpy(askname, he->h_name);
	if (checkhost(askname, hostname)) {
		res.client_name = hostname;
		getdomainname(domain_name, MAX_MACHINE_NAME);
		res.domain_name = domain_name;

		if (res.router_address.address_type != IP_ADDR_TYPE) {
			res.router_address.address_type = IP_ADDR_TYPE;
			bcopy(&route_addr, &res.router_address.bp_address_u.ip_addr, 4);
		}
		if (debug)
			fprintf(stderr, "Returning %s   %s    %d.%d.%d.%d\n",
			    res.client_name, res.domain_name,
			    255 & res.router_address.bp_address_u.ip_addr.net,
			    255 & res.router_address.bp_address_u.ip_addr.host,
			    255 & res.router_address.bp_address_u.ip_addr.lh,
			    255 & res.router_address.bp_address_u.ip_addr.impno);
		if (dolog)
			syslog(LOG_NOTICE, "Returning %s   %s    %d.%d.%d.%d\n",
			    res.client_name, res.domain_name,
			    255 & res.router_address.bp_address_u.ip_addr.net,
			    255 & res.router_address.bp_address_u.ip_addr.host,
			    255 & res.router_address.bp_address_u.ip_addr.lh,
			    255 & res.router_address.bp_address_u.ip_addr.impno);

		return (&res);
	}
failed:
	if (debug)
		fprintf(stderr, "whoami failed\n");
	if (dolog)
		syslog(LOG_NOTICE, "whoami failed\n");
	return (NULL);
}


bp_getfile_res *
bootparamproc_getfile_1(getfile)
	bp_getfile_arg *getfile;
{
	char   *where, *index();
	static bp_getfile_res res;

	if (debug)
		fprintf(stderr, "getfile got question for \"%s\" and file \"%s\"\n",
		    getfile->client_name, getfile->file_id);

	if (dolog)
		syslog(LOG_NOTICE, "getfile got question for \"%s\" and file \"%s\"\n",
		    getfile->client_name, getfile->file_id);

	he = NULL;
	he = gethostbyname(getfile->client_name);
	if (!he)
		goto failed;

	strcpy(askname, he->h_name);
	if (getthefile(askname, getfile->file_id, buffer)) {
		if (where = index(buffer, ':')) {
			/* buffer is re-written to contain the name of the
			 * info of file */
			strncpy(hostname, buffer, where - buffer);
			hostname[where - buffer] = '\0';
			where++;
			strcpy(path, where);
			he = gethostbyname(hostname);
			if (!he)
				goto failed;
			bcopy(he->h_addr, &res.server_address.bp_address_u.ip_addr, 4);
			res.server_name = hostname;
			res.server_path = path;
			res.server_address.address_type = IP_ADDR_TYPE;
		} else {	/* special for dump, answer with null strings */
			if (!strcmp(getfile->file_id, "dump")) {
				res.server_name[0] = '\0';
				res.server_path[0] = '\0';
				bzero(&res.server_address.bp_address_u.ip_addr, 4);
			} else
				goto failed;
		}
		if (debug)
			fprintf(stderr,
			    "returning server:%s path:%s address: %d.%d.%d.%d\n",
			    res.server_name, res.server_path,
			    255 & res.server_address.bp_address_u.ip_addr.net,
			    255 & res.server_address.bp_address_u.ip_addr.host,
			    255 & res.server_address.bp_address_u.ip_addr.lh,
			    255 & res.server_address.bp_address_u.ip_addr.impno);
		if (dolog)
			syslog(LOG_NOTICE,
			    "returning server:%s path:%s address: %d.%d.%d.%d\n",
			    res.server_name, res.server_path,
			    255 & res.server_address.bp_address_u.ip_addr.net,
			    255 & res.server_address.bp_address_u.ip_addr.host,
			    255 & res.server_address.bp_address_u.ip_addr.lh,
			    255 & res.server_address.bp_address_u.ip_addr.impno);
		return (&res);
	}
failed:
	if (debug)
		fprintf(stderr, "getfile failed for %s\n", getfile->client_name);
	if (dolog)
		syslog(LOG_NOTICE,
		    "getfile failed for %s\n", getfile->client_name);
	return (NULL);
}


/*
 * getthefile return 1 and fills the buffer with the information
 * of the file, e g "host:/export/root/client" if it can be found.
 * If the host is in the database, but the file is not, the buffer
 * will be empty. (This makes it possible to give the special
 * empty answer for the file "dump")
 */
getthefile(askname, fileid, buffer)
	char   *askname;
	char   *fileid, *buffer;
{
	FILE   *bpf;
	char   *where;

	int     ch, pch, fid_len, res = 0;
	int     match = 0;
	char    info[MAX_FILEID + MAX_PATH_LEN + MAX_MACHINE_NAME + 3];

	bpf = fopen(bootpfile, "r");
	if (!bpf) {
		fprintf(stderr, "No %s\n", bootpfile);
		exit(1);
	}
	while (fscanf(bpf, "%s", hostname) > 0 && !match) {
		if (*hostname != '#') {	/* comment */
			if (!strcmp(hostname, askname)) {
				match = 1;
			} else {
				he = gethostbyname(hostname);
				if (he && !strcmp(he->h_name, askname))
					match = 1;
			}
		}
		/* skip to next entry */
		if (match)
			break;
		pch = ch = getc(bpf);
		while (!(ch == '\n' && pch != '\\') && ch != EOF) {
			pch = ch;
			ch = getc(bpf);
		}
	}

	/* if match is true we read the rest of the line to get the info of
	 * the file */

	if (match) {
		fid_len = strlen(fileid);
		while (!res && (fscanf(bpf, "%s", info)) > 0) {	/* read a string */
			ch = getc(bpf);	/* and a character */
			if (*info != '#') {	/* Comment ? */
				if (!strncmp(info, fileid, fid_len) &&
				    *(info + fid_len) == '=') {
					where = info + fid_len + 1;
					if (isprint(*where)) {
						strcpy(buffer, where);
						res = 1;
						break;
					}
				} else {
					while (isspace(ch) && ch != '\n')
						ch = getc(bpf);
					if (ch == '\n') {
						res = -1;
						break;
					}
					if (ch == '\\') {
						ch = getc(bpf);
						if (ch == '\n')
							continue;
						ungetc(ch, bpf);
						ungetc('\\', bpf);
					} else
						ungetc(ch, bpf);
				}
			} else
				break;
		}
	}
	if (fclose(bpf)) {
		fprintf(stderr, "Could not close %s\n", bootpfile);
	}
	if (res == -1)
		buffer[0] = '\0';	/* host found, file not */
	return (match);
}


/*
 * checkhost puts the hostname found in the database file in
 * the hostname-variable and returns 1, if askname is a valid
 * name for a host in the database
 */
checkhost(askname, hostname)
	char   *askname;
	char   *hostname;
{
	int     ch, pch;
	FILE   *bpf;
	int     res = 0;

	bpf = fopen(bootpfile, "r");
	if (!bpf) {
		fprintf(stderr, "No %s\n", bootpfile);
		exit(1);
	}
	while (fscanf(bpf, "%s", hostname) > 0) {
		if (!strcmp(hostname, askname)) {
			/* return true for match of hostname */
			res = 1;
			break;
		} else {
			/* check the alias list */
			he = NULL;
			he = gethostbyname(hostname);
			if (he && !strcmp(askname, he->h_name)) {
				res = 1;
				break;
			}
		}
		/* skip to next entry */
		pch = ch = getc(bpf);
		while (!(ch == '\n' && pch != '\\') && ch != EOF) {
			pch = ch;
			ch = getc(bpf);
		}
	}
	fclose(bpf);
	return (res);
}
