/*	$NetBSD: amr.c,v 1.31.8.1 2006/05/24 10:58:00 yamt Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 1999,2000 Michael Smith
 * Copyright (c) 2000 BSDi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from FreeBSD: amr_pci.c,v 1.5 2000/08/30 07:52:40 msmith Exp
 * from FreeBSD: amr.c,v 1.16 2000/08/30 07:52:40 msmith Exp
 */

/*
 * Driver for AMI RAID controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amr.c,v 1.31.8.1 2006/05/24 10:58:00 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/kthread.h>

#include <uvm/uvm_extern.h>

#include <machine/endian.h>
#include <machine/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/amrreg.h>
#include <dev/pci/amrvar.h>

#include "locators.h"

static void	amr_attach(struct device *, struct device *, void *);
static void	amr_ccb_dump(struct amr_softc *, struct amr_ccb *);
static void	*amr_enquire(struct amr_softc *, u_int8_t, u_int8_t, u_int8_t,
			     void *);
static int	amr_init(struct amr_softc *, const char *,
			 struct pci_attach_args *pa);
static int	amr_intr(void *);
static int	amr_match(struct device *, struct cfdata *, void *);
static int	amr_print(void *, const char *);
static void	amr_shutdown(void *);
static void	amr_teardown(struct amr_softc *);
static void	amr_thread(void *);
static void	amr_thread_create(void *);

static int	amr_quartz_get_work(struct amr_softc *,
				    struct amr_mailbox_resp *);
static int	amr_quartz_submit(struct amr_softc *, struct amr_ccb *);
static int	amr_std_get_work(struct amr_softc *, struct amr_mailbox_resp *);
static int	amr_std_submit(struct amr_softc *, struct amr_ccb *);

CFATTACH_DECL(amr, sizeof(struct amr_softc),
    amr_match, amr_attach, NULL, NULL);

#define AT_QUARTZ	0x01	/* `Quartz' chipset */
#define	AT_SIG		0x02	/* Check for signature */

struct amr_pci_type {
	u_short	apt_vendor;
	u_short	apt_product;
	u_short	apt_flags;
} static const amr_pci_type[] = {
	{ PCI_VENDOR_AMI,   PCI_PRODUCT_AMI_MEGARAID,  0 },
	{ PCI_VENDOR_AMI,   PCI_PRODUCT_AMI_MEGARAID2, 0 },
	{ PCI_VENDOR_AMI,   PCI_PRODUCT_AMI_MEGARAID3, AT_QUARTZ },
	{ PCI_VENDOR_SYMBIOS, PCI_PRODUCT_AMI_MEGARAID3, AT_QUARTZ },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_AMI_MEGARAID3, AT_QUARTZ | AT_SIG },
	{ PCI_VENDOR_INTEL,  PCI_PRODUCT_SYMBIOS_MEGARAID_320X, AT_QUARTZ },
	{ PCI_VENDOR_INTEL,  PCI_PRODUCT_SYMBIOS_MEGARAID_320E, AT_QUARTZ },
	{ PCI_VENDOR_SYMBIOS,  PCI_PRODUCT_SYMBIOS_MEGARAID_300X, AT_QUARTZ },
	{ PCI_VENDOR_DELL,  PCI_PRODUCT_DELL_PERC_4DI, AT_QUARTZ },
	{ PCI_VENDOR_DELL,  PCI_PRODUCT_DELL_PERC_4DI_2, AT_QUARTZ },
	{ PCI_VENDOR_DELL,  PCI_PRODUCT_DELL_PERC_4ESI, AT_QUARTZ },
	{ PCI_VENDOR_SYMBIOS,  PCI_PRODUCT_SYMBIOS_PERC_4SC, AT_QUARTZ },
	{ PCI_VENDOR_SYMBIOS,  PCI_PRODUCT_SYMBIOS_MEGARAID_320X, AT_QUARTZ },
	{ PCI_VENDOR_SYMBIOS,  PCI_PRODUCT_SYMBIOS_MEGARAID_320E, AT_QUARTZ },
	{ PCI_VENDOR_SYMBIOS,  PCI_PRODUCT_SYMBIOS_MEGARAID_300X, AT_QUARTZ },
};

struct amr_typestr {
	const char	*at_str;
	int		at_sig;
} static const amr_typestr[] = {
	{ "Series 431",			AMR_SIG_431 },
	{ "Series 438",			AMR_SIG_438 },
	{ "Series 466",			AMR_SIG_466 },
	{ "Series 467",			AMR_SIG_467 },
	{ "Series 490",			AMR_SIG_490 },
	{ "Series 762",			AMR_SIG_762 },
	{ "HP NetRAID (T5)",		AMR_SIG_T5 },
	{ "HP NetRAID (T7)",		AMR_SIG_T7 },
};

struct {
	const char	*ds_descr;
	int	ds_happy;
} static const amr_dstate[] = {
	{ "offline",	0 },
	{ "degraded",	1 },
	{ "optimal",	1 },
	{ "online",	1 },
	{ "failed",	0 },
	{ "rebuilding",	1 },
	{ "hotspare",	0 },
};

static void	*amr_sdh;

static int	amr_max_segs;
int		amr_max_xfer;

static inline u_int8_t
amr_inb(struct amr_softc *amr, int off)
{

	bus_space_barrier(amr->amr_iot, amr->amr_ioh, off, 1,
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ);
	return (bus_space_read_1(amr->amr_iot, amr->amr_ioh, off));
}

static inline u_int32_t
amr_inl(struct amr_softc *amr, int off)
{

	bus_space_barrier(amr->amr_iot, amr->amr_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(amr->amr_iot, amr->amr_ioh, off));
}

