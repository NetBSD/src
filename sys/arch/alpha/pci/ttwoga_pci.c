/* $NetBSD: ttwoga_pci.c,v 1.1.4.1 2002/06/20 03:37:45 nathanw Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ttwoga_pci.c,v 1.1.4.1 2002/06/20 03:37:45 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h> 
#include <sys/lock.h>

#include <dev/pci/pcireg.h> 
#include <dev/pci/pcivar.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>

void		ttwoga_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
int		ttwoga_bus_maxdevs(void *, int);
pcitag_t	ttwoga_make_tag(void *, int, int, int);
void		ttwoga_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	ttwoga_conf_read(void *, pcitag_t, int);
void		ttwoga_conf_write(void *, pcitag_t, int, pcireg_t);

paddr_t		ttwoga_make_type0addr(int, int);

/*
 * The T2 has an annoying bug that can manifest itself while
 * probing for devices.  If a non-existent CBUS address is
 * accessed with a read, "a few" subsequent write cycles
 * will generate spurious memory errors.  We work around this
 * by tracking which CPU has the PCI configuration mutex and
 * "expect" machine checks on that CPU for the duraction of
 * the PCI configuration access routine.
 */

struct simplelock ttwoga_conf_slock;
cpuid_t ttwoga_conf_cpu;		/* XXX core logic bug */

#define	TTWOGA_CONF_LOCK(s)						\
do {									\
	(s) = splhigh();						\
	simple_lock(&ttwoga_conf_slock);				\
	ttwoga_conf_cpu = cpu_number();					\
} while (0)

#define	TTWOGA_CONF_UNLOCK(s)						\
do {									\
	ttwoga_conf_cpu = (cpuid_t)-1;					\
	simple_unlock(&ttwoga_conf_slock);				\
	splx((s));							\
} while (0)

void
ttwoga_pci_init(pci_chipset_tag_t pc, void *v)
{

	simple_lock_init(&ttwoga_conf_slock);

	pc->pc_conf_v = v;
	pc->pc_attach_hook = ttwoga_attach_hook;
	pc->pc_bus_maxdevs = ttwoga_bus_maxdevs;
	pc->pc_make_tag = ttwoga_make_tag;
	pc->pc_decompose_tag = ttwoga_decompose_tag;
	pc->pc_conf_read = ttwoga_conf_read;
	pc->pc_conf_write = ttwoga_conf_write;
}

void
ttwoga_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

int
ttwoga_bus_maxdevs(void *cpv, int busno)
{

	return 32;
}

pcitag_t
ttwoga_make_tag(void *cpv, int b, int d, int f)
{

	/* This is the format used for Type 1 configuration cycles. */
	return (b << 16) | (d << 11) | (f << 8);
}

void
ttwoga_decompose_tag(void *cpv, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

paddr_t
ttwoga_make_type0addr(int d, int f)
{

	if (d > 8)			/* XXX ??? */
		return ((paddr_t) -1);
	return ((0x0800UL << d) | (f << 8));
}

pcireg_t
ttwoga_conf_read(void *cpv, pcitag_t tag, int offset)
{
	struct ttwoga_config *tcp = cpv;
	pcireg_t *datap, data;
	int s, b, d, f, ba;
	paddr_t addr;
	u_int64_t old_hae3;

	pci_decompose_tag(&tcp->tc_pc, tag, &b, &d, &f);

	addr = b ? tag : ttwoga_make_type0addr(d, f);
	if (addr == (paddr_t)-1)
		return ((pcireg_t) -1);

	TTWOGA_CONF_LOCK(s);

	alpha_mb();
	old_hae3 = T2GA(tcp, T2_HAE0_3) & ~HAE0_3_PCA;
	T2GA(tcp, T2_HAE0_3) =
	    old_hae3 | ((b ? 1UL : 0UL) << HAE0_3_PCA_SHIFT);
	alpha_mb();
	alpha_mb();

	datap =
	    (pcireg_t *)ALPHA_PHYS_TO_K0SEG(tcp->tc_sysmap->tsmap_conf_base |
	    addr << 5UL |		/* address shift */
	    (offset & ~0x03) << 5UL |	/* address shift */
	    0x3 << 3UL);		/* 4-byte, size shift */
	data = (pcireg_t)-1;
	if (!(ba = badaddr(datap, sizeof *datap)))
		data = *datap;

	alpha_mb();
	T2GA(tcp, T2_HAE0_3) = old_hae3;
	alpha_mb();
	alpha_mb();

	alpha_pal_draina();
	alpha_mb();
	alpha_mb();

	TTWOGA_CONF_UNLOCK(s);

#if 0
	printf("ttwoga_conf_read: tag 0x%lx, reg 0x%x -> 0x%x @ %p%s\n",
	    tag, offset, data, datap, ba ? " (badaddr)" : "");
#endif

	return (data);
}

void
ttwoga_conf_write(void *cpv, pcitag_t tag, int offset, pcireg_t data)
{
	struct ttwoga_config *tcp = cpv;
	pcireg_t *datap;
	int s, b, d, f;
	paddr_t addr;
	u_int64_t old_hae3;

	pci_decompose_tag(&tcp->tc_pc, tag, &b, &d, &f);

	addr = b ? tag : ttwoga_make_type0addr(d, f);
	if (addr == (paddr_t)-1)
		return;

	TTWOGA_CONF_LOCK(s);

	alpha_mb();
	old_hae3 = T2GA(tcp, T2_HAE0_3) & ~HAE0_3_PCA;
	T2GA(tcp, T2_HAE0_3) =
	    old_hae3 | ((b ? 1UL : 0UL) << HAE0_3_PCA_SHIFT);
	alpha_mb();
	alpha_mb();

	datap =
	    (pcireg_t *)ALPHA_PHYS_TO_K0SEG(tcp->tc_sysmap->tsmap_conf_base |
	    addr << 5UL |		/* address shift */
	    (offset & ~0x03) << 5UL |	/* address shift */
	    0x3 << 3UL);		/* 4-byte, size shift */

	alpha_mb();
	*datap = data;
	alpha_mb();
	alpha_mb();

	alpha_mb();
	T2GA(tcp, T2_HAE0_3) = old_hae3;
	alpha_mb();
	alpha_mb();

	TTWOGA_CONF_UNLOCK(s);

#if 0
	printf("ttwoga_conf_write: tag 0x%lx, reg 0x%x -> 0x%x @ %p\n",
	    tag, offset, data, datap);
#endif
}
