/*	$NetBSD: rnd_component.c,v 1.1.10.3 2017/12/03 11:39:09 jdolecek Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: rnd_component.c,v 1.1.10.3 2017/12/03 11:39:09 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/rnd.h>
#include <sys/stat.h>

#include <rump-sys/kern.h>
#include <rump-sys/dev.h>
#include <rump-sys/vfs.h>

#include "ioconf.h"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern const struct cdevsw rnd_cdevsw;
	devmajor_t bmaj, cmaj;
	int error;

	/* go, mydevfs */
	bmaj = cmaj = -1;

	if ((error = devsw_attach("random", NULL, &bmaj,
	    &rnd_cdevsw, &cmaj)) != 0)
		panic("cannot attach rnd: %d", error);

	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/random",
	    cmaj, RND_DEV_RANDOM)) != 0)
		panic("cannot create /dev/random: %d", error);
	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/urandom",
	    cmaj, RND_DEV_URANDOM)) != 0)
		panic("cannot create /dev/urandom: %d", error);

	rump_pdev_add(rndattach, 4);
	rnd_init();
}

#if 0
/*
 * XXX: the following hack works around PR kern/51135 and should ASAP be
 * nuked to and then from orbit.
 */
#define RNDPRELOAD 256
#include <sys/rndio.h>
RUMP_COMPONENT(RUMP_COMPONENT_POSTINIT)
{
	rnddata_t *rd;
	size_t dsize, i;

	CTASSERT(RNDPRELOAD <= sizeof(rd->data));

	aprint_verbose("/dev/random: "
	    "loading initial entropy to workaround PR kern/51135\n");
	rd = kmem_alloc(sizeof(*rd), KM_SLEEP);
	for (i = 0; i < RNDPRELOAD; i += dsize) {
		if (rumpuser_getrandom(rd->data,
		    RNDPRELOAD-i, RUMPUSER_RANDOM_HARD, &dsize) != 0)
			panic("rumpuser_getrandom failed"); /* XXX */
		rd->len = dsize;
		rd->entropy = dsize*NBBY;
		if (rnd_system_ioctl(NULL, RNDADDDATA, rd))
			panic("rnd_system_ioctl failed"); /* XXX */
	}
}
#endif
