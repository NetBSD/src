/*	$NetBSD: asc.c,v 1.16 1997/02/03 17:36:00 scottr Exp $	*/

/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ASC driver code and asc_ringbell() support
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include "ascvar.h"
#include "obiovar.h"

#define	MAC68K_ASC_BASE	((caddr_t) 0x50f14000)
#define	MAC68K_ASC_LEN	0x01000

/* bell support data */
static int		asc_configured = 0;
static bus_space_tag_t	asc_tag = MAC68K_BUS_SPACE_MEM;
static bus_space_handle_t asc_handle;

static int bell_freq = 1880;
static int bell_length = 10;
static int bell_volume = 100;
static int bell_ringing = 0;

static int  ascmatch __P((struct device *, struct cfdata *, void *));
static void ascattach __P((struct device *, struct device *, void *));

struct cfattach asc_ca = {
	sizeof(struct device), ascmatch, ascattach
};

struct cfdriver asc_cd = {
	NULL, "asc", DV_DULL, NULL, 0
};

static int
ascmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	static int asc_matched = 0;
	struct obio_attach_args *oa = aux;
	bus_space_tag_t bst = MAC68K_BUS_SPACE_MEM;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	int rval = 0;

	/* Allow only one instance. */
	if (asc_matched)
		return (0);
	asc_matched = 1;

	addr = (bus_addr_t) (oa->oa_addr ? oa->oa_addr : MAC68K_ASC_BASE);

	if (bus_space_map(bst, addr, MAC68K_ASC_LEN, 0, &bsh))
		return (0);

	if (bus_probe(bst, bsh, 0, 1))
		rval = 1;
	else
		rval = 0;

	bus_space_unmap(bst, bsh, MAC68K_ASC_LEN);

	return rval;
}

static void
ascattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct obio_attach_args *oa = aux;
	bus_addr_t addr;

	addr = (bus_addr_t) (oa->oa_addr ? oa->oa_addr : MAC68K_ASC_BASE);
	if (bus_space_map(asc_tag, addr, MAC68K_ASC_LEN, 0,
	    &asc_handle)) {
		printf("%s: can't map memory space\n", self->dv_xname);
		return;
	}

	printf(" Apple sound chip\n");
	asc_configured = 1;
}

int 
asc_setbellparams(freq, length, volume)
	int freq;
	int length;
	int volume;
{
	if (!asc_configured)
		return (ENODEV);

	/*
	 * I only perform these checks for sanity.  I suppose
	 * someone might want a bell that rings all day, but then
	 * they can make kernel mods themselves.
	 */

	if (freq < 10 || freq > 40000)
		return (EINVAL);
	if (length < 0 || length > 3600)
		return (EINVAL);
	if (volume < 0 || volume > 100)
		return (EINVAL);

	bell_freq = freq;
	bell_length = length;
	bell_volume = volume;

	return (0);
}


int 
asc_getbellparams(freq, length, volume)
    int *freq;
    int *length;
    int *volume;
{
	if (!asc_configured)
		return (ENODEV);

	*freq = bell_freq;
	*length = bell_length;
	*volume = bell_volume;

	return (0);
}


void 
asc_bellstop(param)
    int param;
{
	if (!asc_configured)
		return;

	if (bell_ringing > 1000 || bell_ringing < 0)
		panic("bell got out of sync?");

	if (--bell_ringing == 0)	/* disable ASC */
		bus_space_write_1(asc_tag, asc_handle, 0x801, 0);
}


int 
asc_ringbell()
{
	int     i;
	unsigned long freq;

	if (!asc_configured)
		return (ENODEV);

	if (bell_ringing == 0) {

		for (i = 0; i < 0x800; i++)
			bus_space_write_1(asc_tag, asc_handle, i, 0);

		for (i = 0; i < 256; i++) {
			bus_space_write_1(asc_tag, asc_handle, i, i / 4);
			bus_space_write_1(asc_tag, asc_handle, i + 512, i / 4);
			bus_space_write_1(asc_tag, asc_handle, i + 1024, i / 4);
			bus_space_write_1(asc_tag, asc_handle, i + 1536, i / 4);
		}		/* up part of wave, four voices ? */
		for (i = 0; i < 256; i++) {
			bus_space_write_1(asc_tag, asc_handle, i + 256,
			    0x3f - (i / 4));
			bus_space_write_1(asc_tag, asc_handle, i + 768,
			    0x3f - (i / 4));
			bus_space_write_1(asc_tag, asc_handle, i + 1280,
			    0x3f - (i / 4));
			bus_space_write_1(asc_tag, asc_handle, i + 1792,
			    0x3f - (i / 4));
		}		/* down part of wave, four voices ? */

		/* Fix this.  Need to find exact ASC sampling freq */
		freq = 65536 * bell_freq / 466;

		/* printf("beep: from %d, %02x %02x %02x %02x\n",
		 * cur_beep.freq, (freq >> 24) & 0xff, (freq >> 16) & 0xff,
		 * (freq >> 8) & 0xff, (freq) & 0xff); */
		for (i = 0; i < 8; i++) {
			bus_space_write_1(asc_tag, asc_handle, 0x814 + 8 * i,
			    (freq >> 24) & 0xff);
			bus_space_write_1(asc_tag, asc_handle, 0x815 + 8 * i,
			    (freq >> 16) & 0xff);
			bus_space_write_1(asc_tag, asc_handle, 0x816 + 8 * i,
			    (freq >> 8) & 0xff);
			bus_space_write_1(asc_tag, asc_handle, 0x817 + 8 * i,
			    (freq) & 0xff);
		}		/* frequency; should put cur_beep.freq in here
				 * somewhere. */

		bus_space_write_1(asc_tag, asc_handle, 0x807, 3); /* 44 ? */
		bus_space_write_1(asc_tag, asc_handle, 0x806,
		    255 * bell_volume / 100);
		bus_space_write_1(asc_tag, asc_handle, 0x805, 0);
		bus_space_write_1(asc_tag, asc_handle, 0x80f, 0);
		bus_space_write_1(asc_tag, asc_handle, 0x802, 2); /* sampled */
		bus_space_write_1(asc_tag, asc_handle, 0x801, 2); /* enable sampled */
	}
	bell_ringing++;
	timeout((void *) asc_bellstop, 0, bell_length);

	return (0);
}
