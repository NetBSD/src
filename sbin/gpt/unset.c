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
__FBSDID("$FreeBSD: src/sbin/gpt/add.c,v 1.14 2006/06/22 22:05:28 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: unset.c,v 1.11 2015/12/03 02:02:43 christos Exp $");
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

static int cmd_unset(gpt_t, int, char *[]);

static const char *unsethelp[] = {
	"-a attribute -i index",
};

struct gpt_cmd c_unset = {
	"unset",
	cmd_unset,
	unsethelp, __arraycount(unsethelp),
	0,
};

#define usage() gpt_usage(NULL, &c_unset)

static int
cmd_unset(gpt_t gpt, int argc, char *argv[])
{
	int ch;
	unsigned int entry = 0;
	uint64_t attributes = 0;

	while ((ch = getopt(argc, argv, "a:i:")) != -1) {
		switch(ch) {
		case 'a':
			if (gpt_attr_get(&attributes) == -1)
				return usage();
			break;
		case 'i':
			if (gpt_uint_get(&entry) == -1)
				return usage();
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	return gpt_attr_update(gpt, entry, 0, attributes);
}
