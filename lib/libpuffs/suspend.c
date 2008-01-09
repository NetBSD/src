/*	$NetBSD: suspend.c,v 1.5.4.2 2008/01/09 01:36:53 matt Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
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
#if !defined(lint)
__RCSID("$NetBSD: suspend.c,v 1.5.4.2 2008/01/09 01:36:53 matt Exp $");
#endif /* !lint */

/*
 * File system suspension
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>
#include <unistd.h>

#include "puffs_priv.h"

/*ARGSUSED*/
int
puffs_fs_suspend(struct puffs_usermount *pu)
{
	struct puffs_req preq;
	size_t n;

	preq.preq_pth.pth_framelen = sizeof(struct puffs_req);
	preq.preq_opclass = PUFFSOP_SUSPEND;

	n = write(pu->pu_fd, &preq, sizeof(preq));

	/* XXX */
	assert(n == sizeof(preq));
	return 0;
}
