/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Adam Glass BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: if_le_subr.c,v 1.6 1994/09/20 16:21:44 gwr Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <net/if.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/isr.h>
#include <machine/obio.h>
#include <machine/idprom.h>

#include "bpfilter.h"

#include "if_lereg.h"
#include "if_le.h"

extern int leintr();

int
le_md_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
    caddr_t le_addr;
    struct obio_cf_loc *obio_loc = (struct obio_cf_loc *) CFDATA_LOC(cf);

    le_addr = OBIO_DEFAULT_PARAM(caddr_t, obio_loc->obio_addr, OBIO_AMD_ETHER);
    return !obio_probe_byte(le_addr);
}

/*
 * things to do:
 *    allocate dvma area memory for dual access
 *    use/default config parameters for lance configuration
 *    setup isr handler
 *    set ethernet address
 *    set meta address
 *
 */

void
le_md_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	caddr_t dvma_malloc();
	int le_addr, level;
	struct le_softc *le = (struct le_softc *) self;
	struct obio_cf_loc *obio_loc = OBIO_LOC(self);
	
	/* allocate "shared" memory */
	le->sc_r2 = (struct lereg2 *) dvma_malloc(sizeof(struct lereg2)); 
	if (!le->sc_r2)
		panic(": not enough dvma space");
	idprom_etheraddr(le->sc_addr); /* ethernet addr */
	le_addr = OBIO_DEFAULT_PARAM(int, obio_loc->obio_addr, OBIO_AMD_ETHER);

	/* register access */
	le->sc_r1 = (struct lereg1 *)
		obio_alloc(le_addr, OBIO_AMD_ETHER_SIZE);
	if (!le->sc_r1)
		panic(": not enough obio space\n");
	level = OBIO_DEFAULT_PARAM(int, obio_loc->obio_level, 3);
	obio_print(le_addr, level);
	le->sc_machdep = NULL;
	isr_add(level, leintr, (int)le);
}
