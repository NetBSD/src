/*	$NetBSD: hyperentropy.c,v 1.7.2.3 2017/12/03 11:39:16 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: hyperentropy.c,v 1.7.2.3 2017/12/03 11:39:16 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <rump-sys/kern.h>

#include <rump/rumpuser.h>

static kmutex_t rndsrc_lock;
static krndsource_t rndsrc;

#define MAXGET (RND_POOLBITS/NBBY)
static void
feedrandom(size_t bytes, void *cookie __unused)
{
	uint8_t *rnddata;
	size_t n, nread;

	rnddata = kmem_intr_alloc(MAXGET, KM_SLEEP);
	n = 0;
	while (n < MIN(MAXGET, bytes)) {
		if (rumpuser_getrandom(rnddata + n, MIN(MAXGET, bytes) - n,
			RUMPUSER_RANDOM_HARD|RUMPUSER_RANDOM_NOWAIT, &nread)
		    != 0)
			break;
		n += MIN(nread, MIN(MAXGET, bytes) - n);
	}
	if (n) {
		mutex_enter(&rndsrc_lock);
		rnd_add_data_sync(&rndsrc, rnddata, n, NBBY*n);
		mutex_exit(&rndsrc_lock);
	}
	kmem_intr_free(rnddata, MAXGET);
}

void
rump_hyperentropy_init(void)
{

	mutex_init(&rndsrc_lock, MUTEX_DEFAULT, IPL_VM);

	rndsource_setcb(&rndsrc, &feedrandom, NULL);
	rnd_attach_source(&rndsrc, "rump_hyperent", RND_TYPE_VM,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
	feedrandom(MAXGET, NULL);
}
