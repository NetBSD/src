/*	$NetBSD: rumpuser_cache.c,v 1.1 2014/06/17 06:31:47 alnsn Exp $	*/

/*-
 * Copyright (c) 2014 Alexander Nasonov.
 * All Rights Reserved.
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

#include "rumpuser_port.h"

#if !defined(lint)
__RCSID("$NetBSD: rumpuser_cache.c,v 1.1 2014/06/17 06:31:47 alnsn Exp $");
#endif /* !lint */

#include <sys/types.h>

#include <machine/sysarch.h>
#include <machine/types.h>

#include <rump/rumpuser.h>


int
rumpuser_sync_icache(void *addr, uint64_t len)
{
#if defined(__arm__)
	/*
	 * arm_sync_icache is recommended over sysarch but
	 * it adds a link dependency -l${MACHINE_CPU}.
	 */
	struct arm_sync_icache_args args;

	args.addr = (uintptr_t)addr;
	args.len = (size_t)len;

	return sysarch(ARM_SYNC_ICACHE, (void *)&args);
#else /* XXX mips_icache_sync_range */

	return 0;
#endif
}
