/*	$KAME: kmpstat.c,v 1.24 2000/12/16 14:15:07 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/pfkeyv2.h>
#include <netkey/keydb.h>
#include <netkey/key_var.h>

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

#include "libpfkey.h"

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "debug.h"

#include "schedule.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "handler.h"
#include "pfkey.h"
#include "admin_var.h"
#include "admin.h"

static void usage __P((void));
static int com_init __P((void));
static int com_send __P((vchar_t *));
static vchar_t *com_recv __P((void));

static vchar_t *get_combuf __P((int, char **));
static vchar_t *f_reload __P((int, char **));
static vchar_t *f_getsched __P((int, char **));
static vchar_t *f_getsa __P((int, char **));
static vchar_t *f_flushsa __P((int, char **));
static vchar_t *f_deletesa __P((int, char **));
static vchar_t *f_exchangesa __P((int, char **));

struct cmd_tag {
	vchar_t *(*func) __P((int, char **));
	int cmd;
	char *str;
} cmdtab[] = {
	{ f_reload,	ADMIN_RELOAD_CONF,	"reload-config" },
	{ f_reload,	ADMIN_RELOAD_CONF,	"rc" },
	{ f_getsched,	ADMIN_SHOW_SCHED,	"show-schedule" },
	{ f_getsched,	ADMIN_SHOW_SCHED,	"sc" },
	{ f_getsa,	ADMIN_SHOW_SA,		"show-sa" },
	{ f_getsa,	ADMIN_SHOW_SA,		"ss" },
	{ f_flushsa,	ADMIN_FLUSH_SA,		"flush-sa" },
	{ f_flushsa,	ADMIN_FLUSH_SA,		"fs" },
	{ f_deletesa,	ADMIN_DELETE_SA,	"delete-sa" },
	{ f_deletesa,	ADMIN_DELETE_SA,	"ds" },
	{ f_exchangesa,	ADMIN_ESTABLISH_SA,	"establish-sa" },
	{ f_exchangesa,	ADMIN_ESTABLISH_SA,	"es" },
	{ NULL, 0, NULL },
};

static int get_proto __P((char *));
static vchar_t *get_index __P((int, char **));
static int get_family __P((char *));
static vchar_t *get_comindexes __P((int, int, char **));
static int get_comindex __P((char *, char **, char **, char **));
static struct sockaddr *get_sockaddr __P((int, char *, char *));
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
	{ 0, NULL },
};

int port = PORT_ADMIN;
int so;

static char _addr1_[NI_MAXHOST], _addr2_[NI_MAXHOST];

char *pname;
int long_format = 0;
u_int32_t loglevel = 4;

void dump_isakmp_sa __P((char *, int));
void dump_internal __P((char *, int));
char *pindex_isakmp __P((isakmp_index *));
void print_schedule __P((caddr_t, int));
char * fixed_addr __P((char *, char *, int));

static void
usage()
{
	printf(
"Usage:\n"
"  %s [-p (admin port)] reload-config\n"
"  %s [-p (admin port)] [-l [-l]] show-sa [protocol]\n"
"  %s [-p (admin port)] flush-sa [protocol]\n"
"  %s [-p (admin port)] delete-sa <saopts>\n"
"  %s [-p (admin port)] establish-sa <saopts>\n"
"\n"
"    <protocol>: \"isakmp\", \"esp\" or \"ah\".\n"
"        In the case of \"show-sa\" or \"flush-sa\", you can use \"ipsec\".\n"
"\n"
"    <saopts>: \"isakmp\" <family> <src> <dst>\n"
"            : {\"esp\",\"ah\"} <family> <src/prefixlen/port> <dst/prefixlen/port>\n"
"                              <ul_proto>\n"
"    <family>: \"inet\" or \"inet6\"\n"
"    <ul_proto>: \"icmp\", \"tcp\", \"udp\" or \"any\"\n",
	pname, pname, pname, pname, pname);
}

int
main(ac, av)
	int ac;
	char **av;
{
	extern char *optarg;
	extern int optind;
	vchar_t *combuf;
	int c;

	pname = *av;

	while ((c = getopt(ac, av, "p:ld")) != EOF) {
		switch(c) {
		case 'p':
			port = atoi(optarg);
			break;

		case 'l':
			long_format++;
			break;

		case 'd':
			loglevel = 0xffffffff;
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
		hexdump(combuf, ((struct admin_com *)combuf)->ac_len);

	if (com_init() < 0)
		goto bad;

	if (com_send(combuf) < 0)
		goto bad;

	vfree(combuf);

	combuf = com_recv();
	if (!combuf)
		goto bad;

	exit(0);

    bad:
	exit(-1);
}

static int
com_init()
{
	struct sockaddr_in name;

	if ((so = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	memset((char *)&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons((u_short)0);
	name.sin_addr.s_addr = htonl(0x7f000001);
	name.sin_len = sizeof(name);

	if (bind(so, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("bind");
		(void)close(so);
		return -1;
	}

	memset((char *)&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons((u_short)port);
	name.sin_addr.s_addr = htonl(0x7f000001);
	name.sin_len = sizeof(name);

	if (connect(so, (struct sockaddr *)&name, sizeof(name)) < 0) {
		perror("connect");
		(void)close(so);
		return -1;
	}

	return so;
}

static int
com_send(combuf)
	vchar_t *combuf;
{
	int len;

	if ((len = send(so, combuf->v, combuf->l, 0)) < 0){
		perror("send");
		(void)close(so);
		return -1;
	}

	return len;
}

static vchar_t *
com_recv()
{
	vchar_t *combuf = NULL;
	struct admin_com h, *com;
	caddr_t buf;
	int len;

	/* receive by PEEK */
	len = recv(so, &h, sizeof(h), MSG_PEEK);
	if (len == -1)
		goto bad;

	/* sanity check */
	if (len < sizeof(h))
		return NULL;
	if (len == 0)
		goto bad;

	/* error ? */
	if (h.ac_errno) {
		errno = h.ac_errno;
		goto bad;
	}

	/* allocate buffer */
	combuf = vmalloc(h.ac_len);
	if (combuf == NULL)
		goto bad;

	/* read real message */
    {
	int l = 0;
	caddr_t p = combuf->v;
	while (l < combuf->l) {
		if ((len = recv(so, p, h.ac_len, 0)) < 0) {
			perror("recv");
			goto bad;
		}
		l += len;
		p += len;
	}
    }

	com = (struct admin_com *)combuf->v;
	len = com->ac_len - sizeof(*com);
	buf = combuf->v + sizeof(*com);

	switch (com->ac_cmd) {
	case ADMIN_SHOW_SCHED:
		print_schedule(buf, len);
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
	}

	(void)close(so);
	return combuf;

    bad:
	(void)close(so);
	return NULL;
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
f_reload(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf;
	struct admin_com *head;

	buf = vmalloc(sizeof(*head));
	if (buf == NULL)
		errx(1, "not enough core");

	head = (struct admin_com *)buf->v;
	head->ac_len = buf->l;
	head->ac_cmd = ADMIN_RELOAD_CONF;
	head->ac_errno = 0;
	head->ac_proto = 0;

	return buf;
}

