/*	$NetBSD: test-policy-priority.c,v 1.4 2007/07/18 12:07:51 vanhu Exp $	*/

/*	$KAME: test-policy.c,v 1.16 2003/08/26 03:24:08 itojun Exp $	*/

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

#include <netinet/in.h>
#include <net/pfkeyv2.h>
#include PATH_IPSEC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include "libpfkey.h"

struct req_t {
	int result;	/* expected result; 0:ok 1:ng */
	int dump_result; /* dumped result expected to match original: 1:yes 0:no */
	char *str;
	u_int32_t expected_priority;
} reqs[] = {
#ifdef HAVE_PFKEY_POLICY_PRIORITY
{ 0, 0, "out ipsec esp/transport//require", PRIORITY_DEFAULT },
{ 0, 0, "out prio -1 ipsec esp/transport//require", PRIORITY_DEFAULT + 1 },
{ 0, 0, "out priority 2147483648 ipsec esp/transport//require", 0 },
{ 0, 1, "in prio def ipsec esp/transport//require", PRIORITY_DEFAULT },
{ 0, 1, "in prio low ipsec esp/transport//require", PRIORITY_LOW },
{ 0, 1, "in prio high ipsec esp/transport//require", PRIORITY_HIGH },
{ 0, 1, "in prio def + 1 ipsec esp/transport//require", PRIORITY_DEFAULT - 1 },
{ 0, 1, "in prio def - 1 ipsec esp/transport//require", PRIORITY_DEFAULT + 1},
{ 0, 1, "in prio low + 1 ipsec esp/transport//require", PRIORITY_LOW - 1 },
{ 0, 1, "in prio low - 1 ipsec esp/transport//require", PRIORITY_LOW + 1 },
{ 0, 1, "in prio high + 1 ipsec esp/transport//require", PRIORITY_HIGH - 1 },
{ 0, 1, "in prio high - 1 ipsec esp/transport//require", PRIORITY_HIGH + 1 },
{ 1, 0, "in prio low - -1 ipsec esp/transport//require", 0 },
{ 1, 0, "in prio low + high ipsec esp/transport//require", 0 },
#else
{ 0, 1, "out ipsec esp/transport//require", 0 },
{ 1, 0, "out prio -1 ipsec esp/transport//require", 0 },
{ 1, 0, "in prio def ipsec esp/transport//require", 0 },
{ 1, 0, "in prio def + 1 ipsec esp/transport//require", 0 },
#endif
};

int test1 __P((void));
int test1sub1 __P((struct req_t *));
int test1sub2 __P((char *, int));
int test2 __P((void));
int test2sub __P((int));

int
main(ac, av)
	int ac;
	char **av;
{
	return test1();
}

int
test1()
{
	int i;
	int result;
	int error = 0;

	printf("TEST1\n");
	for (i = 0; i < sizeof(reqs)/sizeof(reqs[0]); i++) {
		printf("#%d [%s]\n", i + 1, reqs[i].str);

		result = test1sub1(&reqs[i]);
		if (result == 0 && reqs[i].result == 1) {
			error = 1;
			warnx("ERROR: expecting failure.");
		} else if (result == 1 && reqs[i].result == 0) {
			error = 1;
			warnx("ERROR: expecting success.");
		}
	}

	return error;
}

int
test1sub1(req)
	struct req_t *req;
{
	char *policy;
	char *policy_str;
	struct sadb_x_policy *xpl;

	int len;

	policy = ipsec_set_policy(req->str, strlen(req->str));
	if (policy == NULL) {
		if (req->result == 0) {
			printf("ipsec_set_policy: %s\n", ipsec_strerror());
		}
		return 1;
	}

#ifdef HAVE_PFKEY_POLICY_PRIORITY
	/* check priority matches expected */
	xpl = (struct sadb_x_policy *)policy;
	if (xpl->sadb_x_policy_priority != req->expected_priority) {
		printf("Actual priority %u does not match expected priority %u\n",
		       xpl->sadb_x_policy_priority, req->expected_priority);
		free(policy);
		return 1;
	}
#endif

	if (req->dump_result) {
		/* invert policy */
		len = ipsec_get_policylen(policy);
		if ((policy_str = ipsec_dump_policy(policy, NULL)) == NULL) {
			printf("%s\n", ipsec_strerror());
			free(policy);
			return 1;
		}

		/* check that they match */
		if (strcmp(req->str, policy_str) != 0) {
			printf("ipsec_dump_policy result (%s) does not match original "
			       "(%s)\n", policy_str, req->str);
			free(policy_str);
			free(policy);
			return 1;
		}

		free(policy_str);
	}

	free(policy);
	return 0;
}
