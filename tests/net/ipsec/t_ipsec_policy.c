/*	$NetBSD: t_ipsec_policy.c,v 1.2 2026/09/18 13:33:46 riastradh Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_ipsec_policy.c,v 1.2 2026/09/18 13:33:46 riastradh Exp $");

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <net/pfkeyv2.h>
#include <netinet/in.h>
#include <netipsec/ipsec.h>

#include <atf-c.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "h_macros.h"

static void
hexdump(const char *title, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t i;

	printf("# %s (%zu bytes)\n", title, len);
	for (i = 0; i < len; i++) {
		if ((i % 8) == 0)
			printf(" ");
		printf(" %02hhx", p[i]);
		if ((i % 16) == 15)
			printf("\n");
	}
	if (i % 16)
		printf("\n");
}

static void *
append(void **bufp, size_t *buflenp, size_t addlen)
{
	void *p;

	if (addlen == 0)
		return NULL;

	ATF_REQUIRE(addlen < SIZE_MAX - *buflenp);
	RZ(reallocarr(bufp, *buflenp + addlen, 1));
	p = (char *)*bufp + *buflenp;
	memset(p, 0, addlen);
	*buflenp += addlen;
	return p;
}

static void
test_ipsec_policy_roundtrip(int exp_error, uint8_t dir,
    const struct sockaddr *src, const struct sockaddr *dst)
{
	enum { guardlen =  8 };
	void *buf = NULL;
	size_t len = 0;
	struct sadb_x_policy *xpl, *xpl1;
	size_t xpl_start, xpl1_start;
	size_t reqlen;
	void *copybuf = NULL;
	size_t copylen = 0;
	uint8_t guardbyte[2] = {0x5a, 0x3c};
	size_t guard_start[2];
	socklen_t optlen;
	unsigned i;
	int s;

	xpl_start = len;
	xpl = append(&buf, &len, sizeof(*xpl));
	xpl->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	xpl->sadb_x_policy_type = IPSEC_POLICY_IPSEC;
	xpl->sadb_x_policy_dir = dir;
	xpl = NULL;

	for (i = 0; i < 2; i++) {
		const size_t xisr_start = len;
		struct sadb_x_ipsecrequest *xisr;

		xisr = append(&buf, &len, sizeof(*xisr));
		xisr->sadb_x_ipsecrequest_proto = IPPROTO_ESP;
		xisr->sadb_x_ipsecrequest_mode = IPSEC_MODE_TUNNEL;
		xisr->sadb_x_ipsecrequest_level = IPSEC_LEVEL_REQUIRE;
		xisr->sadb_x_ipsecrequest_reqid = 0;
		xisr = NULL;

		memcpy(append(&buf, &len, src->sa_len), src, src->sa_len);
		memcpy(append(&buf, &len, dst->sa_len), dst, dst->sa_len);

		/* pad */
		const size_t npad = PFKEY_ALIGN8(len - xisr_start) -
		    (len - xisr_start);
		(void)append(&buf, &len, npad);

		ATF_REQUIRE(PFKEY_ALIGN8(len - xisr_start) == len - xisr_start);
		xisr = (void *)((char *)buf + xisr_start);
		xisr->sadb_x_ipsecrequest_len = len - xisr_start;
	}

	ATF_REQUIRE(PFKEY_ALIGN8(len - xpl_start) == len - xpl_start);
	xpl = (void *)((char *)buf + xpl_start);
	xpl->sadb_x_policy_len = PFKEY_UNIT64(len - xpl_start);

	reqlen = len - xpl_start;

	RL(s = socket(AF_INET6, SOCK_DGRAM, 0));
	if (exp_error) {
		ATF_CHECK_ERRNO(exp_error,
		    setsockopt(s, IPPROTO_IPV6, IPV6_IPSEC_POLICY, xpl, reqlen)
		    == -1);
		goto out;
	}
	if (setsockopt(s, IPPROTO_IPV6, IPV6_IPSEC_POLICY, xpl, reqlen)
	    == -1) {
		int error = errno;

		atf_tc_fail_nonfatal("setsockopt(IPV6_IPSEC_POLICY): %d (%s)",
		    error, strerror(errno));
		goto out;
	}

	guard_start[0] = copylen;
	memset(append(&copybuf, &copylen, guardlen), guardbyte[0], guardlen);

	xpl1_start = copylen;
	(void *)append(&copybuf, &copylen, reqlen);

	guard_start[1] = copylen;
	memset(append(&copybuf, &copylen, guardlen), guardbyte[1], guardlen);

	xpl = (void *)((char *)buf + xpl_start);
	xpl1 = (void *)((char *)copybuf + xpl1_start);
	memcpy(xpl1, xpl, reqlen);

	optlen = reqlen;
	ATF_CHECK_ERRNO(EINVAL,
	    getsockopt(s, IPPROTO_IPV6, IPV6_IPSEC_POLICY, xpl1, &optlen));

	optlen = reqlen;
	if (getsockopt2(s, IPPROTO_IPV6, IPV6_IPSEC_POLICY, xpl1, &optlen)
	    == -1) {
		int error = errno;

		atf_tc_fail_nonfatal("getsockopt2(IPV6_IPSEC_POLICY): %d (%s)",
		    error, strerror(errno));
		goto out;
	}
	ATF_CHECK_EQ_MSG(reqlen, optlen, "reqlen=%zu optlen=%zu",
	    (size_t)reqlen, (size_t)optlen);

	if (reqlen != optlen || memcmp(xpl, xpl1, len) != 0) {
		hexdump("before", xpl, reqlen);
		hexdump("after", xpl1, optlen);
		fflush(stdout);
		atf_tc_fail_nonfatal("mismatch");
	}

	for (i = 0; i < __arraycount(guard_start); i++) {
		const char *guard = (char *)copybuf + guard_start[i];
		size_t j;

		for (j = 0; j < guardlen; j++) {
			if (guard[j] != guardbyte[i]) {
				char title[8];

				snprintf(title, sizeof(title), "guard %u", i);
				hexdump(title, guard, guardlen);
				fflush(stdout);
				atf_tc_fail_nonfatal("guard %u overwritten",
				    i);
				break;
			}
		}
	}