static vchar_t *
f_getsched(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf;
	struct admin_com *head;

	buf = vmalloc(sizeof(*head));
	if (buf == NULL)
		errx(1, "not enough core");

	head = (struct admin_com *)buf->v;
	head->ac_len = buf->l;
	head->ac_cmd = ADMIN_SHOW_SCHED;
	head->ac_errno = 0;
	head->ac_proto = 0;

	return buf;
}

static vchar_t *
f_getsa(ac, av)
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

	buf = vmalloc(sizeof(*head));
	if (buf == NULL)
		errx(1, "not enough core");

	head = (struct admin_com *)buf->v;
	head->ac_len = buf->l;
	head->ac_cmd = ADMIN_SHOW_SA;
	head->ac_errno = 0;
	head->ac_proto = proto;

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

	buf = vmalloc(sizeof(*head));
	if (buf == NULL)
		errx(1, "not enough core");

	head = (struct admin_com *)buf->v;
	head->ac_len = buf->l;
	head->ac_cmd = ADMIN_FLUSH_SA;
	head->ac_errno = 0;
	head->ac_proto = proto;

	return buf;
}

static vchar_t *
f_deletesa(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf, *index;
	struct admin_com *head;
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

	buf = vmalloc(sizeof(*head) + index->l);
	if (buf == NULL)
		return NULL;

	head = (struct admin_com *)buf->v;
	head->ac_len = buf->l + index->l;
	head->ac_cmd = ADMIN_DELETE_SA;
	head->ac_errno = 0;
	head->ac_proto = proto;

	return buf;
}

