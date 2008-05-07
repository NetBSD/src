/*	$NetBSD: agr.c,v 1.9 2008/05/07 19:55:24 dyoung Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: agr.c,v 1.9 2008/05/07 19:55:24 dyoung Exp $");
#endif /* !defined(lint) */

#include <sys/param.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/agr/if_agrioctl.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "env.h"
#include "parse.h"
#include "extern.h"
#include "agr.h"

static int checkifname(const char *);
static void assertifname(const char *);

static const struct kwinst agrkw[] = {
	  {.k_word = "agrport", .k_type = KW_T_NUM, .k_neg = true,
	   .k_num = AGRCMD_ADDPORT, .k_negnum = AGRCMD_REMPORT,
	   .k_exec = agrsetport}
};

struct pkw agr = PKW_INITIALIZER(&agr, "agr", NULL, NULL,
    agrkw, __arraycount(agrkw), NULL);

static int
checkifname(const char *ifname)
{

	return strncmp(ifname, "agr", 3) != 0 ||
	    !isdigit((unsigned char)ifname[3]);
}

static void
assertifname(const char *ifname)
{

	if (checkifname(ifname)) {
		errx(EXIT_FAILURE, "valid only with agr(4) interfaces");
	}
}

int
agrsetport(prop_dictionary_t env, prop_dictionary_t xenv)
{
	char buf[IFNAMSIZ];
	struct agrreq ar;
	int s;
	const char *port;
	const char *ifname;
	struct ifreq ifr;
	int64_t cmd;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	if (!prop_dictionary_get_int64(env, "agrcmd", &cmd)) {
		warnx("%s.%d", __func__, __LINE__);
		errno = ENOENT;
		return -1;
	}

	if (!prop_dictionary_get_cstring_nocopy(env, "agrport", &port)) {
		warnx("%s.%d", __func__, __LINE__);
		errno = ENOENT;
		return -1;
	}
	strlcpy(buf, port, sizeof(buf));

	memset(&ifr, 0, sizeof(ifr));
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);
	assertifname(ifname);
	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	memset(&ar, 0, sizeof(ar));
	ar.ar_version = AGRREQ_VERSION;
	ar.ar_cmd = cmd;
	ar.ar_buf = buf;
	ar.ar_buflen = strlen(buf);
	ifr.ifr_data = &ar;

	if (ioctl(s, SIOCSETAGR, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETAGR");
	return 0;
}

void
agr_status(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct agrreq ar;
	void *buf = NULL;
	size_t buflen = 0;
	struct agrportlist *apl;
	struct agrportinfo *api;
	int i;
	int s;
	const char *ifname;
	struct ifreq ifr;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	memset(&ifr, 0, sizeof(ifr));
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);
	if (checkifname(ifname))
		return;
	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

again:
	memset(&ar, 0, sizeof(ar));
	ar.ar_version = AGRREQ_VERSION;
	ar.ar_cmd = AGRCMD_PORTLIST;
	ar.ar_buf = buf;
	ar.ar_buflen = buflen;
	ifr.ifr_data = &ar;

	if (ioctl(s, SIOCGETAGR, &ifr) == -1) {
		if (errno != E2BIG) {
			warn("SIOCGETAGR");
			return;
		}

		free(buf);
		buf = NULL;
		buflen = 0;
		goto again;
	}

	if (buf == NULL) {
		buflen = ar.ar_buflen;
		buf = malloc(buflen);
		if (buf == NULL) {
			err(EXIT_FAILURE, "agr_status");
		}
		goto again;
	}

	apl = buf;
	api = (void *)(apl + 1);

	for (i = 0; i < apl->apl_nports; i++) {
		char tmp[256];

		snprintb(tmp, sizeof(tmp), AGRPORTINFO_BITS, api->api_flags);
		printf("\tagrport: %s, flags=%s\n", api->api_ifname, tmp);
		api++;
	}
}