static inline void
amr_outb(struct amr_softc *amr, int off, u_int8_t val)
{

	bus_space_write_1(amr->amr_iot, amr->amr_ioh, off, val);
	bus_space_barrier(amr->amr_iot, amr->amr_ioh, off, 1,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
amr_outl(struct amr_softc *amr, int off, u_int32_t val)
{

	bus_space_write_4(amr->amr_iot, amr->amr_ioh, off, val);
	bus_space_barrier(amr->amr_iot, amr->amr_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

/*
 * Match a supported device.
 */
static int
amr_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa;
	pcireg_t s;
	int i;

	pa = (struct pci_attach_args *)aux;

	/*
	 * Don't match the device if it's operating in I2O mode.  In this
	 * case it should be handled by the `iop' driver.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_I2O)
		return (0);

	for (i = 0; i < sizeof(amr_pci_type) / sizeof(amr_pci_type[0]); i++)
		if (PCI_VENDOR(pa->pa_id) == amr_pci_type[i].apt_vendor &&
		    PCI_PRODUCT(pa->pa_id) == amr_pci_type[i].apt_product)
		    	break;

	if (i == sizeof(amr_pci_type) / sizeof(amr_pci_type[0]))
		return (0);

	if ((amr_pci_type[i].apt_flags & AT_SIG) == 0)
		return (1);

	s = pci_conf_read(pa->pa_pc, pa->pa_tag, AMR_QUARTZ_SIG_REG) & 0xffff;
	return (s == AMR_QUARTZ_SIG0 || s == AMR_QUARTZ_SIG1);
}

/*
 * Attach a supported device.
 */
static void
amr_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa;
	struct amr_attach_args amra;
	const struct amr_pci_type *apt;
	struct amr_softc *amr;
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	const char *intrstr;
	pcireg_t reg;
	int rseg, i, j, size, rv, memreg, ioreg;
        struct amr_ccb *ac;
	int locs[AMRCF_NLOCS];

	aprint_naive(": RAID controller\n");

	amr = (struct amr_softc *)self;
	pa = (struct pci_attach_args *)aux;
	pc = pa->pa_pc;

	for (i = 0; i < sizeof(amr_pci_type) / sizeof(amr_pci_type[0]); i++)
		if (PCI_VENDOR(pa->pa_id) == amr_pci_type[i].apt_vendor &&
		    PCI_PRODUCT(pa->pa_id) == amr_pci_type[i].apt_product)
			break;
	apt = amr_pci_type + i;

	memreg = ioreg = 0;
	for (i = 0x10; i <= 0x14; i += 4) {
		reg = pci_conf_read(pc, pa->pa_tag, i);
		switch (PCI_MAPREG_TYPE(reg)) {
		case PCI_MAPREG_TYPE_MEM:
			if (PCI_MAPREG_MEM_SIZE(reg) != 0)
				memreg = i;
			break;
		case PCI_MAPREG_TYPE_IO:
			if (PCI_MAPREG_IO_SIZE(reg) != 0)
				ioreg = i;
			break;

		}
	}

	if (memreg && pci_mapreg_map(pa, memreg, PCI_MAPREG_TYPE_MEM, 0,
	    &amr->amr_iot, &amr->amr_ioh, NULL, &amr->amr_ios) == 0)
		;
	else if (ioreg && pci_mapreg_map(pa, ioreg, PCI_MAPREG_TYPE_IO, 0,
	    &amr->amr_iot, &amr->amr_ioh, NULL, &amr->amr_ios) == 0)
		;
	else {
		aprint_error("can't map control registers\n");
		amr_teardown(amr);
		return;
	}

	amr->amr_flags |= AMRF_PCI_REGS;
	amr->amr_dmat = pa->pa_dmat;
	amr->amr_pc = pa->pa_pc;

	/* Enable the device. */
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    reg | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("can't map interrupt\n");
		amr_teardown(amr);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	amr->amr_ih = pci_intr_establish(pc, ih, IPL_BIO, amr_intr, amr);
	if (amr->amr_ih == NULL) {
		aprint_error("can't establish interrupt");
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		amr_teardown(amr);
		return;
	}
	amr->amr_flags |= AMRF_PCI_INTR;

	/*
	 * Allocate space for the mailbox and S/G lists.  Some controllers
	 * don't like S/G lists to be located below 0x2000, so we allocate
	 * enough slop to enable us to compensate.
	 *
	 * The standard mailbox structure needs to be aligned on a 16-byte
	 * boundary.  The 64-bit mailbox has one extra field, 4 bytes in
	 * size, which preceeds the standard mailbox.
	 */
	size = AMR_SGL_SIZE * AMR_MAX_CMDS + 0x2000;
	amr->amr_dmasize = size;

	if ((rv = bus_dmamem_alloc(amr->amr_dmat, size, PAGE_SIZE, 0,
	    &amr->amr_dmaseg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: unable to allocate buffer, rv = %d\n",
		    amr->amr_dv.dv_xname, rv);
		amr_teardown(amr);
		return;
	}
	amr->amr_flags |= AMRF_DMA_ALLOC;

	if ((rv = bus_dmamem_map(amr->amr_dmat, &amr->amr_dmaseg, rseg, size,
	    (caddr_t *)&amr->amr_mbox,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		aprint_error("%s: unable to map buffer, rv = %d\n",
		    amr->amr_dv.dv_xname, rv);
		amr_teardown(amr);
		return;
	}
	amr->amr_flags |= AMRF_DMA_MAP;

	if ((rv = bus_dmamap_create(amr->amr_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT, &amr->amr_dmamap)) != 0) {
		aprint_error("%s: unable to create buffer DMA map, rv = %d\n",
		    amr->amr_dv.dv_xname, rv);
		amr_teardown(amr);
		return;
	}
	amr->amr_flags |= AMRF_DMA_CREATE;

	if ((rv = bus_dmamap_load(amr->amr_dmat, amr->amr_dmamap,
	    amr->amr_mbox, size, NULL, BUS_DMA_NOWAIT)) != 0) {
		aprint_error("%s: unable to load buffer DMA map, rv = %d\n",
		    amr->amr_dv.dv_xname, rv);
		amr_teardown(amr);
		return;
	}
	amr->amr_flags |= AMRF_DMA_LOAD;

	memset(amr->amr_mbox, 0, size);

	amr->amr_mbox_paddr = amr->amr_dmamap->dm_segs[0].ds_addr;
	amr->amr_sgls_paddr = (amr->amr_mbox_paddr + 0x1fff) & ~0x1fff;
	amr->amr_sgls = (struct amr_sgentry *)((caddr_t)amr->amr_mbox +
	    amr->amr_sgls_paddr - amr->amr_dmamap->dm_segs[0].ds_addr);

	/*
	 * Allocate and initalise the command control blocks.
	 */
	ac = malloc(sizeof(*ac) * AMR_MAX_CMDS, M_DEVBUF, M_NOWAIT | M_ZERO);
	amr->amr_ccbs = ac;
	SLIST_INIT(&amr->amr_ccb_freelist);
	TAILQ_INIT(&amr->amr_ccb_active);
	amr->amr_flags |= AMRF_CCBS;

	if (amr_max_xfer == 0) {
		amr_max_xfer = min(((AMR_MAX_SEGS - 1) * PAGE_SIZE), MAXPHYS);
		amr_max_segs = (amr_max_xfer + (PAGE_SIZE * 2) - 1) / PAGE_SIZE;
	}

	for (i = 0; i < AMR_MAX_CMDS; i++, ac++) {
		rv = bus_dmamap_create(amr->amr_dmat, amr_max_xfer,
		    amr_max_segs, amr_max_xfer, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &ac->ac_xfer_map);
		if (rv != 0)
			break;

		ac->ac_ident = i;
		amr_ccb_free(amr, ac);
	}
	if (i != AMR_MAX_CMDS) {
		aprint_error("%s: memory exhausted\n", amr->amr_dv.dv_xname);
		amr_teardown(amr);
		return;
	}

	/*
	 * Take care of model-specific tasks.
	 */
	if ((apt->apt_flags & AT_QUARTZ) != 0) {
		amr->amr_submit = amr_quartz_submit;
		amr->amr_get_work = amr_quartz_get_work;
	} else {
		amr->amr_submit = amr_std_submit;
		amr->amr_get_work = amr_std_get_work;

		/* Notify the controller of the mailbox location. */
		amr_outl(amr, AMR_SREG_MBOX, (u_int32_t)amr->amr_mbox_paddr + 16);
		amr_outb(amr, AMR_SREG_MBOX_ENABLE, AMR_SMBOX_ENABLE_ADDR);

		/* Clear outstanding interrupts and enable interrupts. */
		amr_outb(amr, AMR_SREG_CMD, AMR_SCMD_ACKINTR);
		amr_outb(amr, AMR_SREG_TOGL,
		    amr_inb(amr, AMR_SREG_TOGL) | AMR_STOGL_ENABLE);
	}

	/*
	 * Retrieve parameters, and tell the world about us.
	 */
	amr->amr_enqbuf = malloc(AMR_ENQUIRY_BUFSIZE, M_DEVBUF, M_NOWAIT);
	amr->amr_flags |= AMRF_ENQBUF;
	amr->amr_maxqueuecnt = i;
	aprint_normal(": AMI RAID ");
	if (amr_init(amr, intrstr, pa) != 0) {
		amr_teardown(amr);
		return;
	}

	/*
	 * Cap the maximum number of outstanding commands.  AMI's Linux
	 * driver doesn't trust the controller's reported value, and lockups
	 * have been seen when we do.
	 */
	amr->amr_maxqueuecnt = min(amr->amr_maxqueuecnt, AMR_MAX_CMDS);
	if (amr->amr_maxqueuecnt > i)
		amr->amr_maxqueuecnt = i;

	/* Set our `shutdownhook' before we start any device activity. */
	if (amr_sdh == NULL)
		amr_sdh = shutdownhook_establish(amr_shutdown, NULL);

	/* Attach sub-devices. */
	for (j = 0; j < amr->amr_numdrives; j++) {
		if (amr->amr_drive[j].al_size == 0)
			continue;
		amra.amra_unit = j;

		locs[AMRCF_UNIT] = j;

		amr->amr_drive[j].al_dv = config_found_sm_loc(&amr->amr_dv,
			"amr", locs, &amra, amr_print, config_stdsubmatch);
	}

	SIMPLEQ_INIT(&amr->amr_ccb_queue);

	/* XXX This doesn't work for newer boards yet. */
	if ((apt->apt_flags & AT_QUARTZ) == 0)
		kthread_create(amr_thread_create, amr);
}

/*
 * Free up resources.
 */
static void
amr_teardown(struct amr_softc *amr)
{
	struct amr_ccb *ac;
	int fl;

	fl = amr->amr_flags;

	if ((fl & AMRF_THREAD) != 0) {
		amr->amr_flags |= AMRF_THREAD_EXIT;
		wakeup(amr_thread);
		while ((amr->amr_flags & AMRF_THREAD_EXIT) != 0)
			tsleep(&amr->amr_flags, PWAIT, "amrexit", 0);
	}
	if ((fl & AMRF_CCBS) != 0) {
		SLIST_FOREACH(ac, &amr->amr_ccb_freelist, ac_chain.slist) {
			bus_dmamap_destroy(amr->amr_dmat, ac->ac_xfer_map);
		}
		free(amr->amr_ccbs, M_DEVBUF);
	}
	if ((fl & AMRF_ENQBUF) != 0)
		free(amr->amr_enqbuf, M_DEVBUF);
	if ((fl & AMRF_DMA_LOAD) != 0)
		bus_dmamap_unload(amr->amr_dmat, amr->amr_dmamap);
	if ((fl & AMRF_DMA_MAP) != 0)
		bus_dmamem_unmap(amr->amr_dmat, (caddr_t)amr->amr_mbox,
		    amr->amr_dmasize);
	if ((fl & AMRF_DMA_ALLOC) != 0)
		bus_dmamem_free(amr->amr_dmat, &amr->amr_dmaseg, 1);
	if ((fl & AMRF_DMA_CREATE) != 0)
		bus_dmamap_destroy(amr->amr_dmat, amr->amr_dmamap);
	if ((fl & AMRF_PCI_INTR) != 0)
		pci_intr_disestablish(amr->amr_pc, amr->amr_ih);
	if ((fl & AMRF_PCI_REGS) != 0)
		bus_space_unmap(amr->amr_iot, amr->amr_ioh, amr->amr_ios);
}

/*
 * Print autoconfiguration message for a sub-device.
 */
static int
amr_print(void *aux, const char *pnp)
{
	struct amr_attach_args *amra;

	amra = (struct amr_attach_args *)aux;

	if (pnp != NULL)
		aprint_normal("block device at %s", pnp);
	aprint_normal(" unit %d", amra->amra_unit);
	return (UNCONF);
}

/*
 * Retrieve operational parameters and describe the controller.
 */
static int
amr_init(struct amr_softc *amr, const char *intrstr,
	 struct pci_attach_args *pa)
{
	struct amr_adapter_info *aa;
	struct amr_prodinfo *ap;
	struct amr_enquiry *ae;
	struct amr_enquiry3 *aex;
	const char *prodstr;
	u_int i, sig, ishp;
	char sbuf[64];

	/*
	 * Try to get 40LD product info, which tells us what the card is
	 * labelled as.
	 */
	ap = amr_enquire(amr, AMR_CMD_CONFIG, AMR_CONFIG_PRODUCT_INFO, 0,
	    amr->amr_enqbuf);
	if (ap != NULL) {
		aprint_normal("<%.80s>\n", ap->ap_product);
		if (intrstr != NULL)
			aprint_normal("%s: interrupting at %s\n",
			    amr->amr_dv.dv_xname, intrstr);
		aprint_normal("%s: firmware %.16s, BIOS %.16s, %dMB RAM\n",
		    amr->amr_dv.dv_xname, ap->ap_firmware, ap->ap_bios,
		    le16toh(ap->ap_memsize));

		amr->amr_maxqueuecnt = ap->ap_maxio;

		/*
		 * Fetch and record state of logical drives.
		 */
		aex = amr_enquire(amr, AMR_CMD_CONFIG, AMR_CONFIG_ENQ3,
		    AMR_CONFIG_ENQ3_SOLICITED_FULL, amr->amr_enqbuf);
		if (aex == NULL) {
			aprint_error("%s ENQUIRY3 failed\n",
			    amr->amr_dv.dv_xname);
			return (-1);
		}

		if (aex->ae_numldrives > __arraycount(aex->ae_drivestate)) {
			aprint_error("%s: Inquiry returned more drives (%d)"
			   " than the array can handle (%zu)\n",
			   amr->amr_dv.dv_xname, aex->ae_numldrives,
			   __arraycount(aex->ae_drivestate));
			aex->ae_numldrives = __arraycount(aex->ae_drivestate);
		}
		if (aex->ae_numldrives > AMR_MAX_UNITS) {
			aprint_error(
			    "%s: adjust AMR_MAX_UNITS to %d (currently %d)"
			    "\n", amr->amr_dv.dv_xname, AMR_MAX_UNITS,
			    amr->amr_numdrives);
			amr->amr_numdrives = AMR_MAX_UNITS;
		} else
			amr->amr_numdrives = aex->ae_numldrives;

		for (i = 0; i < amr->amr_numdrives; i++) {
			amr->amr_drive[i].al_size =
			    le32toh(aex->ae_drivesize[i]);
			amr->amr_drive[i].al_state = aex->ae_drivestate[i];
			amr->amr_drive[i].al_properties = aex->ae_driveprop[i];
		}

		return (0);
	}

	/*
	 * Try 8LD extended ENQUIRY to get the controller signature.  Once
	 * found, search for a product description.
	 */
	ae = amr_enquire(amr, AMR_CMD_EXT_ENQUIRY2, 0, 0, amr->amr_enqbuf);
	if (ae != NULL) {
		i = 0;
		sig = le32toh(ae->ae_signature);

		while (i < sizeof(amr_typestr) / sizeof(amr_typestr[0])) {
			if (amr_typestr[i].at_sig == sig)
				break;
			i++;
		}
		if (i == sizeof(amr_typestr) / sizeof(amr_typestr[0])) {
			snprintf(sbuf, sizeof(sbuf),
			    "unknown ENQUIRY2 sig (0x%08x)", sig);
			prodstr = sbuf;
		} else
			prodstr = amr_typestr[i].at_str;
	} else {
		ae = amr_enquire(amr, AMR_CMD_ENQUIRY, 0, 0, amr->amr_enqbuf);
		if (ae == NULL) {
			aprint_error("%s: unsupported controller\n",
			    amr->amr_dv.dv_xname);
			return (-1);
		}

		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_AMI_MEGARAID:
			prodstr = "Series 428";
			break;
		case PCI_PRODUCT_AMI_MEGARAID2:
			prodstr = "Series 434";
			break;
		default:
			snprintf(sbuf, sizeof(sbuf), "unknown PCI dev (0x%04x)",
			    PCI_PRODUCT(pa->pa_id));
			prodstr = sbuf;
			break;
		}
	}

	/*
	 * HP NetRaid controllers have a special encoding of the firmware
	 * and BIOS versions.  The AMI version seems to have it as strings
	 * whereas the HP version does it with a leading uppercase character
	 * and two binary numbers.
	*/
	aa = &ae->ae_adapter;

	if (aa->aa_firmware[2] >= 'A' && aa->aa_firmware[2] <= 'Z' &&
	    aa->aa_firmware[1] <  ' ' && aa->aa_firmware[0] <  ' ' &&
	    aa->aa_bios[2] >= 'A' && aa->aa_bios[2] <= 'Z' &&
	    aa->aa_bios[1] <  ' ' && aa->aa_bios[0] <  ' ') {
		if (le32toh(ae->ae_signature) == AMR_SIG_438) {
			/* The AMI 438 is a NetRaid 3si in HP-land. */
			prodstr = "HP NetRaid 3si";
		}
		ishp = 1;
	} else
		ishp = 0;

	aprint_normal("<%s>\n", prodstr);
	if (intrstr != NULL)
		aprint_normal("%s: interrupting at %s\n", amr->amr_dv.dv_xname,
		    intrstr);

	if (ishp)
		aprint_normal("%s: firmware <%c.%02d.%02d>, BIOS <%c.%02d.%02d>"
		    ", %dMB RAM\n", amr->amr_dv.dv_xname, aa->aa_firmware[2],
		     aa->aa_firmware[1], aa->aa_firmware[0], aa->aa_bios[2],
		     aa->aa_bios[1], aa->aa_bios[0], aa->aa_memorysize);
	else
		aprint_normal("%s: firmware <%.4s>, BIOS <%.4s>, %dMB RAM\n",
		    amr->amr_dv.dv_xname, aa->aa_firmware, aa->aa_bios,
		    aa->aa_memorysize);

	amr->amr_maxqueuecnt = aa->aa_maxio;

	/*
	 * Record state of logical drives.
	 */
	if (ae->ae_ldrv.al_numdrives > __arraycount(ae->ae_ldrv.al_size)) {
		aprint_error("%s: Inquiry returned more drives (%d)"
		   " than the array can handle (%zu)\n",
		   amr->amr_dv.dv_xname, ae->ae_ldrv.al_numdrives,
		   __arraycount(ae->ae_ldrv.al_size));
		ae->ae_ldrv.al_numdrives = __arraycount(ae->ae_ldrv.al_size);
	}
	if (ae->ae_ldrv.al_numdrives > AMR_MAX_UNITS) {
		aprint_error("%s: adjust AMR_MAX_UNITS to %d (currently %d)\n",
		    amr->amr_dv.dv_xname, ae->ae_ldrv.al_numdrives,
		    AMR_MAX_UNITS);
		amr->amr_numdrives = AMR_MAX_UNITS;
	} else
		amr->amr_numdrives = ae->ae_ldrv.al_numdrives;

	for (i = 0; i < amr->amr_numdrives; i++) {
		amr->amr_drive[i].al_size = le32toh(ae->ae_ldrv.al_size[i]);
		amr->amr_drive[i].al_state = ae->ae_ldrv.al_state[i];
		amr->amr_drive[i].al_properties = ae->ae_ldrv.al_properties[i];
	}

	return (0);
}

/*
 * Flush the internal cache on each configured controller.  Called at
 * shutdown time.
 */
static void
amr_shutdown(void *cookie)
{
        extern struct cfdriver amr_cd;
	struct amr_softc *amr;
	struct amr_ccb *ac;
	int i, rv, s;

	for (i = 0; i < amr_cd.cd_ndevs; i++) {
		if ((amr = device_lookup(&amr_cd, i)) == NULL)
			continue;

		if ((rv = amr_ccb_alloc(amr, &ac)) == 0) {
			ac->ac_cmd.mb_command = AMR_CMD_FLUSH;
			s = splbio();
			rv = amr_ccb_poll(amr, ac, 30000);
			splx(s);
			amr_ccb_free(amr, ac);
		}
		if (rv != 0)
			printf("%s: unable to flush cache (%d)\n",
			    amr->amr_dv.dv_xname, rv);
	}
}

/*
 * Interrupt service routine.
 */
static int
amr_intr(void *cookie)
{
	struct amr_softc *amr;
	struct amr_ccb *ac;
	struct amr_mailbox_resp mbox;
	u_int i, forus, idx;

	amr = cookie;
	forus = 0;

	while ((*amr->amr_get_work)(amr, &mbox) == 0) {
		/* Iterate over completed commands in this result. */
		for (i = 0; i < mbox.mb_nstatus; i++) {
			idx = mbox.mb_completed[i] - 1;
			ac = amr->amr_ccbs + idx;

			if (idx >= amr->amr_maxqueuecnt) {
				printf("%s: bad status (bogus ID: %u=%u)\n",
				    amr->amr_dv.dv_xname, i, idx);
				continue;
			}

			if ((ac->ac_flags & AC_ACTIVE) == 0) {
				printf("%s: bad status (not active; 0x04%x)\n",
				    amr->amr_dv.dv_xname, ac->ac_flags);
				continue;
			}

			ac->ac_status = mbox.mb_status;
			ac->ac_flags = (ac->ac_flags & ~AC_ACTIVE) |
			    AC_COMPLETE;
			TAILQ_REMOVE(&amr->amr_ccb_active, ac, ac_chain.tailq);

			if ((ac->ac_flags & AC_MOAN) != 0)
				printf("%s: ccb %d completed\n",
				    amr->amr_dv.dv_xname, ac->ac_ident);

			/* Pass notification to upper layers. */
			if (ac->ac_handler != NULL)
				(*ac->ac_handler)(ac);
			else
				wakeup(ac);
		}
		forus = 1;
	}

	if (forus)
		amr_ccb_enqueue(amr, NULL);

	return (forus);
}

/*
 * Create the watchdog thread.
 */
static void
amr_thread_create(void *cookie)
{
	struct amr_softc *amr;
	int rv;

	amr = cookie;

	if ((amr->amr_flags & AMRF_THREAD_EXIT) != 0) {
		amr->amr_flags ^= AMRF_THREAD_EXIT;
		wakeup(&amr->amr_flags);
		return;
	}

	rv = kthread_create1(amr_thread, amr, &amr->amr_thread, "%s",
	    amr->amr_dv.dv_xname);
 	if (rv != 0)
		aprint_error("%s: unable to create thread (%d)",
 		    amr->amr_dv.dv_xname, rv);
 	else
 		amr->amr_flags |= AMRF_THREAD;
}

/*
 * Watchdog thread.
 */
static void
amr_thread(void *cookie)
{
	struct amr_softc *amr;
	struct amr_ccb *ac;
	struct amr_logdrive *al;
	struct amr_enquiry *ae;
	time_t curtime;
	int rv, i, s;

	amr = cookie;
	ae = amr->amr_enqbuf;

	for (;;) {
		tsleep(amr_thread, PWAIT, "amrwdog", AMR_WDOG_TICKS);

		if ((amr->amr_flags & AMRF_THREAD_EXIT) != 0) {
			amr->amr_flags ^= AMRF_THREAD_EXIT;
			wakeup(&amr->amr_flags);
			kthread_exit(0);
		}

		s = splbio();
		amr_intr(cookie);
		curtime = (time_t)mono_time.tv_sec;
		ac = TAILQ_FIRST(&amr->amr_ccb_active);
		while (ac != NULL) {
			if (ac->ac_start_time + AMR_TIMEOUT > curtime)
				break;
			if ((ac->ac_flags & AC_MOAN) == 0) {
				printf("%s: ccb %d timed out; mailbox:\n",
				    amr->amr_dv.dv_xname, ac->ac_ident);
				amr_ccb_dump(amr, ac);
				ac->ac_flags |= AC_MOAN;
			}
			ac = TAILQ_NEXT(ac, ac_chain.tailq);
		}
		splx(s);

		if ((rv = amr_ccb_alloc(amr, &ac)) != 0) {
			printf("%s: ccb_alloc failed (%d)\n",
 			    amr->amr_dv.dv_xname, rv);
			continue;
		}

		ac->ac_cmd.mb_command = AMR_CMD_ENQUIRY;

		rv = amr_ccb_map(amr, ac, amr->amr_enqbuf,
		    AMR_ENQUIRY_BUFSIZE, 0);
		if (rv != 0) {
			printf("%s: ccb_map failed (%d)\n",
 			    amr->amr_dv.dv_xname, rv);
			amr_ccb_free(amr, ac);
			continue;
		}

		rv = amr_ccb_wait(amr, ac);
		amr_ccb_unmap(amr, ac);
		if (rv != 0) {
			printf("%s: enquiry failed (st=%d)\n",
 			    amr->amr_dv.dv_xname, ac->ac_status);
			continue;
		}
		amr_ccb_free(amr, ac);

		al = amr->amr_drive;
		for (i = 0; i < __arraycount(ae->ae_ldrv.al_state); i++, al++) {
			if (al->al_dv == NULL)
				continue;
			if (al->al_state == ae->ae_ldrv.al_state[i])
				continue;

			printf("%s: state changed: %s -> %s\n",
			    al->al_dv->dv_xname,
			    amr_drive_state(al->al_state, NULL),
			    amr_drive_state(ae->ae_ldrv.al_state[i], NULL));

			al->al_state = ae->ae_ldrv.al_state[i];
		}
	}
}

/*
 * Return a text description of a logical drive's current state.
 */
const char *
amr_drive_state(int state, int *happy)
{
	const char *str;

	state = AMR_DRV_CURSTATE(state);
	if (state >= sizeof(amr_dstate) / sizeof(amr_dstate[0])) {
		if (happy)
			*happy = 1;
		str = "status unknown";
	} else {
		if (happy)
			*happy = amr_dstate[state].ds_happy;
		str = amr_dstate[state].ds_descr;
	}

	return (str);
}

/*
 * Run a generic enquiry-style command.
 */
static void *
amr_enquire(struct amr_softc *amr, u_int8_t cmd, u_int8_t cmdsub,
	    u_int8_t cmdqual, void *sbuf)
{
	struct amr_ccb *ac;
	u_int8_t *mb;
	int rv;

	if (amr_ccb_alloc(amr, &ac) != 0)
		return (NULL);

	/* Build the command proper. */
	mb = (u_int8_t *)&ac->ac_cmd;
	mb[0] = cmd;
	mb[2] = cmdsub;
	mb[3] = cmdqual;

	rv = amr_ccb_map(amr, ac, sbuf, AMR_ENQUIRY_BUFSIZE, 0);
	if (rv == 0) {
		rv = amr_ccb_poll(amr, ac, 2000);
		amr_ccb_unmap(amr, ac);
	}
	amr_ccb_free(amr, ac);

	return (rv ? NULL : sbuf);
}

/*
 * Allocate and initialise a CCB.
 */
int
amr_ccb_alloc(struct amr_softc *amr, struct amr_ccb **acp)
{
	int s;

	s = splbio();
	if ((*acp = SLIST_FIRST(&amr->amr_ccb_freelist)) == NULL) {
		splx(s);
		return (EAGAIN);
	}
	SLIST_REMOVE_HEAD(&amr->amr_ccb_freelist, ac_chain.slist);
	splx(s);

	return (0);
}

/*
 * Free a CCB.
 */
void
amr_ccb_free(struct amr_softc *amr, struct amr_ccb *ac)
{
	int s;

	memset(&ac->ac_cmd, 0, sizeof(ac->ac_cmd));
	ac->ac_cmd.mb_ident = ac->ac_ident + 1;
	ac->ac_cmd.mb_busy = 1;
	ac->ac_handler = NULL;
	ac->ac_flags = 0;

	s = splbio();
	SLIST_INSERT_HEAD(&amr->amr_ccb_freelist, ac, ac_chain.slist);
	splx(s);
}

/*
 * If a CCB is specified, enqueue it.  Pull CCBs off the software queue in
 * the order that they were enqueued and try to submit their command blocks
 * to the controller for execution.
 */
void
amr_ccb_enqueue(struct amr_softc *amr, struct amr_ccb *ac)
{
	int s;

	s = splbio();

	if (ac != NULL)
		SIMPLEQ_INSERT_TAIL(&amr->amr_ccb_queue, ac, ac_chain.simpleq);

	while ((ac = SIMPLEQ_FIRST(&amr->amr_ccb_queue)) != NULL) {
		if ((*amr->amr_submit)(amr, ac) != 0)
			break;
		SIMPLEQ_REMOVE_HEAD(&amr->amr_ccb_queue, ac_chain.simpleq);
		TAILQ_INSERT_TAIL(&amr->amr_ccb_active, ac, ac_chain.tailq);
	}

	splx(s);
}

/*
 * Map the specified CCB's data buffer onto the bus, and fill the
 * scatter-gather list.
 */
int
amr_ccb_map(struct amr_softc *amr, struct amr_ccb *ac, void *data, int size,
	    int out)
{
	struct amr_sgentry *sge;
	struct amr_mailbox_cmd *mb;
	int nsegs, i, rv, sgloff;
	bus_dmamap_t xfer;

	xfer = ac->ac_xfer_map;

	rv = bus_dmamap_load(amr->amr_dmat, xfer, data, size, NULL,
	    BUS_DMA_NOWAIT);
	if (rv != 0)
		return (rv);

	mb = &ac->ac_cmd;
	ac->ac_xfer_size = size;
	ac->ac_flags |= (out ? AC_XFER_OUT : AC_XFER_IN);
	sgloff = AMR_SGL_SIZE * ac->ac_ident;

	/* We don't need to use a scatter/gather list for just 1 segment. */
	nsegs = xfer->dm_nsegs;
	if (nsegs == 1) {
		mb->mb_nsgelem = 0;
		mb->mb_physaddr = htole32(xfer->dm_segs[0].ds_addr);
		ac->ac_flags |= AC_NOSGL;
	} else {
		mb->mb_nsgelem = nsegs;
		mb->mb_physaddr = htole32(amr->amr_sgls_paddr + sgloff);

		sge = (struct amr_sgentry *)((caddr_t)amr->amr_sgls + sgloff);
		for (i = 0; i < nsegs; i++, sge++) {
			sge->sge_addr = htole32(xfer->dm_segs[i].ds_addr);
			sge->sge_count = htole32(xfer->dm_segs[i].ds_len);
		}
	}

	bus_dmamap_sync(amr->amr_dmat, xfer, 0, ac->ac_xfer_size,
	    out ? BUS_DMASYNC_PREWRITE : BUS_DMASYNC_PREREAD);

	if ((ac->ac_flags & AC_NOSGL) == 0)
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, sgloff,
		    AMR_SGL_SIZE, BUS_DMASYNC_PREWRITE);

	return (0);
}

/*
 * Unmap the specified CCB's data buffer.
 */
void
amr_ccb_unmap(struct amr_softc *amr, struct amr_ccb *ac)
{

	if ((ac->ac_flags & AC_NOSGL) == 0)
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap,
		    AMR_SGL_SIZE * ac->ac_ident, AMR_SGL_SIZE,
		    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(amr->amr_dmat, ac->ac_xfer_map, 0, ac->ac_xfer_size,
	    (ac->ac_flags & AC_XFER_IN) != 0 ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(amr->amr_dmat, ac->ac_xfer_map);
}

/*
 * Submit a command to the controller and poll on completion.  Return
 * non-zero on timeout or error.  Must be called with interrupts blocked.
 */
int
amr_ccb_poll(struct amr_softc *amr, struct amr_ccb *ac, int timo)
{
	int rv;

	if ((rv = (*amr->amr_submit)(amr, ac)) != 0)
		return (rv);
	TAILQ_INSERT_TAIL(&amr->amr_ccb_active, ac, ac_chain.tailq);

	for (timo *= 10; timo != 0; timo--) {
		amr_intr(amr);
		if ((ac->ac_flags & AC_COMPLETE) != 0)
			break;
		DELAY(100);
	}

	return (timo == 0 || ac->ac_status != 0 ? EIO : 0);
}

/*
 * Submit a command to the controller and sleep on completion.  Return
 * non-zero on error.
 */
int
amr_ccb_wait(struct amr_softc *amr, struct amr_ccb *ac)
{
	int s;

	s = splbio();
	amr_ccb_enqueue(amr, ac);
	tsleep(ac, PRIBIO, "amrcmd", 0);
	splx(s);

	return (ac->ac_status != 0 ? EIO : 0);
}

#if 0
/*
 * Wait for the mailbox to become available.
 */
static int
amr_mbox_wait(struct amr_softc *amr)
{
	int timo;

	for (timo = 10000; timo != 0; timo--) {
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
		    sizeof(struct amr_mailbox), BUS_DMASYNC_POSTREAD);
		if (amr->amr_mbox->mb_cmd.mb_busy == 0)
			break;
		DELAY(100);
	}

	if (timo == 0)
		printf("%s: controller wedged\n", amr->amr_dv.dv_xname);

	return (timo != 0 ? 0 : EAGAIN);
}
#endif

/*
 * Tell the controller that the mailbox contains a valid command.  Must be
 * called with interrupts blocked.
 */
static int
amr_quartz_submit(struct amr_softc *amr, struct amr_ccb *ac)
{
	u_int32_t v;

	amr->amr_mbox->mb_poll = 0;
	amr->amr_mbox->mb_ack = 0;
	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_POSTREAD);
	if (amr->amr_mbox->mb_cmd.mb_busy != 0)
		return (EAGAIN);

	v = amr_inl(amr, AMR_QREG_IDB);
	if ((v & AMR_QIDB_SUBMIT) != 0) {
		amr->amr_mbox->mb_cmd.mb_busy = 0;
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
		    sizeof(struct amr_mailbox), BUS_DMASYNC_PREWRITE);
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
		    sizeof(struct amr_mailbox), BUS_DMASYNC_PREREAD);
		return (EAGAIN);
	}

	amr->amr_mbox->mb_segment = 0;
	memcpy(&amr->amr_mbox->mb_cmd, &ac->ac_cmd, sizeof(ac->ac_cmd));
	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_PREWRITE);

	ac->ac_start_time = (time_t)mono_time.tv_sec;
	ac->ac_flags |= AC_ACTIVE;
	amr_outl(amr, AMR_QREG_IDB,
	    (amr->amr_mbox_paddr + 16) | AMR_QIDB_SUBMIT);
	return (0);
}

