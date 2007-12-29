/* $NetBSD: machdep.c,v 1.1 2007/12/29 14:38:36 jmcneill Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1 2007/12/29 14:38:36 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/buf.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include "opt_memsize.h"

char machine[] = "usermode";
char machine_arch[] = "usermode";

paddr_t		proc0paddr = 0;
int		usermode_x = IPL_NONE;
/* XXX */
int		physmem = MEMSIZE * 1024 / PAGE_SIZE;
struct vm_map	*mb_map = NULL;
struct vm_map	*exec_map = NULL;

void	start(void);

void
start(void)
{
	extern void pmap_bootstrap(void);

	printf("NetBSD/usermode startup\n");

	uvm_setpagesize();
	uvmexp.ncolors = 2;

	pmap_bootstrap();
}

void
microtime(struct timeval *tv)
{
#if notyet
	extern void __libc_gettimeofday(struct timeval *);
	__libc_gettimeofday(tv);
#endif
}

void
setstatclockrate(int arg)
{
}

void
consinit(void)
{
	extern void ttycons_consinit(void);

	ttycons_consinit();

	/* XXX we start in main, not start */
	start();
}

void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
}

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
}
