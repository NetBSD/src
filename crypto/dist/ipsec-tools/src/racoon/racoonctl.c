/*	$NetBSD: racoonctl.c,v 1.15.2.1 2009/05/13 19:15:54 jym Exp $	*/

/*	Id: racoonctl.c,v 1.11 2006/04/06 17:06:25 manubsd Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * Copyright (C) 2008 Timo Teras.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/pfkeyv2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <err.h>
#include <sys/ioctl.h> 
#include <resolv.h>

#include "var.h"
#include "vmbuf.h"
#include "misc.h"
#include "gcmalloc.h"

#include "racoonctl.h"
#include "admin.h"
#include "schedule.h"
#include "handler.h"
#include "sockmisc.h"
#include "vmbuf.h"
#include "plog.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#include "isakmp_unity.h"
#include "ipsec_doi.h"
#include "evt.h"

char *adminsock_path = ADMINSOCK_PATH;

static void usage __P((void));
static vchar_t *get_combuf __P((int, char **));
static int handle_recv __P((vchar_t *));
static vchar_t *f_reload __P((int, char **));
static vchar_t *f_getsched __P((int, char **));
static vchar_t *f_getsa __P((int, char **));
static vchar_t *f_getsacert __P((int, char **));
static vchar_t *f_flushsa __P((int, char **));
static vchar_t *f_deletesa __P((int, char **));
static vchar_t *f_exchangesa __P((int, char **));
static vchar_t *f_vpnc __P((int, char **));
static vchar_t *f_vpnd __P((int, char **));
static vchar_t *f_getevt __P((int, char **));
#ifdef ENABLE_HYBRID
static vchar_t *f_logoutusr __P((int, char **));
#endif

struct cmd_tag {
	vchar_t *(*func) __P((int, char **));
	char *str;
} cmdtab[] = {
	{ f_reload,	"reload-config" },
	{ f_reload,	"rc" },
	{ f_getsched,	"show-schedule" },
	{ f_getsched,	"sc" },
	{ f_getsa,	"show-sa" },
	{ f_getsa,	"ss" },
	{ f_getsacert,	"get-cert" },
	{ f_getsacert,	"gc" },
	{ f_flushsa,	"flush-sa" },
	{ f_flushsa,	"fs" },
	{ f_deletesa,	"delete-sa" },
	{ f_deletesa,	"ds" },
	{ f_exchangesa,	"establish-sa" },
	{ f_exchangesa,	"es" },
	{ f_vpnc,	"vpn-connect" },
	{ f_vpnc,	"vc" },
	{ f_vpnd,	"vpn-disconnect" },
	{ f_vpnd,	"vd" },
	{ f_getevt,	"show-event" },
	{ f_getevt,	"se" },
#ifdef ENABLE_HYBRID
	{ f_logoutusr,	"logout-user" },
	{ f_logoutusr,	"lu" },
#endif
	{ NULL, NULL },
};

struct evtmsg {
	int type;
	char *msg;
} evtmsg[] = {
	{ EVT_RACOON_QUIT,		"Racoon terminated" },

	{ EVT_PHASE1_UP,		"Phase 1 established" },
	{ EVT_PHASE1_DOWN,		"Phase 1 deleted" },
	{ EVT_PHASE1_NO_RESPONSE,	"Phase 1 error: peer not responding" },
	{ EVT_PHASE1_NO_PROPOSAL,	"Phase 1 error: no proposal chosen" },
	{ EVT_PHASE1_AUTH_FAILED,
	  "Phase 1 error: authentication failed (bad certificate?)" },
	{ EVT_PHASE1_DPD_TIMEOUT,	"Phase 1 error: dead peer detected" },
	{ EVT_PHASE1_MODE_CFG,		"Phase 1 mode configuration done" },
	{ EVT_PHASE1_XAUTH_SUCCESS,	"Phase 1 Xauth succeeded" },
	{ EVT_PHASE1_XAUTH_FAILED,	"Phase 1 Xauth failed" },

	{ EVT_PHASE2_NO_PHASE1,		"Phase 2 error: no suitable phase 1" },
	{ EVT_PHASE2_UP,		"Phase 2 established" },
	{ EVT_PHASE2_DOWN,		"Phase 2 deleted" },
	{ EVT_PHASE2_NO_RESPONSE,	"Phase 2 error: no response" },
};

static vchar_t *get_proto_and_index __P((int, char **, u_int16_t *));
static int get_proto __P((char *));
static vchar_t *get_index __P((int, char **));
static int get_family __P((char *));
static vchar_t *get_comindexes __P((int, int, char **));
static int get_comindex __P((char *, char **, char **, char **));
static int get_ulproto __P((char *));

struct proto_tag {
	int proto;
	char *str;
} prototab[] = {
	{ ADMIN_PROTO_ISAKMP,	"isakmp" },
	{ ADMIN_PROTO_IPSEC,	"ipsec" },
	{ ADMIN_PROTO_AH,	"ah" },
	{ ADMIN_PROTO_ESP,	"esp" },
	{ ADMIN_PROTO_INTERNAL,	"internal" },
	{ 0, NULL },
};

struct ulproto_tag {
	int ul_proto;
	char *str;
} ulprototab[] = {
	{ 0,		"any" },
	{ IPPROTO_ICMP,	"icmp" },
	{ IPPROTO_TCP,	"tcp" },
	{ IPPROTO_UDP,	"udp" },
	{ IPPROTO_GRE,	"gre" },
	{ 0, NULL },
};

int so;

static char _addr1_[NI_MAXHOST], _addr2_[NI_MAXHOST];

char *pname;
int long_format = 0;
int evt_quit_event = 0;

void dump_isakmp_sa __P((char *, int));
void dump_internal __P((char *, int));
char *pindex_isakmp __P((isakmp_index *));
void print_schedule __P((caddr_t, int));
void print_evt __P((struct evt_async *));
char * fixed_addr __P((char *, char *, int));

static void
usage()
{
	printf(
"Usage:\n"
"  %s [opts] reload-config\n"
"  %s [opts] show-schedule\n"
"  %s [opts] show-sa [protocol]\n"
"  %s [opts] flush-sa [protocol]\n"
"  %s [opts] delete-sa <saopts>\n"
"  %s [opts] establish-sa [-u identity] [-n remoteconf] [-w] <saopts>\n"
"  %s [opts] vpn-connect [-u identity] vpn_gateway\n"
"  %s [opts] vpn-disconnect vpn_gateway\n"
"  %s [opts] show-event\n"
"  %s [opts] logout-user login\n"
"\n"
"General options:\n"
"  -d		Debug: hexdump admin messages before sending\n"
"  -l		Increase output verbosity (mainly for show-sa)\n"
"  -s <socket>	Specify adminport socket to use (default: %s)\n"
"\n"
"Parameter specifications:\n"
"    <protocol>: \"isakmp\", \"esp\" or \"ah\".\n"
"        In the case of \"show-sa\" or \"flush-sa\", you can use \"ipsec\".\n"
"\n"
"    <saopts>: \"isakmp\" <family> <src> <dst>\n"
"            : {\"esp\",\"ah\"} <family> <src/prefixlen/port> <dst/prefixlen/port>\n"
"                              <ul_proto>\n"
"    <family>: \"inet\" or \"inet6\"\n"
"    <ul_proto>: \"icmp\", \"tcp\", \"udp\", \"gre\" or \"any\"\n"
"\n",
		pname, pname, pname, pname, pname, pname, pname, pname, pname, pname,
		ADMINSOCK_PATH);
}

/*
 * Check for proper racoonctl interface
 */
