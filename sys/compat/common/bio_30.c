/*	$NetBSD: bio_30.c,v 1.1.2.1 2018/03/28 04:18:24 pgoyette Exp $ */
/*	$OpenBSD: bio.c,v 1.9 2007/03/20 02:35:55 marco Exp $	*/

/*
 * Copyright (c) 2002 Niklas Hallqvist.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bio_30.c,v 1.1.2.1 2018/03/28 04:18:24 pgoyette Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/compat_stub.h>

#include <dev/biovar.h>
#include <dev/sysmon/sysmonvar.h>

#include <compat/common/compat_mod.h>

static int
compat_30_bio(void *cookie, u_long cmd, void *addr,
    int (*delegate)(void *, u_long, void *))
{
	int error;

	switch (cmd) {
	case OBIOCDISK: {
		struct bioc_disk *bd =
		    malloc(sizeof(*bd), M_DEVBUF, M_WAITOK|M_ZERO);

		(void)memcpy(bd, addr, sizeof(struct obioc_disk));
		error = (*delegate)(cookie, BIOCDISK, bd);
		if (error) {
			free(bd, M_DEVBUF);
			return error;
		}

		(void)memcpy(addr, bd, sizeof(struct obioc_disk));
		free(bd, M_DEVBUF);
		return 0;
	}
	case OBIOCVOL: {
		struct bioc_vol *bv =
		    malloc(sizeof(*bv), M_DEVBUF, M_WAITOK|M_ZERO);

		(void)memcpy(bv, addr, sizeof(struct obioc_vol));
		error = (*delegate)(cookie, BIOCVOL, bv);
		if (error) {
			free(bv, M_DEVBUF);
			return error;
		}

		(void)memcpy(addr, bv, sizeof(struct obioc_vol));
		free(bv, M_DEVBUF);
		return 0;
	}
	default:
		return ENOSYS;
	}
}

void
bio_30_init(void)
{

	compat_bio_30 = compat_30_bio;
}

void
bio_30_fini(void)
{

	compat_bio_30 = (void *)enosys;
}
