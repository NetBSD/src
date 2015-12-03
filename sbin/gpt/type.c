/*-
 * Copyright (c) 2004 Marcel Moolenaar
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
__FBSDID("$FreeBSD: src/sbin/gpt/remove.c,v 1.10 2006/10/04 18:20:25 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: type.c,v 1.12 2015/12/03 01:07:28 christos Exp $");
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

static int cmd_type(gpt_t, int, char *[]);

static const char *typehelp[] = {
	"-a -T newtype",
	"[-b blocknr] [-i index] [-L label] [-s sectors] [-t type] -T newtype",
};

struct gpt_cmd c_type = {
	"type",
	cmd_type,
	typehelp, __arraycount(typehelp),
	0,
};

#define usage() gpt_usage(NULL, &c_type)

static void
change(struct gpt_ent *ent, void *v)
{
	gpt_uuid_t *newtype = v;
	gpt_uuid_copy(ent->ent_type, *newtype);
}

static int
cmd_type(gpt_t gpt, int argc, char *argv[])
{
	int ch;
	gpt_uuid_t newtype;
	struct gpt_find find;

	memset(&find, 0, sizeof(find));
	gpt_uuid_copy(newtype, gpt_uuid_nil);
	find.msg = "type changed";

	/* Get the type options */
	while ((ch = getopt(argc, argv, GPT_FIND "T:")) != -1) {
		switch(ch) {
		case 'T':
			if (gpt_uuid_get(gpt, &newtype) == -1)
				return -1;
			break;
		default:
			if (gpt_add_find(gpt, &find, ch) == -1)
				return usage();
			break;
		}
	}

	if (gpt_uuid_is_nil(newtype) || argc != optind)
		return usage();

	return gpt_change_ent(gpt, &find, change, &newtype);
}