#if ((RACOONCTL_INTERFACE_MAJOR != 1) || (RACOONCTL_INTERFACE < 20041230))
#error	"Incompatible racoonctl interface"
#endif

int
main(ac, av)
	int ac;
	char **av;
{
	vchar_t *combuf;
	int c;

	pname = *av;

	/*
	 * Check for proper racoonctl interface
	 */
	if ((racoonctl_interface_major != RACOONCTL_INTERFACE_MAJOR) ||
	    (racoonctl_interface < RACOONCTL_INTERFACE))
		errx(1, "Incompatible racoonctl interface");

#ifdef __linux__
	/*
	 * Disable GNU extensions that will prevent racoonct vc -u login
	 * from working (GNU getopt(3) does not like options after vc)
	 */
	setenv("POSIXLY_CORRECT", "1", 0);
#endif
	while ((c = getopt(ac, av, "lds:")) != -1) {
		switch(c) {
		case 'l':
			long_format++;
			break;

		case 'd':
			loglevel++;
			break;

		case 's':
			adminsock_path = optarg;
			break;

		default:
			usage();
			exit(0);
		}
	}

	ac -= optind;
	av += optind;

	combuf = get_combuf(ac, av);
	if (!combuf)
		err(1, "kmpstat");

	if (loglevel)
		racoon_hexdump(combuf, ((struct admin_com *)combuf)->ac_len);

	com_init();

	if (com_send(combuf) != 0)
		goto bad;

	vfree(combuf);

	do {
		if (com_recv(&combuf) != 0)
			goto bad;
		if (handle_recv(combuf) != 0)
			goto bad;
		vfree(combuf);
	} while (evt_quit_event != 0);

	close(so);
	exit(0);

bad:
	close(so);
	if (errno == EEXIST)
		exit(0);
	exit(1);
}

/* %%% */
/*
 * return command buffer.
 */
