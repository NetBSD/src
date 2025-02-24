/* $NetBSD: devpath.c,v 1.1 2025/02/24 13:47:56 christos Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: devpath.c,v 1.1 2025/02/24 13:47:56 christos Exp $");
#endif /* not lint */

#include <sys/queue.h>

#include <err.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "defs.h"
#include "devpath.h"
#include "devpath1.h"
#include "devpath2.h"
#include "devpath3.h"
#include "devpath4.h"
#include "devpath5.h"

#define easprintf	(size_t)easprintf

typedef SIMPLEQ_HEAD(devpath_head, devpath_blk) devpath_head_t;

typedef struct devpath_blk {
	devpath_elm_t	path;
	devpath_elm_t	dbg;
	SIMPLEQ_ENTRY(devpath_blk) entry;
} devpath_blk_t;

static void
devpath_end(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	assert(dp->Type == 0x7f);
	assert(dp->Length == 4);

	switch (dp->SubType) {
	case 1:
		path->cp = estrdup("");	/* end of devpath instance */
		path->sz = 1;
		break;
	case 0xff:
		path->cp = NULL;	/* end of entire devpath */
		path->sz = 0;
		break;
	default:
		path->sz = easprintf(&path->cp,
		    "unknown device path end subtype: %u\n", dp->SubType);
		break;
	}

	if (dbg != NULL)
		devpath_hdr(dp, dbg);
}

static char *
collapse_list(devpath_head_t *head, size_t plen, char **dmsg, size_t dlen)
{
	devpath_blk_t *blk, *next;
	char *bp, *path;

	bp = path = emalloc(plen + 1);
	SIMPLEQ_FOREACH_SAFE(blk, head, entry, next) {
		if (blk->path.cp == NULL) {
			*bp = '\0';
			assert(next == NULL);
			break;
		}
		else if (*blk->path.cp == '\0') {
			*bp++ = ':';
			next = SIMPLEQ_NEXT(blk, entry);
		}
		else {
			bp = stpcpy(bp, blk->path.cp);
			if (next->path.cp != NULL && *next->path.cp != '\0')
				*bp++ = '/';
		}
		free(blk->path.cp);
	}
	if (dmsg) {
		bp = *dmsg = emalloc(dlen + 1);
		SIMPLEQ_FOREACH_SAFE(blk, head, entry, next) {
			bp = stpcpy(bp, blk->dbg.cp);
			free(blk->dbg.cp);
		}
	}
	return path;
}

static void
devpath_parse_core(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	switch (dp->Type) {
	case DEVPATH_TYPE_HW:	 devpath_hw(dp, path, dbg);	return; /* Type 1 */
	case DEVPATH_TYPE_ACPI:	 devpath_acpi(dp, path, dbg);	return; /* Type 2 */
	case DEVPATH_TYPE_MSG:	 devpath_msg(dp, path, dbg);	return; /* Type 3 */
	case DEVPATH_TYPE_MEDIA: devpath_media(dp, path, dbg);	return; /* Type 4 */
	case DEVPATH_TYPE_BIOS:	 devpath_bios(dp, path, dbg);	return; /* Type 5 */
	case DEVPATH_TYPE_END:	 devpath_end(dp, path, dbg);	return; /* Type 0x7F */
	default:		 devpath_unsupported(dp, path, dbg);	return;
	}
}

PUBLIC char *
devpath_parse(devpath_t *dp, size_t dplen, char **dmsg)
{
	devpath_head_t head = SIMPLEQ_HEAD_INITIALIZER(head);
	devpath_blk_t *blk;
	union {
		char *cp;
		devpath_t *dp;
	} u;
	size_t dlen = 0, plen = 0;
	char *ep;

	if (dmsg)
		*dmsg = NULL;

	u.dp = dp;
	ep = u.cp + dplen;
	for (/*EMPTY*/; u.cp < ep; u.cp += u.dp->Length) {
		blk = ecalloc(1, sizeof(*blk));
		devpath_parse_core(u.dp, &blk->path, dmsg ? &blk->dbg : NULL);
		plen += blk->path.sz;
		dlen += blk->dbg.sz;
		SIMPLEQ_INSERT_TAIL(&head, blk, entry);
	}

	return collapse_list(&head, plen, dmsg, dlen);
}