static int
amr_std_submit(struct amr_softc *amr, struct amr_ccb *ac)
{

	amr->amr_mbox->mb_poll = 0;
	amr->amr_mbox->mb_ack = 0;
	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_POSTREAD);
	if (amr->amr_mbox->mb_cmd.mb_busy != 0)
		return (EAGAIN);

	if ((amr_inb(amr, AMR_SREG_MBOX_BUSY) & AMR_SMBOX_BUSY_FLAG) != 0) {
		amr->amr_mbox->mb_cmd.mb_busy = 0;
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
		    sizeof(struct amr_mailbox), BUS_DMASYNC_PREWRITE);
		bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
		    sizeof(struct amr_mailbox), BUS_DMASYNC_PREREAD);
		return (EAGAIN);
	}

	amr->amr_mbox->mb_segment = 0;
	memcpy(&amr->amr_mbox->mb_cmd, &ac->ac_cmd, sizeof(ac->ac_cmd));
	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_PREWRITE);

	ac->ac_start_time = (time_t)mono_time.tv_sec;
	ac->ac_flags |= AC_ACTIVE;
	amr_outb(amr, AMR_SREG_CMD, AMR_SCMD_POST);
	return (0);
}

/*
 * Claim any work that the controller has completed; acknowledge completion,
 * save details of the completion in (mbsave).  Must be called with
 * interrupts blocked.
 */