static vchar_t *
get_combuf(ac, av)
	int ac;
	char **av;
{
	struct cmd_tag *cp;

	if (ac == 0) {
		usage();
		exit(0);
	}

	/* checking the string of command. */
	for (cp = &cmdtab[0]; cp->str; cp++) {
		if (strcmp(*av, cp->str) == 0) {
			break;
		}
	}
	if (!cp->str) {
		printf("Invalid command [%s]\n", *av);
		errno = EINVAL;
		return NULL;
	}

	ac--;
	av++;
	return (cp->func)(ac, av);
}

static vchar_t *
make_request(u_int16_t cmd, u_int16_t proto, size_t len)
{
	vchar_t *buf;
	struct admin_com *head;

	buf = vmalloc(sizeof(struct admin_com) + len);
	if (buf == NULL)
		errx(1, "not enough core");

	head = (struct admin_com *) buf->v;
	head->ac_len = buf->l;
	head->ac_cmd = ADMIN_FLAG_VERSION | cmd;
	head->ac_version = 1;
	head->ac_proto = proto;

	return buf;
}

static vchar_t *
f_reload(ac, av)
	int ac;
	char **av;
{
	return make_request(ADMIN_RELOAD_CONF, 0, 0);
}

static vchar_t *
f_getevt(ac, av)
	int ac;
	char **av;
{
	evt_quit_event = -1;
	if (ac >= 1)
		errx(1, "too many arguments");

	return make_request(ADMIN_SHOW_EVT, 0, 0);
}

static vchar_t *
f_getsched(ac, av)
	int ac;
	char **av;
{
	return make_request(ADMIN_SHOW_SCHED, 0, 0);
}

static vchar_t *
f_getsa(ac, av)
	int ac;
	char **av;
{
	int proto;

	/* need protocol */
	if (ac != 1)
		errx(1, "insufficient arguments");
	proto = get_proto(*av);
	if (proto == -1)
		errx(1, "unknown protocol %s", *av);

	return make_request(ADMIN_SHOW_SA, proto, 0);
}

static vchar_t *
f_getsacert(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf, *index;
	struct admin_com_indexes *com;

	index = get_index(ac, av);
	if (index == NULL)
		return NULL;

	com = (struct admin_com_indexes *) index->v;
	buf = make_request(ADMIN_GET_SA_CERT, ADMIN_PROTO_ISAKMP, index->l);
	if (buf == NULL)
		errx(1, "Cannot allocate buffer");

	memcpy(buf->v+sizeof(struct admin_com), index->v, index->l);

	vfree(index);

	return buf;
}

static vchar_t *
f_flushsa(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf;
	struct admin_com *head;
	int proto;

	/* need protocol */
	if (ac != 1)
		errx(1, "insufficient arguments");
	proto = get_proto(*av);
	if (proto == -1)
		errx(1, "unknown protocol %s", *av);

	return make_request(ADMIN_FLUSH_SA, proto, 0);
}

static vchar_t *
f_deletesa(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf, *index;
	int proto;

	/* need protocol */
	if (ac < 1)
		errx(1, "insufficient arguments");
	proto = get_proto(*av);
	if (proto == -1)
		errx(1, "unknown protocol %s", *av);

	/* get index(es) */
	av++;
	ac--;
	switch (proto) {
	case ADMIN_PROTO_ISAKMP:
		index = get_index(ac, av);
		if (index == NULL)
			return NULL;
		break;
	case ADMIN_PROTO_AH:
	case ADMIN_PROTO_ESP:
		index = get_index(ac, av);
		if (index == NULL)
			return NULL;
		break;
	default:
		errno = EPROTONOSUPPORT;
		return NULL;
	}

	buf = make_request(ADMIN_DELETE_SA, proto, index->l);
	if (buf == NULL)
		goto out;

	memcpy(buf->v + sizeof(struct admin_com), index->v, index->l);

out:
	if (index != NULL)
		vfree(index);

	return buf;
}

static vchar_t *
f_deleteallsadst(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf, *index;
	u_int16_t proto;

	index = get_proto_and_index(ac, av, &proto);
	if (index == NULL)
		return NULL;

	buf = make_request(ADMIN_DELETE_ALL_SA_DST, proto, index->l);
	if (buf == NULL)
		goto out;

	memcpy(buf->v+sizeof(struct admin_com), index->v, index->l);

out:
	if (index != NULL)
		vfree(index);

	return buf;
}

