/*	$NetBSD: hyperentropy.c,v 1.3 2014/08/10 16:44:36 tls Exp $	*/

/*
 * Copyright (c) 2014 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: hyperentropy.c,v 1.3 2014/08/10 16:44:36 tls Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/rnd.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

static krndsource_t rndsrc;

#define MAXGET (RND_POOLBITS/NBBY)
static void
feedrandom(size_t bytes, void *arg)
{
	uint8_t *rnddata;
	size_t dsize;

	rnddata = kmem_intr_alloc(MAXGET, KM_SLEEP);
	if (rumpuser_getrandom(rnddata, MIN(MAXGET, bytes),
	    RUMPUSER_RANDOM_HARD|RUMPUSER_RANDOM_NOWAIT, &dsize) == 0)
		rnd_add_data(&rndsrc, rnddata, dsize, NBBY*dsize);
	kmem_intr_free(rnddata, MAXGET);
}

void
rump_hyperentropy_init(void)
{

	if (rump_threads) {
		rndsource_setcb(&rndsrc, feedrandom, &rndsrc);
		rnd_attach_source(&rndsrc, "rump_hyperent", RND_TYPE_VM,
		    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
	} else {
		/* without threads, just fill the pool */
		rnd_attach_source(&rndsrc, "rump_hyperent", RND_TYPE_VM,
		    RND_FLAG_COLLECT_VALUE);
		feedrandom(RND_POOLBITS/NBBY, NULL);
	}
}
