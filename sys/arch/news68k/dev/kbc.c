/*	$NetBSD: kbc.c,v 1.7 2003/07/15 02:59:26 lukem Exp $	*/

/*-
 * Copyright (C) 2001 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kbc.c,v 1.7 2003/07/15 02:59:26 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/cons.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/kbcvar.h>

#define KBC_SIZE 0x10 /* XXX */

/* Definition of the driver for autoconfig. */
static int  kbc_match(struct device *, struct cfdata *, void *);
static void kbc_attach(struct device *, struct device *, void *);
static int  kbc_print(void *, const char *name);

CFATTACH_DECL(kbc, sizeof(struct device),
    kbc_match, kbc_attach, NULL, NULL);

extern struct cfdriver kbc_cd;

static int kbc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	u_int addr;

	if (strcmp(ha->ha_name, "kbc"))
		return 0;

	/* XXX no default address */
	if (ha->ha_address == (u_int)-1)
		return 0;

	addr = IIOV(ha->ha_address); /* XXX */

	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

static void
kbc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	struct kbc_attach_args ka;
	bus_space_tag_t bt = ha->ha_bust;
	bus_space_handle_t bh;

	if (bus_space_map(bt, ha->ha_address, KBC_SIZE, 0, &bh) != 0) {
		printf("can't map device space\n");
		return;
	}

	printf("\n");

	ka.ka_bt = bt;
	ka.ka_bh = bh;
	ka.ka_ipl = ha->ha_ipl;

	if (ka.ka_ipl == -1)
		ka.ka_ipl = KBC_PRI;

	ka.ka_name = "kb";
	config_found(self, (void *)&ka, kbc_print);

	ka.ka_name = "ms";
	config_found(self, (void *)&ka, kbc_print);
}

static int
kbc_print(aux, name)
	void *aux;
	const char *name;
{

	if (name != NULL)
		aprint_normal("%s: ", name);

	return UNCONF;
}