static vchar_t *
f_exchangesa(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf, *index;
	u_int16_t proto;
	int cmd = ADMIN_ESTABLISH_SA;
	size_t com_len = 0;
	char *id = NULL;
	char *key = NULL;
	char *remoteconf = NULL;
	struct admin_com_psk *acp;
	int wait = 0;

	if (ac < 1)
		errx(1, "insufficient arguments");

	/* Optional -u identity */
	if (strcmp(av[0], "-u") == 0) {
		if (ac < 2)
			errx(1, "-u require an argument");

		id = av[1];
		if ((key = getpass("Password: ")) == NULL)
			errx(1, "getpass() failed: %s", strerror(errno));
		
		com_len += sizeof(*acp) + strlen(id) + 1 + strlen(key) + 1;
		cmd = ADMIN_ESTABLISH_SA_PSK;

		av += 2;
		ac -= 2;
	}

	if (ac >= 2 && strcmp(av[0], "-n") == 0) {
		/* Remoteconf name */
		remoteconf = av[1];
		av += 2;
		ac -= 2;
	}

	if (ac >= 1 && strcmp(av[0], "-w") == 0) {
		wait = 1;
		av++;
		ac--;
	}

	index = get_proto_and_index(ac, av, &proto);
	if (index == NULL)
		return NULL;

	if (proto == ADMIN_PROTO_ISAKMP && cmd == ADMIN_ESTABLISH_SA &&
	    remoteconf != NULL)
		com_len += strlen(remoteconf) + 1;

	if (wait) {
		switch (proto) {
		case ADMIN_PROTO_ISAKMP:
			evt_quit_event = EVT_PHASE1_MODE_CFG;
			break;
		case ADMIN_PROTO_AH:
		case ADMIN_PROTO_ESP:
			evt_quit_event = EVT_PHASE2_UP;
			break;
		default:
			errno = EPROTONOSUPPORT;
			return NULL;
		}
	}

	com_len += index->l;
	buf = make_request(cmd, proto, com_len);
	if (buf == NULL)
		errx(1, "Cannot allocate buffer");

	memcpy(buf->v+sizeof(struct admin_com), index->v, index->l);

	if (proto == ADMIN_PROTO_ISAKMP && cmd == ADMIN_ESTABLISH_SA &&
	    remoteconf != NULL) {
		strcpy(buf->v + sizeof(struct admin_com) + index->l,
		       remoteconf);
	} else if (id && key) {
		char *data;
		acp = (struct admin_com_psk *)
		    (buf->v + sizeof(struct admin_com) + index->l);

		acp->id_type = IDTYPE_USERFQDN;
		acp->id_len = strlen(id) + 1;
		acp->key_len = strlen(key) + 1;

		data = (char *)(acp + 1);
		strcpy(data, id);

		data = (char *)(data + acp->id_len);
		strcpy(data, key);
	}

	vfree(index);

	return buf;
}

static vchar_t *
f_vpnc(ac, av)
	int ac;
	char **av;
{
	char *nav[] = {NULL, NULL, NULL, NULL, NULL, NULL};
	int nac = 0;
	char *isakmp = "isakmp";
	char *inet = "inet";
	char *srcaddr;
	struct addrinfo hints, *res;
	struct sockaddr *src;
	char *idx;

	if (ac < 1)
		errx(1, "insufficient arguments");

	evt_quit_event = EVT_PHASE1_MODE_CFG;
	
	/* Optional -u identity */
	if (strcmp(av[0], "-u") == 0) {
		if (ac < 2)
			errx(1, "-u require an argument");

		nav[nac++] = av[0];
		nav[nac++] = av[1];

		ac -= 2;
		av += 2;
	}

	if (ac < 1)
		errx(1, "VPN gateway required");
	if (ac > 1)
		warnx("Extra arguments");

	/*
	 * Find the source address
	 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	if (getaddrinfo(av[0], "4500", &hints, &res) != 0)
		errx(1, "Cannot resolve destination address");

	if ((src = getlocaladdr(res->ai_addr)) == NULL)
		errx(1, "cannot find source address");

	if ((srcaddr = saddr2str(src)) == NULL)
		errx(1, "cannot read source address");

	/* We get "ip[port]" strip the port */
	if ((idx = index(srcaddr, '[')) == NULL) 
		errx(1, "unexpected source address format");
	*idx = '\0';

	nav[nac++] = isakmp;
	nav[nac++] = inet;
	nav[nac++] = srcaddr;
	nav[nac++] = av[0];

	return f_exchangesa(nac, nav);
}

static vchar_t *
f_vpnd(ac, av)
	int ac;
	char **av;
{
	char *nav[] = {NULL, NULL, NULL, NULL};
	int nac = 0;
	char *isakmp = "isakmp";
	char *inet = "inet";
	char *anyaddr = "0.0.0.0";
	char *idx;

	if (ac < 1)
		errx(1, "VPN gateway required");
	if (ac > 1)
		warnx("Extra arguments");

	evt_quit_event = EVT_PHASE1_DOWN;

	nav[nac++] = isakmp;
	nav[nac++] = inet;
	nav[nac++] = anyaddr;
	nav[nac++] = av[0];

	return f_deleteallsadst(nac, nav);
}

