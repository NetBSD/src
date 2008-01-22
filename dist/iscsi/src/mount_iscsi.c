/* $NetBSD: mount_iscsi.c,v 1.3.4.2 2008/01/22 19:06:06 bouyer Exp $ */

/*
 * Copyright © 2005 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__COPYRIGHT("@(#) Copyright © 2005 \
	        The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: mount_iscsi.c,v 1.3.4.2 2008/01/22 19:06:06 bouyer Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "iscsi.h"
#include "initiator.h"
#include "tests.h"

static int	verbose;

enum {
	ISCSI_DOMAIN_NAME_SIZE = 512,
	ISCSI_PRODUCT_NAME_SIZE = 256,
	ISCSI_TARGET_NAME_SIZE = 256,
	ISCSI_LUN_NAME_SIZE = 64,

	MAX_DOMAIN_PARTS = 100
};

/* iSCSI addresses are specified using an IQN, or iSCSI Qualified Name */
/* this structure is used to unpack the components of an IQN */
typedef struct iqn_t {
	char			revdomain[ISCSI_DOMAIN_NAME_SIZE];
	char			domain[ISCSI_DOMAIN_NAME_SIZE];
	char			product[ISCSI_PRODUCT_NAME_SIZE];
	char			target[ISCSI_TARGET_NAME_SIZE];
	char			lun[ISCSI_LUN_NAME_SIZE];
	struct sockaddr_in	sock;
	int			socklen;
} iqn_t;

/* regular expression used to parse IQNs */
#define IQN_PARSE	"iqn\\.[^.]*\\.(.*)\\.([^.]*)\\.([^.]*)\\.([^.]*)"

/* copy a name to an output array */
static void
copyname(char *array, size_t size, char *s, int from, int to, const char *field)
{
	if (to - from > size - 1) {
		warnx("truncating %s `%.*s'",  field, (int)(to - from), &s[from]);
	}
	(void) strlcpy(array, &s[from], MIN((int)(to - from + 1), size));
}

/* reverse the components of the domain name */
static void
reversedomain(char *in, char *out, size_t size)
{
	char	*cp;
	char	*dots[MAX_DOMAIN_PARTS];
	int	 dotc;
	int	 i;

	for (cp = in, dotc = 0 ; *cp != 0x0 && dotc < MAX_DOMAIN_PARTS ; cp++) {
		if (*cp == '.') {
			dots[dotc++] = cp;
		}
	}
	(void) memset(out, 0x0, size);
	for (cp = out, i = dotc - 1 ; i >= 0 ; --i) {
		cp += snprintf(cp, size - (int)(cp - out), "%s%.*s",
			(i == dotc - 1) ? "" : ".",
			(i == dotc - 1) ? strlen(dots[i]) - 1 : (int)(dots[i + 1] - dots[i] - 1),
			dots[i] + 1);
	}
	(void) snprintf(cp, size - (int)(cp - out), "%s%.*s", ".", (int)(dots[0] - in), in);
}

/* iqn.2003-02.com.alistaircrooks.product.target.lun */
static int
iqnparse(char *s, iqn_t *iqn)
{
	static int	done;
	regmatch_t	matchv[10];
	regex_t		r;

	(void) memset(iqn, 0x0, sizeof(*iqn));
	if (!done) {
		(void) memset(&r, 0x0, sizeof(r));
		if (regcomp(&r, IQN_PARSE, REG_EXTENDED) != 0) {
			warn("failed to compile iqn parse string");
		}
		done = 1;
	}
	if (regexec(&r, s, 10, matchv, 0) != 0) {
		warnx("iqnparse: invalid IQN - `%s'", s);
		return 0;
	}

	/* get the domain name from the iqn and reverse it */
	copyname(iqn->revdomain, sizeof(iqn->revdomain), s, (int)(matchv[1].rm_so), (int)(matchv[1].rm_eo), "revdomain");
	reversedomain(iqn->revdomain, iqn->domain, sizeof(iqn->domain));

	/* get the product name from the iqn */
	copyname(iqn->product, sizeof(iqn->product), s, (int)(matchv[2].rm_so), (int)(matchv[2].rm_eo), "product");

	/* get the target name from the iqn */
	copyname(iqn->target, sizeof(iqn->target), s, (int)(matchv[3].rm_so), (int)(matchv[3].rm_eo), "target");

	/* get the lun from the iqn */
	copyname(iqn->lun, sizeof(iqn->lun), s, (int)(matchv[4].rm_so), (int)(matchv[4].rm_eo), "lun");

	return 1;
}

