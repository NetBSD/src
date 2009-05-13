/*	$NetBSD: setkey.c,v 1.12.20.1 2009/05/13 19:15:55 jym Exp $	*/

/*	$KAME: setkey.c,v 1.36 2003/09/24 23:52:51 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <err.h>
#include <netinet/in.h>
#include <net/pfkeyv2.h>
#include PATH_IPSEC_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "config.h"
#include "libpfkey.h"
#include "package_version.h"
#define extern /* so that variables in extern.h are not extern... */
#include "extern.h"

#define strlcpy(d,s,l) (strncpy(d,s,l), (d)[(l)-1] = '\0')

void usage __P((int));
int main __P((int, char **));
int get_supported __P((void));
void sendkeyshort __P((u_int));
void promisc __P((void));
int postproc __P((struct sadb_msg *, int));
int verifypriority __P((struct sadb_msg *m));
int fileproc __P((const char *));
const char *numstr __P((int));
void shortdump_hdr __P((void));
void shortdump __P((struct sadb_msg *));
static void printdate __P((void));
static int32_t gmt2local __P((time_t));
void stdin_loop __P((void));

#define MODE_SCRIPT	1
#define MODE_CMDDUMP	2
#define MODE_CMDFLUSH	3
#define MODE_PROMISC	4
#define MODE_STDIN	5

int so;

int f_forever = 0;
int f_all = 0;
int f_verbose = 0;
int f_mode = 0;
int f_cmddump = 0;
int f_policy = 0;
int f_hexdump = 0;
int f_tflag = 0;
int f_notreally = 0;
int f_withports = 0;
#ifdef HAVE_POLICY_FWD
int f_rfcmode = 1;
#define RK_OPTS "rk"
#else
int f_rkwarn = 0;
#define RK_OPTS ""
static void rkwarn(void);
static void
rkwarn(void)
{
	if (!f_rkwarn) {
		f_rkwarn = 1;
		printf("warning: -r and -k options are not supported in this environment\n");
	}
}

#endif
static time_t thiszone;