#ifdef ENABLE_HYBRID
static vchar_t *
f_logoutusr(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf;
	char *user;
	size_t userlen;

	/* need username */
	if (ac < 1)
		errx(1, "insufficient arguments");
	user = av[0];
	userlen = strlen(user);
	if ((user == NULL) || (userlen > LOGINLEN))
		errx(1, "bad login (too long?)");

	buf = make_request(ADMIN_LOGOUT_USER, 0, userlen);
	if (buf == NULL)
		return NULL;

	strncpy(buf->v + sizeof(struct admin_com), user, userlen);

	return buf;
}
#endif /* ENABLE_HYBRID */

static vchar_t *
get_proto_and_index(ac, av, proto)
	int ac;
	char **av;
	u_int16_t *proto;
{
	vchar_t *index = NULL;

	/* need protocol */
	if (ac < 1)
		errx(1, "insufficient arguments");
	*proto = get_proto(*av);
	if (*proto == (u_int16_t) -1)
		errx(1, "unknown protocol %s", *av);

	/* get index(es) */
	av++;
	ac--;
	switch (*proto) {
	case ADMIN_PROTO_ISAKMP:
	case ADMIN_PROTO_AH:
	case ADMIN_PROTO_ESP:
		index = get_index(ac, av);
		break;
	default:
		errno = EPROTONOSUPPORT;
		break;
	}
	return index;
}