/* make a connection to an IQN */
static int
make_connection(char *hostname, iqn_t *iqn)
{
	struct addrinfo		 hints;
	struct addrinfo		*res;
	struct addrinfo		*res0 = NULL;
	volatile int		 s;
	in_port_t		 portnum;
	char			 hbuf[NI_MAXHOST];
	char			 service[32];
	char			*host;
	int			 error;

	(void) memset(iqn, 0x0, sizeof(*iqn));
	(void) iqnparse(hostname, iqn);
	printf("revdomain: `%s'\n", iqn->revdomain);
	printf("domain: `%s'\n", iqn->domain);
	printf("product: `%s'\n", iqn->product);
	printf("target: `%s'\n", iqn->target);
	printf("lun: `%s'\n", iqn->lun);

	(void) memset(&hints, 0x0, sizeof(hints));
	(void) strlcpy(service, "iscsi", sizeof(service));
	hints.ai_flags = 0;
	hints.ai_family = AF_INET;	/* expand this to IPv6 at the right time - XXX */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if ((error = getaddrinfo(host = iqn->domain, NULL, &hints, &res0)) != 0) {
		warnx("%s: %s", host, gai_strerror(error));
		return 0;
	}
	if (res0->ai_canonname) {
		host = res0->ai_canonname;
	}

	s = -1;
	for (res = res0; res; res = res->ai_next) {
#if 0
		/* XXX fixme */
		/*
		 * see comment in hookup()
		 */
		ai_unmapped(res);
#endif
		if (getnameinfo(res->ai_addr, res->ai_addrlen, hbuf, sizeof(hbuf), service, sizeof(service), NI_NUMERICHOST) != 0) {
			strlcpy(hbuf, "invalid", sizeof(hbuf));
		}

		((struct sockaddr_in *)res->ai_addr)->sin_port = htons(portnum);
		(void) memcpy(&iqn->sock, &res->ai_addr, sizeof(res->ai_addr));
		iqn->socklen = res->ai_addrlen;
		return 1;

#if 0
		if ((s = socket(res->ai_family, SOCK_STREAM, res->ai_protocol)) < 0) {
			warn("Can't create socket");
			continue;
		}

		if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
			warn("Connect to address `%s'", hbuf);
			close(s);
			s = -1;
			continue;
		}
#endif
	}

	return 0;
}

int
main(int argc, char **argv)
{
	struct sigaction	act;
	iqn_t			iqn;
	int             	begin_tid = 0;
	int             	end_tid = CONFIG_INITIATOR_NUM_TARGETS;
	int             	lun = 0;
	int			i;
	int			j;

	while ((i = getopt(argc, argv, "v")) != -1) {
		switch(i) {
		case 'v':
			verbose += 1;
			break;
		default:
			errx(EXIT_FAILURE, "Unknown option `%c'", i);
			break;
		}
	}
	if (argc - optind != 2) {
		errx(EXIT_FAILURE, "Usage: %s [-v] iqn mountpoint\n", *argv);
	}
	(void) memset(&iqn, 0x0, sizeof(iqn));
	if (!make_connection(argv[optind], &iqn)) {
		exit(EXIT_FAILURE);
	}
	for (j = 0; j < 1; j++) {

		printf("<ITER %i>\n", j);

		/* Ignore sigpipe */

		act.sa_handler = SIG_IGN;
		sigaction(SIGPIPE, &act, NULL);

		/* Initialize Initiator */

		if (initiator_init(iqn.domain) == -1) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_init() failed\n");
			return -1;
		}
		/* Run tests for each target */

		for (i = begin_tid; i < end_tid; i++) {
			if (test_all(i, lun) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "test_all() failed\n");
				return -1;
			}
		}

		/* Shutdown Initiator */

		if (initiator_shutdown() == -1) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_shutdown() failed\n");
			return -1;
		}
	}
	exit(EXIT_SUCCESS);
}
