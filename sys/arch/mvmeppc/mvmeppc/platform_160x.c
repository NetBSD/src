/*	$NetBSD: platform_160x.c,v 1.2 2003/07/15 02:43:52 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
__KERNEL_RCSID(0, "$NetBSD: platform_160x.c,v 1.2 2003/07/15 02:43:52 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/bootinfo.h>
#include <machine/pio.h>
#include <machine/platform.h>

static int	p160x_match(struct platform *);
static void	p160x_pci_intr_fixup(int, int, int *);
static void	p160x_cpu_setup(struct device *);
static void	p160x_reset(void);

struct platform	platform_160x = {
	NULL,
	p160x_match,
	p160x_pci_intr_fixup,
	ext_intr,
	p160x_cpu_setup,
	p160x_reset
};

struct ppcbug_brdid {
	char	version[4];
	char	serial[12];
	char	id[16];
	char	pwa[16];
	char	reserved_0[4];
	char	ethernet_adr[6];
	char	reserved_1[2];
	char	lscsiid[2];
	char	speed_mpu[3];
	char	speed_bus[3];
	char	reserved[187];
	char	cksum[1];
};
#define	NVRAM_BRDID_OFF		0x1ef8

static void	p160x_get_brdid(struct ppcbug_brdid *);

static char	p160x_model[64];

static u_int32_t p160x_dram_size[] = {
	0x08000000, 0x02000000, 0x00800000, 0x00000000,
	0x08000000, 0x02000000, 0x00800000, 0x00000000
};

static int
p160x_match(struct platform *p)
{
	struct ppcbug_brdid bid;
	char speed[4], *cp;
	u_int8_t dsr;

	if (MVMEPPC_FAMILY(bootinfo.bi_modelnumber) != MVMEPPC_FAMILY_160x)
		return(0);

	p160x_get_brdid(&bid);

	for (cp = &bid.id[sizeof(bid.id) - 1]; *cp == ' '; cp--)
		*cp = '\0';
	for (cp = &bid.serial[sizeof(bid.serial) - 1]; *cp == ' '; cp--)
		*cp = '\0';
	for (cp = &bid.pwa[sizeof(bid.pwa) - 1]; *cp == ' '; cp--)
		*cp = '\0';

	sprintf(p160x_model, "%s, Serial: %s, PWA: %s", bid.id,
	    bid.serial, bid.pwa);
	p->model = p160x_model;

	speed[3] = '\0';
	strncpy(speed, bid.speed_mpu, sizeof(bid.speed_mpu));
	bootinfo.bi_mpuspeed = strtoul(speed, NULL, 10) * 1000000;
	strncpy(speed, bid.speed_bus, sizeof(bid.speed_bus));
	bootinfo.bi_busspeed = strtoul(speed, NULL, 10) * 1000000;
	bootinfo.bi_clocktps = bootinfo.bi_busspeed / 4;

	/* Fetch the DRAM size register */
	dsr = inb(0x80000804);
	bootinfo.bi_memsize = p160x_dram_size[dsr & 0x7];
	bootinfo.bi_memsize += p160x_dram_size[(dsr >> 4) & 0x7];

	return(1);
}

static void
p160x_pci_intr_fixup(int bus, int dev, int *line)
{

	printf("p160x_pci_intr_fixup: bus=%d, dev=%d, line=%d\n",
	    bus, dev, *line);
}

static void
p160x_cpu_setup(struct device *dev)
{
}

static void
p160x_reset(void)
{
}

static void
p160x_get_brdid(struct ppcbug_brdid *bid)
{
	u_int8_t *pbid = (u_int8_t *)bid;
	int off;
	int i;

	for (i = 0, off = NVRAM_BRDID_OFF; i < sizeof(*bid); i++, off++) {
		outb(0x80000074, off & 0xff);
		outb(0x80000075, (off >> 8) & 0xff);
		pbid[i] = inb(0x80000077);
	}
}
