/*-
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: src/sbin/gpt/destroy.c,v 1.6 2005/08/31 01:47:19 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: destroy.c,v 1.8 2015/12/01 09:05:33 christos Exp $");
#endif

#include <sys/types.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"
#include "gpt_private.h"

static int recoverable;
static int force;

const char destroymsg[] = "destroy [-rf]";

static int
usage_destroy(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), destroymsg);
	return -1;
}

static int
destroy(gpt_t gpt)
{
	map_t pri_hdr, sec_hdr;

	pri_hdr = map_find(gpt, MAP_TYPE_PRI_GPT_HDR);
	sec_hdr = map_find(gpt, MAP_TYPE_SEC_GPT_HDR);

	if (pri_hdr == NULL && sec_hdr == NULL) {
		gpt_warnx(gpt, "Device doesn't contain a GPT");
		return -1;
	}

	if (recoverable && sec_hdr == NULL) {
		gpt_warnx(gpt, "Recoverability not possible");
		return -1;
	}

	if (pri_hdr != NULL) {
		memset(pri_hdr->map_data, 0, gpt->secsz);
		if (gpt_write(gpt, pri_hdr) == -1) {
			gpt_warnx(gpt, "Error writing primary header");
			return -1;
		}
	}

	if (!recoverable && sec_hdr != NULL) {
		memset(sec_hdr->map_data, 0, gpt->secsz);
		if (gpt_write(gpt, sec_hdr) == -1) {
			gpt_warnx(gpt, "Error writing backup header");
			return -1;
		}
	}

	return 0;
}

int
cmd_destroy(gpt_t gpt, int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "fr")) != -1) {
		switch(ch) {
		case 'f':
			force = 1;
			break;
		case 'r':
			recoverable = 1;
			break;
		default:
			return usage_destroy();
		}
	}

	if (argc != optind)
		return usage_destroy();

	return destroy(gpt);
}
