/*	$NetBSD: cpu_cons.c,v 1.9 2003/08/07 16:28:54 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University, Ralph Campbell, Sony Corp. and Kazumasa
 * Utashiro of Software Research Associates, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University, Ralph Campbell, Sony Corp. and Kazumasa
 * Utashiro of Software Research Associates, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_cons.c,v 1.9 2003/08/07 16:28:54 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/adrsmap.h>
#include <machine/cpu.h>
#include <dev/cons.h>

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 * XXX need something better here.
 */
#define	SCC_CONSOLE	0
#define	SW_CONSOLE	0x07
#define	SW_NWB512	0x04
#define	SW_NWB225	0x01
#define	SW_FBPOP	0x02
#define	SW_FBPOP1	0x06
#define	SW_FBPOP2	0x03
#define	SW_AUTOSEL	0x07

struct consdev *cn_tab = NULL;
extern struct consdev consdev_zs, consdev_zs_ap;

void fb_cnattach(void);
void kb_hb_cnattach(void);

void xafb_cnattach(void);
void kb_ap_cnattach(void);

#include "fb.h"
#include "xafb.h"
#include "zsc.h"

void
consinit()
{
	volatile int *dipsw;
	static int initted = 0;

	if (initted)
		return;

	initted = 1;

	switch (systype) {

#ifdef news3400
	case NEWS3400:
		dipsw = (void *)DIP_SWITCH;

#if NFB > 0
		if (*dipsw & SW_CONSOLE) {
			fb_cnattach();
			kb_hb_cnattach();
			break;
		}
#endif

#if NZSC > 0
		cn_tab = &consdev_zs;
		(*cn_tab->cn_init)(cn_tab);
#endif
		break;

#endif /* news3400 */

#ifdef news5000
	case NEWS5000:
		dipsw = (void *)NEWS5000_DIP_SWITCH;

#if NXAFB > 0
		if (*dipsw & SW_CONSOLE) {
			xafb_cnattach();
			kb_ap_cnattach();
			break;
		}
#endif

#if NZSC > 0
		cn_tab = &consdev_zs_ap;
		(*cn_tab->cn_init)(cn_tab);
#endif
		break;

#endif /* news5000 */
	}
}
