/*	$NetBSD: cpu.c,v 1.9 2002/03/13 13:12:29 simonb Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001 Jason R. Thorpe.
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

#include "opt_machtypes.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/cache.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpu_match, cpu_attach
};

static void
sgimips_find_l2cache(struct arcbios_component *comp,
    struct arcbios_treewalk_context *atc)
{
	struct device *self = atc->atc_cookie;

	if (comp->Class != COMPONENT_CLASS_CacheClass)
		return;

	switch (comp->Type) {
	case COMPONENT_TYPE_SecondaryICache:
		panic("%s: split L2 cache", self->dv_xname);
	case COMPONENT_TYPE_SecondaryDCache:
	case COMPONENT_TYPE_SecondaryCache:
		mips_sdcache_size = COMPONENT_KEY_Cache_CacheSize(comp->Key);
		mips_sdcache_line_size =
		    COMPONENT_KEY_Cache_LineSize(comp->Key);
		/* XXX */
		mips_sdcache_ways = 1;
		break;
	}
}

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
	/*
	 * Walk the ARCBIOS device tree to find the L2 cache.
	 *
	 * XXX We should be walking the tree to attach the CPUs,
	 * XXX etc, but we don't currently do that.
	 */
	arcbios_tree_walk(sgimips_find_l2cache, self);

	printf(": ");
	cpu_identify();

#ifdef IP22
	if (mach_type == MACH_SGI_IP22) {		/* XXX Indy */
		extern void ip22_cache_init(struct device *);
		ip22_cache_init(self);
	}
#endif
}