static vchar_t *
f_exchangesa(ac, av)
	int ac;
	char **av;
{
	vchar_t *buf, *index;
	struct admin_com *head;
	int proto;

	/* need protocol */
	if (ac < 1)
		errx(1, "insufficient arguments");
	if ((proto = get_proto(*av)) == -1)
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

	buf = vmalloc(sizeof(*head) + index->l);
	if (buf == NULL)
		return NULL;

	head = (struct admin_com *)buf->v;
	head->ac_len = buf->l + index->l;
	head->ac_cmd = ADMIN_DELETE_SA;
	head->ac_errno = 0;
	head->ac_proto = proto;

	return buf;
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

	if (ac != 3) {
		errno = EINVAL;
		return NULL;
	}

	/* checking the string of family */
	family = get_family(*av);
	if (family == -1)
		return NULL;
	av++;

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

	if (ac != 2) {
		errno = EINVAL;
		return NULL;
	}

	if (get_comindex(*av, &p_name, &p_port, &p_prefs) == -1)
		goto bad;
	src = get_sockaddr(family, p_name, p_port);
	if (p_name) {
		free(p_name);
		p_name = NULL;
	}
	if (p_port) {
		free(p_port);
		p_port = NULL;
	}
	if (src == NULL)
		goto bad;
	av++;
	if (get_comindex(*av, &p_name, &p_port, &p_prefd) == -1)
		goto bad;
	dst = get_sockaddr(family, p_name, p_port);
	if (dst == NULL)
		goto bad;

	buf = vmalloc(sizeof(*ci));
	if (buf == NULL)
		goto bad;

	av++;
	ulproto = get_ulproto(*av);
	if (ulproto == -1)
		goto bad;

	ci = (struct admin_com_indexes *)buf;
	ci->prefs = (u_int8_t)atoi(p_prefs); /* XXX should be handled error. */
	ci->prefd = (u_int8_t)atoi(p_prefd); /* XXX should be handled error. */
	ci->ul_proto = ulproto;
	memcpy(&ci->src, src, src->sa_len);
	memcpy(&ci->dst, dst, dst->sa_len);

	if (p_name)
		
	return buf;

   bad:
	if (p_name)
		free(p_name);
	if (p_port)
		free(p_port);
	if (p_prefs);
		free(p_prefs);
	if (p_prefd);
		free(p_prefd);
	return NULL;
}

static int
get_comindex(str, name, port, pref)
	char *str, **name, **port, **pref;
{
	char *p;

	*name = *port = *pref = NULL;

	*name = strdup(str);
	p = strpbrk(*name, "/[");
	if (p != NULL) {
		if (*(p + 1) == '\0')
			goto bad;
		if (*p == '/') {
			*p = '\0';
			*pref = strdup(p + 1);
			p = strchr(*pref, '[');
			if (p != NULL) {
				if (*(p + 1) == '\0')
					goto bad;
				*p = '\0';
				*port = strdup(p + 1);
				p = strchr(*pref, ']');
				if (p == NULL)
					goto bad;
				*p = '\0';
			}
		} else if (*p == '[') {
			*p = '\0';
			*port = strdup(p + 1);
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
		free(*name);
	if (*port)
		free(*port);
	if (*pref)
		free(*pref);
	*name = *port = *pref = NULL;
	return -1;
}

static struct sockaddr *
get_sockaddr(family, name, port)
	int family;
	char *name, *port;
{
	struct addrinfo hint, *ai;
	int error;

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = PF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	error = getaddrinfo(name, port, &hint, &ai);
	if (error != 0) {
		printf("%s: %s/%s\n", gai_strerror(error), name, port);
		return NULL;
	}

	return ai->ai_addr;
}

static int
get_ulproto(str)
	char *str;
{
	struct ulproto_tag *cp;

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
		tlen -= addr->sa_len;

		GETNAMEINFO(addr, _addr1_, _addr2_);
		printf("%s ", long_format ?
			  fixed_addr(_addr1_, _addr2_, 45)
			: fixed_addr(_addr1_, _addr2_, 22));
		addr++;
		tlen -= addr->sa_len;

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
