/*	$NetBSD: cpu.c,v 1.5 2001/07/08 23:59:32 thorpej Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpu_match, cpu_attach
};

static int
cpu_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
cpu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	u_int32_t config;

	switch (mach_type) {
	case MACH_SGI_IP22:
		mips_L2CacheSize = 1024 * 1024;		/* XXX Indy */
		break;

	case MACH_SGI_IP32:
		mips_L2CacheSize = 512 * 1024;		/* XXX O2 */
		break;
	}

#if 1	/* XXX XXX XXX */
	config = mips3_cp0_config_read();
	config &= ~MIPS3_CONFIG_SC;
	mips3_cp0_config_write(config);
#endif

	printf(": ");
	cpu_identify();

	if (mach_type == MACH_SGI_IP22) {		/* XXX Indy */
		unsigned long tmp1, tmp2, tmp3;

		printf("%s: disabling IP22 SysAD L2 cache\n", self->dv_xname);

	        __asm__ __volatile__("
                .set noreorder
                .set mips3
                li      %0, 0x1
                dsll    %0, 31
                lui     %1, 0x9000
                dsll32  %1, 0
                or      %0, %1, %0
                mfc0    %2, $12
                nop; nop; nop; nop;
                li      %1, 0x80
                mtc0    %1, $12
                nop; nop; nop; nop;
                sh      $0, 0(%0)
                mtc0    $0, $12
                nop; nop; nop; nop;  
                mtc0    %2, $12 
                nop; nop; nop; nop;
                .set mips2
                .set reorder
        	" : "=r" (tmp1), "=r" (tmp2), "=r" (tmp3));
	}

#if 0
	mips_L2CacheSize = 0;
	mips_L2CachePresent = 0;
#endif
}
