/*	$NetBSD: npfext_log.c,v 1.2 2013/03/11 00:03:18 christos Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npfext_log.c,v 1.2 2013/03/11 00:03:18 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

#include <npf.h>

int		npfext_log_init(void);
nl_ext_t *	npfext_log_construct(const char *);
int		npfext_log_param(nl_ext_t *, const char *, const char *);

int
npfext_log_init(void)
{
	/* Nothing to initialise. */
	return 0;
}

nl_ext_t *
npfext_log_construct(const char *name)
{
	assert(strcmp(name, "log") == 0);
	return npf_ext_construct(name);
}

int
npfext_log_param(nl_ext_t *ext, const char *param, const char *val __unused)
{
	unsigned long if_idx;

	assert(param != NULL);

	if_idx = if_nametoindex(param);
	if (if_idx == 0) {
		int s;
		struct ifreq ifr;
		struct ifconf ifc;

		if ((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
			warn("Can't create datagram socket for `%s'", param);
			return errno;
		}

		memset(&ifr, 0, sizeof(ifr));
		strlcpy(ifr.ifr_name, param, sizeof(ifr.ifr_name));
		if (ioctl(s, SIOCIFCREATE, &ifr) == -1) {
			warn("Can't SIOCIFCREATE `%s'", param);
			close(s);
			return errno;
		}

		memset(&ifc, 0, sizeof(ifc));
		strlcpy(ifr.ifr_name, param, sizeof(ifr.ifr_name));
		if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) {
			warn("Can't SIOCGIFFLAGS `%s'", param);
			close(s);
			return errno;
		}

		ifr.ifr_flags |= IFF_UP;
		if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
			warn("Can't SIOSGIFFLAGS `%s'", param);
			close(s);
			return errno;
		}
		close(s);

		if_idx = if_nametoindex(param);
		if (if_idx == 0)
			return EINVAL;
	}
	npf_ext_param_u32(ext, "log-interface", if_idx);
	return 0;
}
