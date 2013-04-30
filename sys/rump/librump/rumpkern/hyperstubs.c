/*	$NetBSD: hyperstubs.c,v 1.2 2013/04/30 14:28:52 pooka Exp $	*/

/*
 * Copyright (c) 2013 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: hyperstubs.c,v 1.2 2013/04/30 14:28:52 pooka Exp $");

#include <sys/errno.h>

/* provide weak aliases for optional rump kernel hypervisor features */

int rump_hypernotsupp(void);
int rump_hypernotsupp(void) { return ENOTSUP; }

/* dynlinking */

__weak_alias(rumpuser_dl_bootstrap,rump_hypernotsupp);
__weak_alias(rumpuser_dl_globalsym,rump_hypernotsupp);

/* race-free daemon creation */

__weak_alias(rumpuser_daemonize_begin,rump_hypernotsupp);
__weak_alias(rumpuser_daemonize_done,rump_hypernotsupp);

/* process env */

__weak_alias(rumpuser_kill,rump_hypernotsupp);

/* anonmmap (for proplib and modules) */

__weak_alias(rumpuser_anonmmap,rump_hypernotsupp);
__weak_alias(rumpuser_unmap,rump_hypernotsupp);

/* syscall proxy */

__weak_alias(rumpuser_sp_init,rump_hypernotsupp);
__weak_alias(rumpuser_sp_copyin,rump_hypernotsupp);
__weak_alias(rumpuser_sp_copyinstr,rump_hypernotsupp);
__weak_alias(rumpuser_sp_copyout,rump_hypernotsupp);
__weak_alias(rumpuser_sp_copyoutstr,rump_hypernotsupp);
__weak_alias(rumpuser_sp_anonmmap,rump_hypernotsupp);
__weak_alias(rumpuser_sp_raise,rump_hypernotsupp);
__weak_alias(rumpuser_sp_fini,rump_hypernotsupp);