static int
amr_quartz_get_work(struct amr_softc *amr, struct amr_mailbox_resp *mbsave)
{

	/* Work waiting for us? */
	if (amr_inl(amr, AMR_QREG_ODB) != AMR_QODB_READY)
		return (-1);

	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_POSTREAD);

	/* Save the mailbox, which contains a list of completed commands. */
	memcpy(mbsave, &amr->amr_mbox->mb_resp, sizeof(*mbsave));

	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_PREREAD);

	/* Ack the interrupt and mailbox transfer. */
	amr_outl(amr, AMR_QREG_ODB, AMR_QODB_READY);
	amr_outl(amr, AMR_QREG_IDB, (amr->amr_mbox_paddr+16) | AMR_QIDB_ACK);

	/*
	 * This waits for the controller to notice that we've taken the
	 * command from it.  It's very inefficient, and we shouldn't do it,
	 * but if we remove this code, we stop completing commands under
	 * load.
	 *
	 * Peter J says we shouldn't do this.  The documentation says we
	 * should.  Who is right?
	 */
	while ((amr_inl(amr, AMR_QREG_IDB) & AMR_QIDB_ACK) != 0)
		DELAY(10);

	return (0);
}

static int
amr_std_get_work(struct amr_softc *amr, struct amr_mailbox_resp *mbsave)
{
	u_int8_t istat;

	/* Check for valid interrupt status. */
	if (((istat = amr_inb(amr, AMR_SREG_INTR)) & AMR_SINTR_VALID) == 0)
		return (-1);

	/* Ack the interrupt. */
	amr_outb(amr, AMR_SREG_INTR, istat);

	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_POSTREAD);

	/* Save mailbox, which contains a list of completed commands. */
	memcpy(mbsave, &amr->amr_mbox->mb_resp, sizeof(*mbsave));

	bus_dmamap_sync(amr->amr_dmat, amr->amr_dmamap, 0,
	    sizeof(struct amr_mailbox), BUS_DMASYNC_PREREAD);

	/* Ack mailbox transfer. */
	amr_outb(amr, AMR_SREG_CMD, AMR_SCMD_ACKINTR);

	return (0);
}

static void
amr_ccb_dump(struct amr_softc *amr, struct amr_ccb *ac)
{
	int i;

	printf("%s: ", amr->amr_dv.dv_xname);
	for (i = 0; i < 4; i++)
		printf("%08x ", ((u_int32_t *)&ac->ac_cmd)[i]);
	printf("\n");
}