static int
get_proto(str)
	char *str;
{
	struct proto_tag *cp;

	if (str == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* checking the string of command. */
	for (cp = &prototab[0]; cp->str; cp++) {
		if (strcmp(str, cp->str) == 0)
			return cp->proto;
	}

	errno = EINVAL;
	return -1;
}

static vchar_t *
get_index(ac, av)
	int ac;
	char **av;
{
	int family;

	if (ac != 3 && ac != 4) {
		errno = EINVAL;
		return NULL;
	}

	/* checking the string of family */
	family = get_family(*av);
	if (family == -1)
		return NULL;
	av++;
	ac--;

	return get_comindexes(family, ac, av);
}

static int
get_family(str)
	char *str;
{
	if (strcmp("inet", str) == 0)
		return AF_INET;
#ifdef INET6
	else if (strcmp("inet6", str) == 0)
		return AF_INET6;
#endif
	errno = EAFNOSUPPORT;
	return -1;
}

static vchar_t *
get_comindexes(family, ac, av)
	int family;
	int ac;
	char **av;
{
	vchar_t *buf;
	struct admin_com_indexes *ci;
	char *p_name = NULL, *p_port = NULL;
	char *p_prefs = NULL, *p_prefd = NULL;
	struct sockaddr *src = NULL, *dst = NULL;
	int ulproto;

	if (ac != 2 && ac != 3) {
		errno = EINVAL;
		return NULL;
	}

	if (get_comindex(*av, &p_name, &p_port, &p_prefs) == -1)
		goto bad;
	src = get_sockaddr(family, p_name, p_port);
	if (p_name) {
		racoon_free(p_name);
		p_name = NULL;
	}
	if (p_port) {
		racoon_free(p_port);
		p_port = NULL;
	}
	if (src == NULL)
		goto bad;
	av++;
	ac--;
	if (get_comindex(*av, &p_name, &p_port, &p_prefd) == -1)
		goto bad;
	dst = get_sockaddr(family, p_name, p_port);
	if (p_name) {
		racoon_free(p_name);
		p_name = NULL;
	}
	if (p_port) {
		racoon_free(p_port);
		p_port = NULL;
	}
	if (dst == NULL)
		goto bad;

	buf = vmalloc(sizeof(*ci));
	if (buf == NULL)
		goto bad;

	av++;
	ac--;
	if(ac){
		ulproto = get_ulproto(*av);
		if (ulproto == -1)
			goto bad;
	}else
		ulproto=0;

	ci = (struct admin_com_indexes *)buf->v;
	if(p_prefs)
		ci->prefs = (u_int8_t)atoi(p_prefs); /* XXX should be handled error. */
	else
		ci->prefs = 32;
	if(p_prefd)
		ci->prefd = (u_int8_t)atoi(p_prefd); /* XXX should be handled error. */
	else
		ci->prefd = 32;
	ci->ul_proto = ulproto;
	memcpy(&ci->src, src, sysdep_sa_len(src));
	memcpy(&ci->dst, dst, sysdep_sa_len(dst));

	if (p_name)
		racoon_free(p_name);

	return buf;

   bad:
	if (p_name)
		racoon_free(p_name);
	if (p_port)
		racoon_free(p_port);
	if (p_prefs)
		racoon_free(p_prefs);
	if (p_prefd)
		racoon_free(p_prefd);
	return NULL;
}

static int
get_comindex(str, name, port, pref)
	char *str, **name, **port, **pref;
{
	char *p;

	*name = *port = *pref = NULL;

	*name = racoon_strdup(str);
	STRDUP_FATAL(*name);
	p = strpbrk(*name, "/[");
	if (p != NULL) {
		if (*(p + 1) == '\0')
			goto bad;
		if (*p == '/') {
			*p = '\0';
			*pref = racoon_strdup(p + 1);
			STRDUP_FATAL(*pref);
			p = strchr(*pref, '[');
			if (p != NULL) {
				if (*(p + 1) == '\0')
					goto bad;
				*p = '\0';
				*port = racoon_strdup(p + 1);
				STRDUP_FATAL(*port);
				p = strchr(*pref, ']');
				if (p == NULL)
					goto bad;
				*p = '\0';
			}
		} else if (*p == '[') {
			if (*pref == NULL)
				goto bad;
			*p = '\0';
			*port = racoon_strdup(p + 1);
			STRDUP_FATAL(*port);
			p = strchr(*pref, ']');
			if (p == NULL)
				goto bad;
			*p = '\0';
		} else {
			/* XXX */
		}
	}

	return 0;

    bad:

	if (*name)
		racoon_free(*name);
	if (*port)
		racoon_free(*port);
	if (*pref)
		racoon_free(*pref);
	*name = *port = *pref = NULL;
	return -1;
}

static int
get_ulproto(str)
	char *str;
{
	struct ulproto_tag *cp;

	if(str == NULL){
		errno = EINVAL;
		return -1;
	}

	/* checking the string of upper layer protocol. */
	for (cp = &ulprototab[0]; cp->str; cp++) {
		if (strcmp(str, cp->str) == 0)
			return cp->ul_proto;
	}

	errno = EINVAL;
	return -1;
}

/* %%% */
void
dump_isakmp_sa(buf, len)
	char *buf;
	int len;
{
	struct ph1dump *pd;
	struct tm *tm;
	char tbuf[56];
	caddr_t p = NULL;

/* isakmp status header */
/* short header;
 1234567890123456789012 0000000000000000:0000000000000000 000000000000
*/
char *header1 = 
"Destination            Cookies                           Created";

/* semi long header;
 1234567890123456789012 0000000000000000:0000000000000000 00 X 00 X 0000-00-00 00:00:00 000000
*/
char *header2 = 
"Destination            Cookies                           ST S  V E Created             Phase2";

/* long header;
 0000:0000:0000:0000:0000:0000:0000:0000.00000 0000:0000:0000:0000:0000:0000:0000:0000.00000 0000000000000000:0000000000000000 00 X 00 X 0000-00-00 00:00:00 000000
*/
char *header3 =
"Source                                        Destination                                   Cookies                           ST S  V E Created             Phase2";

/* phase status header */
/* short format;
   side stats source address         destination address   
   xxx  xxxxx 1234567890123456789012 1234567890123456789012
*/

	static char *estr[] = { "", "B", "M", "U", "A", "I", };

	switch (long_format) {
	case 0:
		printf("%s\n", header1);
		break;
	case 1:
		printf("%s\n", header2);
		break;
	case 2:
	default:
		printf("%s\n", header3);
		break;
	}

	if (len % sizeof(*pd))
		printf("invalid length %d\n", len);
	len /= sizeof(*pd);

	pd = (struct ph1dump *)buf;

	while (len-- > 0) {
		/* source address */
		if (long_format >= 2) {
			GETNAMEINFO((struct sockaddr *)&pd->local, _addr1_, _addr2_);
			switch (long_format) {
			case 0:
				break;
			case 1:
				p = fixed_addr(_addr1_, _addr2_, 22);
				break;
			case 2:
			default:
				p = fixed_addr(_addr1_, _addr2_, 45);
				break;
			}
			printf("%s ", p);
		}

		/* destination address */
		GETNAMEINFO((struct sockaddr *)&pd->remote, _addr1_, _addr2_);
		switch (long_format) {
		case 0:
		case 1:
			p = fixed_addr(_addr1_, _addr2_, 22);
			break;
		case 2:
		default:
			p = fixed_addr(_addr1_, _addr2_, 45);
			break;
		}
		printf("%s ", p);

		printf("%s ", pindex_isakmp(&pd->index));

		/* statuc, side and version */
		if (long_format >= 1) {
			printf("%2d %c %2x ",
				pd->status,
				pd->side == INITIATOR ? 'I' : 'R',
				pd->version);
			if (ARRAYLEN(estr) > pd->etype)
				printf("%s ", estr[pd->etype]);
		}

		/* created date */
		if (pd->created) {
			tm = localtime(&pd->created);
			strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %T", tm);
		} else
			snprintf(tbuf, sizeof(tbuf), "                   ");
		printf("%s ", tbuf);

		/* counter of phase 2 */
		if (long_format >= 1)
			printf("%6d ", pd->ph2cnt);

		printf("\n");

		pd++;
	}

	return;
}

/* %%% */
void
dump_internal(buf, tlen)
	char *buf;
	int tlen;
{
	struct ph2handle *iph2;
	struct sockaddr *addr;

/*
short header;
 source address         destination address    
 1234567890123456789012 1234567890123456789012 
*/
char *short_h1 = 
"Source                 Destination            ";

/*
long header;
 source address                                destination address                           
 123456789012345678901234567890123456789012345 123456789012345678901234567890123456789012345 
 0000:0000:0000:0000:0000:0000:0000:0000.00000 0000:0000:0000:0000:0000:0000:0000:0000.00000 0000:0000:0000:0000:0000:0000:0000:0000.00000
*/
char *long_h1 = 
"Source                                        Destination                                  ";

	printf("%s\n", long_format ? long_h1 : short_h1);

	while (tlen > 0) {
		iph2 = (struct ph2handle *)buf;
		addr = (struct sockaddr *)(++iph2);

		GETNAMEINFO(addr, _addr1_, _addr2_);
		printf("%s ", long_format ?
			  fixed_addr(_addr1_, _addr2_, 45)
			: fixed_addr(_addr1_, _addr2_, 22));
		addr++;
		tlen -= sysdep_sa_len(addr);

		GETNAMEINFO(addr, _addr1_, _addr2_);
		printf("%s ", long_format ?
			  fixed_addr(_addr1_, _addr2_, 45)
			: fixed_addr(_addr1_, _addr2_, 22));
		addr++;
		tlen -= sysdep_sa_len(addr);

		printf("\n");
	}

	return;
}

/* %%% */
char *
pindex_isakmp(index)
	isakmp_index *index;
{
	static char buf[64];
	u_char *p;
	int i, j;

	memset(buf, 0, sizeof(buf));

	/* copy index */
	p = (u_char *)index;
	for (j = 0, i = 0; i < sizeof(isakmp_index); i++) {
		snprintf((char *)&buf[j], sizeof(buf) - j, "%02x", p[i]);
		j += 2;
		switch (i) {
		case 7:
#if 0
		case 15:
#endif
			buf[j++] = ':';
		}
	}

	return buf;
}

/* print schedule */
char *str_sched_stat[] = {
"off",
"on",
"dead",
};

char *str_sched_id[] = {
"PH1resend",
"PH1lifetime",
"PH2resend",
"PSTacquire",
"PSTlifetime",
};

void
print_schedule(buf, len)
	caddr_t buf;
	int len;
{
	struct scheddump *sc = (struct scheddump *)buf;
	struct tm *tm;
	char tbuf[56];

	if (len % sizeof(*sc))
		printf("invalid length %d\n", len);
	len /= sizeof(*sc);

	/*      00000000 00000000 00000000 xxx........*/
	printf("index    tick     xtime    created\n");

	while (len-- > 0) {
		tm = localtime(&sc->created);
		strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %T", tm);

		printf("%-8ld %-8ld %-8ld %s\n",
			sc->id,
			(long)sc->tick,
			(long)sc->xtime,
			tbuf);
		sc++;
	}

	return;
}


void
print_evt(evtdump)
	struct evt_async *evtdump;
{
	int i;
	char *srcstr;
	char *dststr;
	
	for (i = 0; i < sizeof(evtmsg) / sizeof(evtmsg[0]); i++)
		if (evtmsg[i].type == evtdump->ec_type)
			break;

	if (evtmsg[i].msg == NULL)
		printf("Event %d: ", evtdump->ec_type);
	else
		printf("%s : ", evtmsg[i].msg);

	if ((srcstr = saddr2str((struct sockaddr *)&evtdump->ec_ph1src)) == NULL)
		printf("unknown");
	else
		printf("%s", srcstr);
	printf(" -> ");
	if ((dststr = saddr2str((struct sockaddr *)&evtdump->ec_ph1dst)) == NULL)
		printf("unknown");
	else
		printf("%s", dststr);
	printf("\n");
}

/*
 * Print ISAKMP mode config info (IP and banner)
 */
void
print_cfg(buf, len)
	caddr_t buf;
	int len;
{
	struct evt_async *evtdump = (struct evt_async *)buf;
	struct isakmp_data *attr;
	char *banner = NULL;
	struct in_addr addr4;
	
	memset(&addr4, 0, sizeof(addr4));

	if (evtdump->ec_type != EVT_PHASE1_MODE_CFG)
		return;

	len -= sizeof(*evtdump);
	attr = (struct isakmp_data *)(evtdump + 1);

	while (len > 0) {
		if (len < sizeof(*attr)) {
			printf("short attribute too short\n");
			break;
		}

		if ((ntohs(attr->type) & ISAKMP_GEN_MASK) == ISAKMP_GEN_TV) {
			/* Short attribute, skip */
			len -= sizeof(*attr);
			attr++;
		} else { /* Long attribute */
			char *n;

			if (len < (sizeof(*attr) + ntohs(attr->lorv))) {
				printf("long attribute too long\n");
				break;
			}

			switch (ntohs(attr->type) & ~ISAKMP_GEN_MASK) {
			case INTERNAL_IP4_ADDRESS:
				if (ntohs(attr->lorv) < sizeof(addr4)) {
					printf("addr4 attribute too short\n");
					break;
				}
				memcpy(&addr4, attr + 1, sizeof(addr4));
				break;

			case UNITY_BANNER:
				banner = racoon_malloc(ntohs(attr->lorv) + 1);
				if (banner == NULL) {
					printf("malloc failed\n");
					break;
				}
				memcpy(banner, attr + 1, ntohs(attr->lorv));
				banner[ntohs(attr->lorv)] = '\0';
				break;

			default:
				break;
			}

			len -= (sizeof(*attr) + ntohs(attr->lorv));
			n = (char *)attr;
			attr = (struct isakmp_data *)
			    (n + sizeof(*attr) + ntohs(attr->lorv));
		}
	}

	if (len > 0)
		printf("Bound to address %s\n", inet_ntoa(addr4));
	else
		printf("VPN connexion established\n");

	if (banner) {
		struct winsize win;
		int col = 0;
		int i;

		if (ioctl(1, TIOCGWINSZ, &win) != 1) 
			col = win.ws_col;
			
		for (i = 0; i < col; i++)
			printf("%c", '=');
		printf("\n%s\n", banner);
		for (i = 0; i < col; i++)
			printf("%c", '=');
		printf("\n");
		racoon_free(banner);
	}
}


char *
fixed_addr(addr, port, len)
	char *addr, *port;
	int len;
{
	static char _addr_buf_[BUFSIZ];
	char *p;
	int plen, i;

	/* initialize */
	memset(_addr_buf_, ' ', sizeof(_addr_buf_));

	plen = strlen(port);
	if (len < plen + 1)
		return NULL;

	p = _addr_buf_;
	for (i = 0; i < len - plen - 1 && addr[i] != '\0'; /*noting*/)
		*p++ = addr[i++];
	*p++ = '.';

	for (i = 0; i < plen && port[i] != '\0'; /*noting*/)
		*p++ = port[i++];

	_addr_buf_[len] = '\0';

	return _addr_buf_;
}

static int
handle_recv(combuf)
	vchar_t *combuf;
{
        struct admin_com *com;
        caddr_t buf;
        int len;

	com = (struct admin_com *)combuf->v;
	len = com->ac_len - sizeof(*com);
	buf = combuf->v + sizeof(*com);

	switch (com->ac_cmd) {
	case ADMIN_SHOW_SCHED:
		print_schedule(buf, len);
		break;

	case ADMIN_SHOW_EVT: {
		struct evt_async *ec;

		/* We got no event? */
		if (len == 0)
			break;

		if (len < sizeof(struct evt_async))
			errx(1, "Short buffer\n");

		ec = (struct evt_async *) buf;
		if (evt_quit_event <= 0)
			print_evt(ec);
		else if (evt_quit_event == ec->ec_type) {
			switch (ec->ec_type) {
			case EVT_PHASE1_MODE_CFG:
				print_cfg(ec, len);
				break;
			default:
				print_evt(ec);
				break;
			}
			evt_quit_event = 0;
		}
		break;
	}

	case ADMIN_GET_SA_CERT:
		fwrite(buf, len, 1, stdout);
		break;

	case ADMIN_SHOW_SA:
	   {
		switch (com->ac_proto) {
		case ADMIN_PROTO_ISAKMP:
			dump_isakmp_sa(buf, len);
			break;
		case ADMIN_PROTO_IPSEC:
		case ADMIN_PROTO_AH:
		case ADMIN_PROTO_ESP:
		    {
			struct sadb_msg *msg = (struct sadb_msg *)buf;

			switch (msg->sadb_msg_errno) {
			case ENOENT:
				switch (msg->sadb_msg_type) {
				case SADB_DELETE:
				case SADB_GET:
					printf("No entry.\n");
					break;
				case SADB_DUMP:
					printf("No SAD entries.\n");
					break;
				}
				break;
			case 0:
				while (1) {
					pfkey_sadump(msg);
					if (msg->sadb_msg_seq == 0)
						break;
					msg = (struct sadb_msg *)((caddr_t)msg +
						     PFKEY_UNUNIT64(msg->sadb_msg_len));
				}
				break;
			default:
				printf("%s.\n", strerror(msg->sadb_msg_errno));
			}
		    }
			break;
		case ADMIN_PROTO_INTERNAL:
			dump_internal(buf, len);
			break;
		default:
			printf("Invalid proto [%d]\n", com->ac_proto);
		}

	    }
		break;

	default:
		/* IGNORE */
		break;
	}

	return 0;

bad:
	return -1;
}