void
usage(int only_version)
{
	printf("setkey @(#) %s (%s)\n", TOP_PACKAGE_STRING, TOP_PACKAGE_URL); 
	if (! only_version) {
		printf("usage: setkey [-v" RK_OPTS "] file ...\n");
		printf("       setkey [-nv" RK_OPTS "] -c\n");
		printf("       setkey [-nv" RK_OPTS "] -f filename\n");
		printf("       setkey [-Palpv" RK_OPTS "] -D\n");
		printf("       setkey [-Pv] -F\n");
		printf("       setkey [-H] -x\n");
		printf("       setkey [-V] [-h]\n");
	}
	exit(1);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	FILE *fp = stdin;
	int c;

	if (argc == 1) {
		usage(0);
		/* NOTREACHED */
	}

	thiszone = gmt2local(0);

	while ((c = getopt(argc, argv, "acdf:HlnvxDFPphVrk?")) != -1) {
		switch (c) {
		case 'c':
			f_mode = MODE_STDIN;
#ifdef HAVE_READLINE
			/* disable filename completion */
			rl_bind_key('\t', rl_insert);
#endif
			break;
		case 'f':
			f_mode = MODE_SCRIPT;
			if ((fp = fopen(optarg, "r")) == NULL) {
				err(1, "fopen");
				/*NOTREACHED*/
			}
			break;
		case 'D':
			f_mode = MODE_CMDDUMP;
			break;
		case 'F':
			f_mode = MODE_CMDFLUSH;
			break;
		case 'a':
			f_all = 1;
			break;
		case 'l':
			f_forever = 1;
			break;
		case 'n':
			f_notreally = 1;
			break;
#ifdef __NetBSD__
		case 'h':
#endif
		case 'H':
			f_hexdump = 1;
			break;
		case 'x':
			f_mode = MODE_PROMISC;
			f_tflag++;
			break;
		case 'P':
			f_policy = 1;
			break;
		case 'p':
			f_withports = 1;
			break;
		case 'v':
			f_verbose = 1;
			break;
		case 'r':
#ifdef HAVE_POLICY_FWD
			f_rfcmode = 1;
#else
			rkwarn();
#endif
			break;
		case 'k':
#ifdef HAVE_POLICY_FWD
			f_rfcmode = 0;
#else
			rkwarn();
#endif
			break;
		case 'V':
			usage(1);
			break;
			/*NOTREACHED*/
#ifndef __NetBSD__
		case 'h':
#endif
		case '?':
		default:
			usage(0);
			/*NOTREACHED*/
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 0) {
		while (argc--)
			if (fileproc(*argv++) < 0) {
				err(1, "%s", argv[-1]);
				/*NOTREACHED*/
			}
		exit(0);
	}

	so = pfkey_open();
	if (so < 0) {
		perror("pfkey_open");
		exit(1);
	}

	switch (f_mode) {
	case MODE_CMDDUMP:
		sendkeyshort(f_policy ? SADB_X_SPDDUMP : SADB_DUMP);
		break;
	case MODE_CMDFLUSH:
		sendkeyshort(f_policy ? SADB_X_SPDFLUSH: SADB_FLUSH);
		break;
	case MODE_SCRIPT:
		if (get_supported() < 0) {
			errx(1, "%s", ipsec_strerror());
			/*NOTREACHED*/
		}
		if (parse(&fp))
			exit (1);
		break;
	case MODE_STDIN:
		if (get_supported() < 0) {
			errx(1, "%s", ipsec_strerror());
			/*NOTREACHED*/
		}
		stdin_loop();
		break;
	case MODE_PROMISC:
		promisc();
		/*NOTREACHED*/
	default:
		usage(0);
		/*NOTREACHED*/
	}

	exit(0);
}

int
get_supported()
{

	if (pfkey_send_register(so, SADB_SATYPE_UNSPEC) < 0)
		return -1;

	if (pfkey_recv_register(so) < 0)
		return -1;

	return (0);
}

void
stdin_loop()
{
	char line[1024], *semicolon, *comment;
	size_t linelen = 0;
	
	memset (line, 0, sizeof(line));

	parse_init();
	while (1) {
#ifdef HAVE_READLINE
		char *rbuf;
		rbuf = readline ("");
		if (! rbuf)
			break;
#else
		char rbuf[1024];
		rbuf[0] = '\0';
		fgets (rbuf, sizeof(rbuf), stdin);
		if (!rbuf[0])
			break;
		if (rbuf[strlen(rbuf)-1] == '\n')
			rbuf[strlen(rbuf)-1] = '\0';
#endif
		comment = strchr(rbuf, '#');
		if (comment)
			*comment = '\0';

		if (!rbuf[0])
			continue;

		linelen += snprintf (&line[linelen], sizeof(line) - linelen,
				     "%s%s", linelen > 0 ? " " : "", rbuf);

		semicolon = strchr(line, ';');
		while (semicolon) {
			char saved_char = *++semicolon;
			*semicolon = '\0';
#ifdef HAVE_READLINE
			add_history (line);
#endif

#ifdef HAVE_PFKEY_POLICY_PRIORITY
			last_msg_type = -1;  /* invalid message type */
#endif

			parse_string (line);
			if (exit_now)
				return;
			if (saved_char) {
				*semicolon = saved_char;
				linelen = strlen (semicolon);
				memmove (line, semicolon, linelen + 1);
				semicolon = strchr(line, ';');
			}
			else {
				semicolon = NULL;
				linelen = 0;
			}
		}
	}
}

void
sendkeyshort(type)
        u_int type;
{
	struct sadb_msg msg;

	msg.sadb_msg_version = PF_KEY_V2;
	msg.sadb_msg_type = type;
	msg.sadb_msg_errno = 0;
	msg.sadb_msg_satype = SADB_SATYPE_UNSPEC;
	msg.sadb_msg_len = PFKEY_UNIT64(sizeof(msg));
	msg.sadb_msg_reserved = 0;
	msg.sadb_msg_seq = 0;
	msg.sadb_msg_pid = getpid();

	sendkeymsg((char *)&msg, sizeof(msg));

	return;
}

void
promisc()
{
	struct sadb_msg msg;
	u_char rbuf[1024 * 32];	/* XXX: Enough ? Should I do MSG_PEEK ? */
	ssize_t l;

	msg.sadb_msg_version = PF_KEY_V2;
	msg.sadb_msg_type = SADB_X_PROMISC;
	msg.sadb_msg_errno = 0;
	msg.sadb_msg_satype = 1;
	msg.sadb_msg_len = PFKEY_UNIT64(sizeof(msg));
	msg.sadb_msg_reserved = 0;
	msg.sadb_msg_seq = 0;
	msg.sadb_msg_pid = getpid();

	if ((l = send(so, &msg, sizeof(msg), 0)) < 0) {
		err(1, "send");
		/*NOTREACHED*/
	}

	while (1) {
		struct sadb_msg *base;

		if ((l = recv(so, rbuf, sizeof(*base), MSG_PEEK)) < 0) {
			err(1, "recv");
			/*NOTREACHED*/
		}

		if (l != sizeof(*base))
			continue;

		base = (struct sadb_msg *)rbuf;
		if ((l = recv(so, rbuf, PFKEY_UNUNIT64(base->sadb_msg_len),
				0)) < 0) {
			err(1, "recv");
			/*NOTREACHED*/
		}
		printdate();
		if (f_hexdump) {
			int i;
			for (i = 0; i < l; i++) {
				if (i % 16 == 0)
					printf("%08x: ", i);
				printf("%02x ", rbuf[i] & 0xff);
				if (i % 16 == 15)
					printf("\n");
			}
			if (l % 16)
				printf("\n");
		}
		/* adjust base pointer for promisc mode */
		if (base->sadb_msg_type == SADB_X_PROMISC) {
			if ((ssize_t)sizeof(*base) < l)
				base++;
			else
				base = NULL;
		}
		if (base) {
			kdebug_sadb(base);
			printf("\n");
			fflush(stdout);
		}
	}
}

/* Generate 'spi' array with SPIs matching 'satype', 'srcs', and 'dsts'
 * Return value is dynamically generated array of SPIs, also number of
 * SPIs through num_spi pointer.
 * On any error, set *num_spi to 0 and return NULL.
 */
u_int32_t *
sendkeymsg_spigrep(satype, srcs, dsts, num_spi)
	unsigned int satype;
	struct addrinfo *srcs;
	struct addrinfo *dsts;
	int *num_spi;
{
	struct sadb_msg msg, *m;
	char *buf;
	size_t len;
	ssize_t l;
	u_char rbuf[1024 * 32];
	caddr_t mhp[SADB_EXT_MAX + 1];
	struct sadb_address *saddr;
	struct sockaddr *s;
	struct addrinfo *a;
	struct sadb_sa *sa;
	u_int32_t *spi = NULL;
	int max_spi = 0, fail = 0;

	*num_spi = 0;

	if (f_notreally) {
		return NULL;
	}

    {
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(so, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt");
		return NULL;
	}
    }

	msg.sadb_msg_version = PF_KEY_V2;
	msg.sadb_msg_type = SADB_DUMP;
	msg.sadb_msg_errno = 0;
	msg.sadb_msg_satype = satype;
	msg.sadb_msg_len = PFKEY_UNIT64(sizeof(msg));
	msg.sadb_msg_reserved = 0;
	msg.sadb_msg_seq = 0;
	msg.sadb_msg_pid = getpid();
	buf = (char *)&msg;
	len = sizeof(msg);

	if (f_verbose) {
		kdebug_sadb(&msg);
		printf("\n");
	}
	if (f_hexdump) {
		int i;
		for (i = 0; i < len; i++) {
			if (i % 16 == 0)
				printf("%08x: ", i);
			printf("%02x ", buf[i] & 0xff);
			if (i % 16 == 15)
				printf("\n");
		}
		if (len % 16)
			printf("\n");
	}

	if ((l = send(so, buf, len, 0)) < 0) {
		perror("send");
		return NULL;
	}

	m = (struct sadb_msg *)rbuf;
	do {
		if ((l = recv(so, rbuf, sizeof(rbuf), 0)) < 0) {
			perror("recv");
			fail = 1;
			break;
		}

		if (PFKEY_UNUNIT64(m->sadb_msg_len) != l) {
			warnx("invalid keymsg length");
			fail = 1;
			break;
		}

		if (f_verbose) {
			kdebug_sadb(m);
			printf("\n");
		}

		if (m->sadb_msg_type != SADB_DUMP) {
			warnx("unexpected message type");
			fail = 1;
			break;
		}

		if (m->sadb_msg_errno != 0) {
			warnx("error encountered");
			fail = 1;
			break;
		}

		/* match satype */
		if (m->sadb_msg_satype != satype)
			continue;

		pfkey_align(m, mhp);
		pfkey_check(mhp);

		/* match src */
		saddr = (struct sadb_address *)mhp[SADB_EXT_ADDRESS_SRC];
		if (saddr == NULL)
			continue;
		s = (struct sockaddr *)(saddr + 1);
		for (a = srcs; a; a = a->ai_next)
			if (memcmp(a->ai_addr, s, a->ai_addrlen) == 0)
				break;
		if (a == NULL)
			continue;

		/* match dst */
		saddr = (struct sadb_address *)mhp[SADB_EXT_ADDRESS_DST];
		if (saddr == NULL)
			continue;
		s = (struct sockaddr *)(saddr + 1);
		for (a = dsts; a; a = a->ai_next)
			if (memcmp(a->ai_addr, s, a->ai_addrlen) == 0)
				break;
		if (a == NULL)
			continue;

		if (*num_spi >= max_spi) {
			max_spi += 512;
			spi = realloc(spi, max_spi * sizeof(u_int32_t));
		}

		sa = (struct sadb_sa *)mhp[SADB_EXT_SA];
		if (sa != NULL)
			spi[(*num_spi)++] = (u_int32_t)ntohl(sa->sadb_sa_spi);

		m = (struct sadb_msg *)((caddr_t)m + PFKEY_UNUNIT64(m->sadb_msg_len));

		if (f_verbose) {
			kdebug_sadb(m);
			printf("\n");
		}

	} while (m->sadb_msg_seq);

	if (fail) {
		free(spi);
		*num_spi = 0;
		return NULL;
	}

	return spi;
}

int
sendkeymsg(buf, len)
	char *buf;
	size_t len;
{
	u_char rbuf[1024 * 32];	/* XXX: Enough ? Should I do MSG_PEEK ? */
	ssize_t l;
	struct sadb_msg *msg;

	if (f_notreally) {
		goto end;
	}

    {
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(so, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt");
		goto end;
	}
    }

	if (f_forever)
		shortdump_hdr();
again:
	if (f_verbose) {
		kdebug_sadb((struct sadb_msg *)buf);
		printf("\n");
	}
	if (f_hexdump) {
		int i;
		for (i = 0; i < len; i++) {
			if (i % 16 == 0)
				printf("%08x: ", i);
			printf("%02x ", buf[i] & 0xff);
			if (i % 16 == 15)
				printf("\n");
		}
		if (len % 16)
			printf("\n");
	}

	if ((l = send(so, buf, len, 0)) < 0) {
		perror("send");
		goto end;
	}

	msg = (struct sadb_msg *)rbuf;
	do {
		if ((l = recv(so, rbuf, sizeof(rbuf), 0)) < 0) {
			perror("recv");
			goto end;
		}

		if (PFKEY_UNUNIT64(msg->sadb_msg_len) != l) {
			warnx("invalid keymsg length");
			break;
		}

		if (f_verbose) {
			kdebug_sadb(msg);
			printf("\n");
		}
		if (postproc(msg, l) < 0)
			break;
	} while (msg->sadb_msg_errno || msg->sadb_msg_seq);

	if (f_forever) {
		fflush(stdout);
		sleep(1);
		goto again;
	}

end:
	return (0);
}

int
postproc(msg, len)
	struct sadb_msg *msg;
	int len;
{
#ifdef HAVE_PFKEY_POLICY_PRIORITY
	static int priority_support_check = 0;
#endif

	if (msg->sadb_msg_errno != 0) {
		char inf[80];
		const char *errmsg = NULL;

		if (f_mode == MODE_SCRIPT)
			snprintf(inf, sizeof(inf), "The result of line %d: ", lineno);
		else
			inf[0] = '\0';

		switch (msg->sadb_msg_errno) {
		case ENOENT:
			switch (msg->sadb_msg_type) {
			case SADB_DELETE:
			case SADB_GET:
			case SADB_X_SPDDELETE:
				errmsg = "No entry";
				break;
			case SADB_DUMP:
				errmsg = "No SAD entries";
				break;
			case SADB_X_SPDDUMP:
				errmsg = "No SPD entries";
				break;
			}
			break;
		default:
			errmsg = strerror(msg->sadb_msg_errno);
		}
		printf("%s%s.\n", inf, errmsg);
		return (-1);
	}

	switch (msg->sadb_msg_type) {
	case SADB_GET:
		if (f_withports)
			pfkey_sadump_withports(msg);
		else
			pfkey_sadump(msg);
		break;

	case SADB_DUMP:
		/* filter out DEAD SAs */
		if (!f_all) {
			caddr_t mhp[SADB_EXT_MAX + 1];
			struct sadb_sa *sa;
			pfkey_align(msg, mhp);
			pfkey_check(mhp);
			if ((sa = (struct sadb_sa *)mhp[SADB_EXT_SA]) != NULL) {
				if (sa->sadb_sa_state == SADB_SASTATE_DEAD)
					break;
			}
		}
		if (f_forever) {
			/* TODO: f_withports */
			shortdump(msg);
		} else {
			if (f_withports)
				pfkey_sadump_withports(msg);
			else
				pfkey_sadump(msg);
		}
		msg = (struct sadb_msg *)((caddr_t)msg +
				     PFKEY_UNUNIT64(msg->sadb_msg_len));
		if (f_verbose) {
			kdebug_sadb((struct sadb_msg *)msg);
			printf("\n");
		}
		break;

	case SADB_X_SPDGET:
		if (f_withports) 
			pfkey_spdump_withports(msg);
		else
			pfkey_spdump(msg);
		break;

	case SADB_X_SPDDUMP:
		if (f_withports) 
			pfkey_spdump_withports(msg);
		else
			pfkey_spdump(msg);
		if (msg->sadb_msg_seq == 0) break;
		msg = (struct sadb_msg *)((caddr_t)msg +
				     PFKEY_UNUNIT64(msg->sadb_msg_len));
		if (f_verbose) {
			kdebug_sadb((struct sadb_msg *)msg);
			printf("\n");
		}
		break;
#ifdef HAVE_PFKEY_POLICY_PRIORITY
	case SADB_X_SPDADD:
		if (last_msg_type == SADB_X_SPDADD && last_priority != 0 && 
		    msg->sadb_msg_pid == getpid() && !priority_support_check) {
			priority_support_check = 1;	
			if (!verifypriority(msg))
				printf ("WARNING: Kernel does not support policy priorities\n");
		}
		break;
#endif
	}

	return (0);
}

#ifdef HAVE_PFKEY_POLICY_PRIORITY
int
verifypriority(m)
	struct sadb_msg *m;
{
	caddr_t mhp[SADB_EXT_MAX + 1];
	struct sadb_x_policy *xpl;

	/* check pfkey message. */
	if (pfkey_align(m, mhp)) {
		printf("(%s\n", ipsec_strerror());
		return 0;
	}
	if (pfkey_check(mhp)) {
		printf("%s\n", ipsec_strerror());
		return 0;
	}

	xpl = (struct sadb_x_policy *) mhp[SADB_X_EXT_POLICY];

	if (xpl == NULL) {
		printf("no X_POLICY extension.\n");
		return 0;
	}

	/* now make sure they match */
	if (last_priority != xpl->sadb_x_policy_priority)
		return 0;

	return 1; 
}
#endif

int
fileproc(filename)
	const char *filename;
{
	int fd;
	ssize_t len, l;
	u_char *p, *ep;
	struct sadb_msg *msg;
	u_char rbuf[1024 * 32];	/* XXX: Enough ? Should I do MSG_PEEK ? */

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return -1;

	l = 0;
	while (1) {
		len = read(fd, rbuf + l, sizeof(rbuf) - l);
		if (len < 0) {
			close(fd);
			return -1;
		} else if (len == 0)
			break;
		l += len;
	}

	if (l < sizeof(struct sadb_msg)) {
		close(fd);
		errno = EINVAL;
		return -1;
	}
	close(fd);

	p = rbuf;
	ep = rbuf + l;

	while (p < ep) {
		msg = (struct sadb_msg *)p;
		len = PFKEY_UNUNIT64(msg->sadb_msg_len);
		postproc(msg, len);
		p += len;
	}

	return (0);
}


/*------------------------------------------------------------*/
static const char *satype[] = {
	NULL, NULL, "ah", "esp"
};
static const char *sastate[] = {
	"L", "M", "D", "d"
};
static const char *ipproto[] = {
/*0*/	"ip", "icmp", "igmp", "ggp", "ip4",
	NULL, "tcp", NULL, "egp", NULL,
/*10*/	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, "udp", NULL, NULL,
/*20*/	NULL, NULL, "idp", NULL, NULL,
	NULL, NULL, NULL, NULL, "tp",
/*30*/	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
/*40*/	NULL, "ip6", NULL, "rt6", "frag6",
	NULL, "rsvp", "gre", NULL, NULL,
/*50*/	"esp", "ah", NULL, NULL, NULL,
	NULL, NULL, NULL, "icmp6", "none",
/*60*/	"dst6",
};

#define STR_OR_ID(x, tab) \
	(((x) < sizeof(tab)/sizeof(tab[0]) && tab[(x)])	? tab[(x)] : numstr(x))

const char *
numstr(x)
	int x;
{
	static char buf[20];
	snprintf(buf, sizeof(buf), "#%d", x);
	return buf;
}

void
shortdump_hdr()
{
	printf("%-4s %-3s %-1s %-8s %-7s %s -> %s\n",
		"time", "p", "s", "spi", "ltime", "src", "dst");
}

void
shortdump(msg)
	struct sadb_msg *msg;
{
	caddr_t mhp[SADB_EXT_MAX + 1];
	char buf[NI_MAXHOST], pbuf[NI_MAXSERV];
	struct sadb_sa *sa;
	struct sadb_address *saddr;
	struct sadb_lifetime *lts, *lth, *ltc;
	struct sockaddr *s;
	u_int t;
	time_t cur = time(0);

	pfkey_align(msg, mhp);
	pfkey_check(mhp);

	printf("%02lu%02lu", (u_long)(cur % 3600) / 60, (u_long)(cur % 60));

	printf(" %-3s", STR_OR_ID(msg->sadb_msg_satype, satype));

	if ((sa = (struct sadb_sa *)mhp[SADB_EXT_SA]) != NULL) {
		printf(" %-1s", STR_OR_ID(sa->sadb_sa_state, sastate));
		printf(" %08x", (u_int32_t)ntohl(sa->sadb_sa_spi));
	} else
		printf("%-1s %-8s", "?", "?");

	lts = (struct sadb_lifetime *)mhp[SADB_EXT_LIFETIME_SOFT];
	lth = (struct sadb_lifetime *)mhp[SADB_EXT_LIFETIME_HARD];
	ltc = (struct sadb_lifetime *)mhp[SADB_EXT_LIFETIME_CURRENT];
	if (lts && lth && ltc) {
		if (ltc->sadb_lifetime_addtime == 0)
			t = (u_long)0;
		else
			t = (u_long)(cur - ltc->sadb_lifetime_addtime);
		if (t >= 1000)
			strlcpy(buf, " big/", sizeof(buf));
		else
			snprintf(buf, sizeof(buf), " %3lu/", (u_long)t);
		printf("%s", buf);

		t = (u_long)lth->sadb_lifetime_addtime;
		if (t >= 1000)
			strlcpy(buf, "big", sizeof(buf));
		else
			snprintf(buf, sizeof(buf), "%-3lu", (u_long)t);
		printf("%s", buf);
	} else
		printf(" ??\?/???");	/* backslash to avoid trigraph ??/ */

	printf(" ");

	if ((saddr = (struct sadb_address *)mhp[SADB_EXT_ADDRESS_SRC]) != NULL) {
		if (saddr->sadb_address_proto)
			printf("%s ", STR_OR_ID(saddr->sadb_address_proto, ipproto));
		s = (struct sockaddr *)(saddr + 1);
		getnameinfo(s, sysdep_sa_len(s), buf, sizeof(buf),
			pbuf, sizeof(pbuf), NI_NUMERICHOST|NI_NUMERICSERV);
		if (strcmp(pbuf, "0") != 0)
			printf("%s[%s]", buf, pbuf);
		else
			printf("%s", buf);
	} else
		printf("?");

	printf(" -> ");

	if ((saddr = (struct sadb_address *)mhp[SADB_EXT_ADDRESS_DST]) != NULL) {
		if (saddr->sadb_address_proto)
			printf("%s ", STR_OR_ID(saddr->sadb_address_proto, ipproto));

		s = (struct sockaddr *)(saddr + 1);
		getnameinfo(s, sysdep_sa_len(s), buf, sizeof(buf),
			pbuf, sizeof(pbuf), NI_NUMERICHOST|NI_NUMERICSERV);
		if (strcmp(pbuf, "0") != 0)
			printf("%s[%s]", buf, pbuf);
		else
			printf("%s", buf);
	} else
		printf("?");

	printf("\n");
}

/* From: tcpdump(1):gmt2local.c and util.c */
/*
 * Print the timestamp
 */
static void
printdate()
{
	struct timeval tp;
	int s;

	if (gettimeofday(&tp, NULL) == -1) {
		perror("gettimeofday");
		return;
	}

	if (f_tflag == 1) {
		/* Default */
		s = (tp.tv_sec + thiszone ) % 86400;
		(void)printf("%02d:%02d:%02d.%06u ",
		    s / 3600, (s % 3600) / 60, s % 60, (u_int32_t)tp.tv_usec);
	} else if (f_tflag > 1) {
		/* Unix timeval style */
		(void)printf("%u.%06u ",
		    (u_int32_t)tp.tv_sec, (u_int32_t)tp.tv_usec);
	}

	printf("\n");
}

/*
 * Returns the difference between gmt and local time in seconds.
 * Use gmtime() and localtime() to keep things simple.
 */
int32_t
gmt2local(time_t t)
{
	register int dt, dir;
	register struct tm *gmt, *loc;
	struct tm sgmt;

	if (t == 0)
		t = time(NULL);
	gmt = &sgmt;
	*gmt = *gmtime(&t);
	loc = localtime(&t);
	dt = (loc->tm_hour - gmt->tm_hour) * 60 * 60 +
	    (loc->tm_min - gmt->tm_min) * 60;

	/*
	 * If the year or julian day is different, we span 00:00 GMT
	 * and must add or subtract a day. Check the year first to
	 * avoid problems when the julian day wraps.
	 */
	dir = loc->tm_year - gmt->tm_year;
	if (dir == 0)
		dir = loc->tm_yday - gmt->tm_yday;
	dt += dir * 24 * 60 * 60;

	return (dt);
}