out:	RL(close(s));
}

ATF_TC(pr60669);
ATF_TC_HEAD(pr60669, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test IPV6_IPSEC_POLICY round-trip with"
	    " v4 src and v4 dst");
}
ATF_TC_BODY(pr60669, tc)
{
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
	} v4[2] = {
		[0] = {
			.sin = {
				.sin_len = sizeof(v4[0].sin),
				.sin_family = AF_INET,
				.sin_port = 0,
			}
		},
		[1] = {
			.sin = {
				.sin_len = sizeof(v4[1].sin),
				.sin_family = AF_INET,
				.sin_port = 0,
			}
		},
	};
	union {
		struct sockaddr sa;
		struct sockaddr_in6 sin6;
	} v6[2] = {
		[0] = {
			.sin6 = {
				.sin6_len = sizeof(v6[0].sin6),
				.sin6_family = AF_INET6,
				.sin6_port = 0,
			}
		},
		[1] = {
			.sin6 = {
				.sin6_len = sizeof(v6[1].sin6),
				.sin6_family = AF_INET6,
				.sin6_port = 0,
			}
		},
	};
	union {
		struct sockaddr sa;
		struct sockaddr_un sun;
	} un[2] = {
		[0] = {
			.sun = {
				.sun_len = -1,
				.sun_family = AF_LOCAL,
				.sun_path = "/socket0",
			}
		},
		[1] = {
			.sun = {
				.sun_len = -1,
				.sun_family = AF_LOCAL,
				.sun_path = "/socket1",
			}
		},
	};
	struct sockaddr sa[2] = {
		[0] = { .sa_len = sizeof(sa[0]), .sa_family = AF_UNSPEC },
		[1] = { .sa_len = sizeof(sa[1]), .sa_family = AF_UNSPEC },
	};
	struct sockaddr sa1[2] = {
		[0] = { .sa_len = 1, .sa_family = AF_UNSPEC },
		[1] = { .sa_len = 1, .sa_family = AF_UNSPEC },
	};
	struct sockaddr sa0[2] = {
		[0] = { .sa_len = 0, .sa_family = AF_UNSPEC },
		[1] = { .sa_len = 0, .sa_family = AF_UNSPEC },
	};
	const struct {
		struct sockaddr *src;
		struct sockaddr *dst;
		int expected_error;
		const char *xfail;
	} C[] = {
		[0] = { &v4[0].sa, &v4[1].sa, 0, NULL },
		[1] = { &v4[0].sa, &v6[1].sa, EINVAL, NULL },
		[2] = { &v4[0].sa, &un[1].sa, EINVAL, NULL },
		[3] = { &v4[0].sa, &sa[1], EINVAL, NULL },
		[4] = { &v4[0].sa, &sa1[1], EINVAL, NULL },
		[5] = { &v4[0].sa, &sa0[1], EINVAL, NULL },

		[6] = { &v6[0].sa, &v4[1].sa, EINVAL, NULL },
		[7] = { &v6[0].sa, &v6[1].sa, 0, NULL },
		[8] = { &v6[0].sa, &un[1].sa, EINVAL, NULL },
		[9] = { &v6[0].sa, &sa[1], EINVAL, NULL },
		[10] = { &v6[0].sa, &sa1[1], EINVAL, NULL },
		[11] = { &v6[0].sa, &sa0[1], EINVAL, NULL },

		[12] = { &un[0].sa, &v4[1].sa, EINVAL, NULL },
		[13] = { &un[0].sa, &v6[1].sa, EINVAL, NULL },
		[14] = { &un[0].sa, &un[1].sa, EINVAL, NULL },
		[15] = { &un[0].sa, &sa[1], EINVAL, NULL },
		[16] = { &un[0].sa, &sa1[1], EINVAL, NULL },
		[17] = { &un[0].sa, &sa0[1], EINVAL, NULL },

		[18] = { &sa[0], &v4[1].sa, EINVAL, NULL },
		[19] = { &sa[0], &v6[1].sa, EINVAL, NULL },
		[20] = { &sa[0], &un[1].sa, EINVAL, NULL },
		[21] = { &sa[0], &sa[1], EINVAL, NULL },
		[22] = { &sa[0], &sa1[1], EINVAL, NULL },
		[23] = { &sa[0], &sa0[1], EINVAL, NULL },

		[24] = { &sa1[0], &v4[1].sa, EINVAL, NULL },
		[25] = { &sa1[0], &v6[1].sa, EINVAL, NULL },
		[26] = { &sa1[0], &un[1].sa, EINVAL, NULL },
		[27] = { &sa1[0], &sa[1], EINVAL, NULL },
		[28] = { &sa1[0], &sa1[1], EINVAL, NULL },
		[29] = { &sa1[0], &sa0[1], EINVAL, NULL },

		/*
		 * sa0 as the src one doesn't even make sense to test,
		 * because sa_len won't even advance past it, so it
		 * will be the same as testing a nonempty src and sa0
		 * dst above.
		 */
	};
	unsigned i;

	RL(inet_pton(AF_INET, "192.0.2.42", &v4[0].sin.sin_addr));
	RL(inet_pton(AF_INET, "192.51.100.54", &v4[1].sin.sin_addr));

	RL(inet_pton(AF_INET6, "2001:db8:1::1", &v6[0].sin6.sin6_addr));
	RL(inet_pton(AF_INET6, "2001:db8:2::1", &v6[1].sin6.sin6_addr));

	un[0].sun.sun_len = SUN_LEN(&un[0].sun);
	un[1].sun.sun_len = SUN_LEN(&un[1].sun);

	for (i = 0; i < __arraycount(C); i++) {
		const char *const dirname[2] = { "inbound", "outbound" };
		const unsigned dir[2] =
		    { IPSEC_DIR_INBOUND, IPSEC_DIR_OUTBOUND };
		unsigned j;

		for (j = 0; j < 2; j++) {
			printf("# case %u %s\n", i, dirname[j]);
			fflush(stdout);
			if (C[i].xfail)
				atf_tc_expect_fail("%s", C[i].xfail);
			test_ipsec_policy_roundtrip(C[i].expected_error,
			    dir[j], C[i].src, C[i].dst);
			if (C[i].xfail)
				atf_tc_expect_pass();
		}
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, pr60669);
	return atf_no_error();
}
