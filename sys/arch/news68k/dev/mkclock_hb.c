/*	$NetBSD: mkclock_hb.c,v 1.2 2002/02/23 17:18:55 scw Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>

#include <news68k/news68k/clockvar.h>

#include <news68k/dev/hbvar.h>

int	mkclock_hb_match __P((struct device *, struct cfdata  *, void *));
void	mkclock_hb_attach __P((struct device *, struct device *, void *));

struct cfattach mkclock_hb_ca = {
	sizeof(struct device), mkclock_hb_match, mkclock_hb_attach
};

extern struct cfdriver mkclock_cd;

int
mkclock_hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	static int mkclock_hb_matched;

	/* Only one clock, please. */
	if (mkclock_hb_matched)
		return (0);

	if (strcmp(ha->ha_name, mkclock_cd.cd_name))
		return (0);

	ha->ha_size = MK48T02_CLKSZ;

	mkclock_hb_matched = 1;

	return 1;
}

void
mkclock_hb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	bus_space_tag_t bst = ha->ha_bust;
	bus_space_handle_t bsh;
	todr_chip_handle_t handle;

	if (bus_space_map(bst, (bus_addr_t)ha->ha_address, ha->ha_size,
	    0, &bsh) != 0)
		printf("can't map device space\n");

	handle = mk48txx_attach(bst, bsh, "mk48t02", 1900, NULL, NULL);
	if (handle == NULL)
		panic("can't attach tod clock");

	printf("\n");

	handle->bus_cookie = NULL;
	handle->todr_setwen = NULL;

        todclock_config(handle);
}
