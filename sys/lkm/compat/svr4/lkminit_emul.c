/* $NetBSD: lkminit_emul.c,v 1.8 2003/03/04 10:39:10 dsl Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: lkminit_emul.c,v 1.8 2003/03/04 10:39:10 dsl Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/lkm.h>

extern const struct emul emul_svr4;

int compat_svr4_lkmentry __P((struct lkm_table *, int, int));

static int svr4_init __P((struct lkm_table *lkmtp, int cmd));
static int svr4_done __P((struct lkm_table *lkmtp, int cmd));

/*
 * declare the emulation
 */
MOD_COMPAT("compat_svr4", -1, &emul_svr4);

/*
 * entry point
 */
int
compat_svr4_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;	
	int cmd;
	int ver;
{

	DISPATCH(lkmtp, cmd, ver, svr4_init, svr4_done, lkm_nofunc);
}

static int
svr4_init(lkmtp, cmd)
	struct lkm_table *lkmtp;
	int cmd;
{
#ifdef i386
	/*
	 * XXX Yeah, this is ugly.
	 * Ideally, there would be some compat init/done routine, called
	 * by both this code and i386/machdep.c. However, that seems like
	 * overkill given that only svr4 compat needs an initialization.
	 */
#define	IDTVEC(name)	__CONCAT(X, name)
	extern void IDTVEC(svr4_fasttrap) __P((void));

	setgate(&idt[0xd2], &IDTVEC(svr4_fasttrap), 0, SDT_SYS386TGT,
		SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
#endif

	return (0);
}

static int
svr4_done(struct lkm_table *lkmtp, int cmd)
{
#ifdef i386
	/* XXX is this right? Wouldn't this cause null pointer dereference
	 * if some userland code would use the gate after the LKM is unloaded?
	 */
	setgate(&idt[0xd2], NULL, 0, SDT_SYS386TGT,
		SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
#endif

	return (0);
}
