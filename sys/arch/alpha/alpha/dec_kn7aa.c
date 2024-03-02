/* $NetBSD: dec_kn7aa.c,v 1.1 2024/03/02 20:15:33 thorpej Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_kn7aa.c,v 1.1 2024/03/02 20:15:33 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <machine/rpb.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>

void		dec_kn7aa_init(void);
void		dec_kn7aa_cons_init(void);
static void	dec_kn7aa_device_register(device_t, void *);

const struct alpha_variation_table dec_kn7aa_variations[] = {
	{ 0, "DEC 7000" },
	{ 1, "DEC 10000" },
	{ 0, NULL },
};

void
dec_kn7aa_init(void)
{
	uint64_t variation;

	platform.family = "KN7AA (\"Ruby\")";

	if ((platform.model = alpha_dsr_sysname()) == NULL) {
		variation = hwrpb->rpb_variation & SV_ST_MASK;
		if ((platform.model = alpha_variation_name(variation,
		    dec_kn7aa_variations)) == NULL)
			platform.model = alpha_unknown_sysname();
	}

	platform.iobus = "lsb";
	platform.cons_init = dec_kn7aa_cons_init;
	platform.device_register = dec_kn7aa_device_register;
}

void
dec_kn7aa_cons_init(void)
{
}

static void
dec_kn7aa_device_register(device_t dev, void *aux)
{
}
