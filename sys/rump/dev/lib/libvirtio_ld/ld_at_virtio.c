/*	$NetBSD: ld_at_virtio.c,v 1.1 2014/08/22 09:57:05 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: ld_at_virtio.c,v 1.1 2014/08/22 09:57:05 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/stat.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

#include "ioconf.c"

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{

	config_init_component(cfdriver_ioconf_virtio_ld,
	    cfattach_ioconf_virtio_ld, cfdata_ioconf_virtio_ld);
}

RUMP_COMPONENT(RUMP_COMPONENT_VFS)
{
	extern const struct bdevsw ld_bdevsw;
	extern const struct cdevsw ld_cdevsw;
	devmajor_t bmaj = -1, cmaj = -1;
	int error;

	if ((error = devsw_attach("ld", &ld_bdevsw, &bmaj,
	    &ld_cdevsw, &cmaj)) != 0)
		panic("cannot attach ld: %d", error);
        
	if ((error = rump_vfs_makedevnodes(S_IFBLK, "/dev/ld0", 'a',
	    bmaj, 0, 7)) != 0)
		panic("cannot create cooked ld dev nodes: %d", error);
	if ((error = rump_vfs_makedevnodes(S_IFCHR, "/dev/rld0", 'a',
	    cmaj, 0, 7)) != 0)
		panic("cannot create raw ld dev nodes: %d", error);
}
