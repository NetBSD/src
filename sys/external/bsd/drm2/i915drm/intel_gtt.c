/*	$NetBSD: intel_gtt.c,v 1.3 2014/05/23 22:59:23 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

/* Intel GTT stubs */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_gtt.c,v 1.3 2014/05/23 22:59:23 riastradh Exp $");

#include <sys/types.h>			/* XXX pcivar.h needs...@!&#^  */

#include <dev/pci/pcivar.h>		/* XXX agpvar.h needs...  */
#include <dev/pci/agpvar.h>
#include <dev/pci/agp_i810var.h>

#include "drm/intel-gtt.h"

bool
intel_enable_gtt(void)
{

	/*
	 * The agp_i810 initialization code already enables the GTT, as
	 * far as I can tell.
	 *
	 * XXX That might not be correct.
	 */
	return (agp_i810_sc != NULL);
}

void
intel_gtt_chipset_flush(void)
{

	KASSERT(agp_i810_sc != NULL);
	agp_i810_chipset_flush(agp_i810_sc->as_chipc);
}
