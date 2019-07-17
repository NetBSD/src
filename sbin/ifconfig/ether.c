/*	$NetBSD: ether.c,v 1.6 2019/07/17 03:26:24 msaitoh Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ether.c,v 1.6 2019/07/17 03:26:24 msaitoh Exp $");
#endif /* not lint */

#include <sys/param.h> 
#include <sys/ioctl.h> 

#include <net/if.h> 
#include <net/if_ether.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "env.h"
#include "parse.h"
#include "extern.h"
#include "prog_ops.h"

static void ether_status(prop_dictionary_t, prop_dictionary_t);
static void ether_constructor(void) __attribute__((constructor));
static int  setethercaps(prop_dictionary_t, prop_dictionary_t);

static status_func_t status;
static cmdloop_branch_t branch;

#define MAX_PRINT_LEN 55

static const struct kwinst ethercapskw[] = {
	IFKW("vlan-hwfilter",	ETHERCAP_VLAN_HWFILTER),
	IFKW("vlan-hwtagging",	ETHERCAP_VLAN_HWTAGGING),
	IFKW("eee",		ETHERCAP_EEE)
};

struct pkw ethercaps = PKW_INITIALIZER(&ethercaps, "ethercaps", setethercaps,
    "ethercap", ethercapskw, __arraycount(ethercapskw),
    &command_root.pb_parser);

void
do_setethercaps(prop_dictionary_t env)
{
	struct eccapreq eccr;
	prop_data_t d;

	d = (prop_data_t )prop_dictionary_get(env, "ethercaps");
	if (d == NULL)
		return;

	assert(sizeof(eccr) == prop_data_size(d));

	memcpy(&eccr, prop_data_data_nocopy(d), sizeof(eccr));
	if (direct_ioctl(env, SIOCSETHERCAP, &eccr) == -1)
		err(EXIT_FAILURE, "SIOCSETHERCAP");
}

static int
getethercaps(prop_dictionary_t env, prop_dictionary_t oenv,
    struct eccapreq *oeccr)
{
	bool rc;
	struct eccapreq eccr;
	const struct eccapreq *tmpeccr;
	prop_data_t capdata;

	capdata = (prop_data_t)prop_dictionary_get(env, "ethercaps");

	if (capdata != NULL) {
		tmpeccr = prop_data_data_nocopy(capdata);
		*oeccr = *tmpeccr;
		return 0;
	}

	(void)direct_ioctl(env, SIOCGETHERCAP, &eccr);
	*oeccr = eccr;

	capdata = prop_data_create_data(&eccr, sizeof(eccr));

	rc = prop_dictionary_set(oenv, "ethercaps", capdata);

	prop_object_release((prop_object_t)capdata);

	return rc ? 0 : -1;
}

static int
setethercaps(prop_dictionary_t env, prop_dictionary_t oenv)
{
	int64_t ethercap;
	bool rc;
	prop_data_t capdata;
	struct eccapreq eccr;

	rc = prop_dictionary_get_int64(env, "ethercap", &ethercap);
	assert(rc);

	if (getethercaps(env, oenv, &eccr) == -1)
		return -1;

	if (ethercap < 0) {
		ethercap = -ethercap;
		eccr.eccr_capenable &= ~ethercap;
	} else
		eccr.eccr_capenable |= ethercap;

	if ((capdata = prop_data_create_data(&eccr, sizeof(eccr))) == NULL)
		return -1;

	rc = prop_dictionary_set(oenv, "ethercaps", capdata);
	prop_object_release((prop_object_t)capdata);

	return rc ? 0 : -1;
}

void
ether_status(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct eccapreq eccr;
	char fbuf[BUFSIZ];
	char *bp;

	memset(&eccr, 0, sizeof(eccr));

	if (direct_ioctl(env, SIOCGETHERCAP, &eccr) == -1)
		return;

	if (eccr.eccr_capabilities != 0) {
		(void)snprintb_m(fbuf, sizeof(fbuf), ECCAPBITS,
		    eccr.eccr_capabilities, MAX_PRINT_LEN);
		bp = fbuf;
		while (*bp != '\0') {
			printf("\tec_capabilities=%s\n", &bp[2]);
			bp += strlen(bp) + 1;
		}
		(void)snprintb_m(fbuf, sizeof(fbuf), ECCAPBITS,
		    eccr.eccr_capenable, MAX_PRINT_LEN);
		bp = fbuf;
		while (*bp != '\0') {
			printf("\tec_enabled=%s\n", &bp[2]);
			bp += strlen(bp) + 1;
		}
	}
}

static void
ether_constructor(void)
{

	cmdloop_branch_init(&branch, &ethercaps.pk_parser);
	register_cmdloop_branch(&branch);
	status_func_init(&status, ether_status);
	register_status(&status);
}
