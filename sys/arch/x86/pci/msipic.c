/*	$NetBSD: msipic.c,v 1.11.2.2 2017/12/03 11:36:50 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan Inc.
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
__KERNEL_RCSID(0, "$NetBSD: msipic.c,v 1.11.2.2 2017/12/03 11:36:50 jdolecek Exp $");

#include "opt_intrdebug.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <dev/pci/pcivar.h>

#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/pic.h>
#include <machine/lock.h>

#include <x86/pci/msipic.h>

#ifdef INTRDEBUG
#define MSIPICDEBUG
#endif

#ifdef MSIPICDEBUG
#define DPRINTF(msg) printf msg
#else
#define DPRINTF(msg)
#endif

#define BUS_SPACE_WRITE_FLUSH(pc, tag) (void)bus_space_read_4(pc, tag, 0)

#define MSIPICNAMEBUF 16

/*
 * A Pseudo pic for single MSI/MSI-X device.
 * The pic and MSI/MSI-X device are distinbuished by "devid". The "devid"
 * is managed by below "dev_seqs".
 */
struct msipic {
	int mp_bus;
	int mp_dev;
	int mp_fun;

	int mp_devid; /* The device id for the MSI/MSI-X device. */
	int mp_veccnt; /* The number of MSI/MSI-X vectors. */

	char mp_pic_name[MSIPICNAMEBUF]; /* The MSI/MSI-X device's name. */

	struct pci_attach_args mp_pa;
	bus_space_tag_t mp_bstag;
	bus_space_handle_t mp_bshandle;
	bus_size_t mp_bssize;
	struct pic *mp_pic;

	LIST_ENTRY(msipic) mp_list;
};

static kmutex_t msipic_list_lock;

static LIST_HEAD(, msipic) msipic_list =
	LIST_HEAD_INITIALIZER(msipic_list);

/*
 * This struct managements "devid" to use the same "devid" for the device
 * re-attached. If the device's bus number and device numer and function
 * number are equal, it is assumed re-attached.
 */
struct dev_last_used_seq {
	bool ds_using;
	int ds_bus;
	int ds_dev;
	int ds_fun;
};
/* The number of MSI/MSI-X devices supported by system. */
#define NUM_MSI_DEVS 256
/* Record devids to use the same devid when the device is re-attached. */
static struct dev_last_used_seq dev_seqs[NUM_MSI_DEVS];

static int msipic_allocate_common_msi_devid(const struct pci_attach_args *);
static void msipic_release_common_msi_devid(int);

static struct pic *msipic_find_msi_pic_locked(int);
static struct pic *msipic_construct_common_msi_pic(const struct pci_attach_args *,
						   struct pic *);
static void msipic_destruct_common_msi_pic(struct pic *);

static void msi_set_msictl_enablebit(struct pic *, int, int);
static void msi_hwmask(struct pic *, int);
static void msi_hwunmask(struct pic *, int);
static void msi_addroute(struct pic *, struct cpu_info *, int, int, int);
static void msi_delroute(struct pic *, struct cpu_info *, int, int, int);

static void msix_set_vecctl_mask(struct pic *, int, int);
static void msix_hwmask(struct pic *, int);
static void msix_hwunmask(struct pic *, int);
static void msix_addroute(struct pic *, struct cpu_info *, int, int, int);
static void msix_delroute(struct pic *, struct cpu_info *, int, int, int);

/*
 * Return new "devid" for the device attached first.
 * Return the same "devid" for the device re-attached after dettached once.
 * Return -1 if the number of attached MSI/MSI-X devices is over NUM_MSI_DEVS.
 */
static int
msipic_allocate_common_msi_devid(const struct pci_attach_args *pa)
{
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int bus, dev, fun, i;

	KASSERT(mutex_owned(&msipic_list_lock));

	pc = pa->pa_pc;
	tag = pa->pa_tag;
	pci_decompose_tag(pc, tag, &bus, &dev, &fun);

	/* if the device was once attached, use same devid */
	for (i = 0; i < NUM_MSI_DEVS; i++) {
		/* skip host bridge */
		if (dev_seqs[i].ds_bus == 0
		    && dev_seqs[i].ds_dev == 0
		    && dev_seqs[i].ds_fun == 0)
			break;

		if (dev_seqs[i].ds_bus == bus
		    && dev_seqs[i].ds_dev == dev
		    && dev_seqs[i].ds_fun == fun) {
			dev_seqs[i].ds_using = true;
			return i;
		}
	}

	for (i = 0; i < NUM_MSI_DEVS; i++) {
		if (dev_seqs[i].ds_using == 0) {
			dev_seqs[i].ds_using = true;
			dev_seqs[i].ds_bus = bus;
			dev_seqs[i].ds_dev = dev;
			dev_seqs[i].ds_fun = fun;
			return i;
		}
	}

	DPRINTF(("too many MSI devices.\n"));
	return -1;
}

/*
 * Set the "devid" unused, but keep reserving the "devid" to reuse when
 * the device is re-attached.
 */
static void
msipic_release_common_msi_devid(int devid)
{

	KASSERT(mutex_owned(&msipic_list_lock));

	if (devid < 0 || NUM_MSI_DEVS <= devid) {
		DPRINTF(("%s: invalid devid.\n", __func__));
		return;
	}

	dev_seqs[devid].ds_using = false;
	/* Keep ds_* to reuse the same devid for the same device. */
}

static struct pic *
msipic_find_msi_pic_locked(int devid)
{
	struct msipic *mpp;

	KASSERT(mutex_owned(&msipic_list_lock));

	LIST_FOREACH(mpp, &msipic_list, mp_list) {
		if(mpp->mp_devid == devid)
			return mpp->mp_pic;
	}
	return NULL;
}

/*
 * Return the msi_pic whose device is already registered.
 * If the device is not registered yet, return NULL.
 */
struct pic *
msipic_find_msi_pic(int devid)
{
	struct pic *msipic;

	mutex_enter(&msipic_list_lock);
	msipic = msipic_find_msi_pic_locked(devid);
	mutex_exit(&msipic_list_lock);

	return msipic;
}

/*
 * A common construct process of MSI and MSI-X.
 */
static struct pic *
msipic_construct_common_msi_pic(const struct pci_attach_args *pa,
    struct pic *pic_tmpl)
{
	struct pic *pic;
	struct msipic *msipic;
	int devid;

	pic = kmem_alloc(sizeof(*pic), KM_SLEEP);
	msipic = kmem_zalloc(sizeof(*msipic), KM_SLEEP);

	mutex_enter(&msipic_list_lock);

	devid = msipic_allocate_common_msi_devid(pa);
	if (devid == -1) {
		mutex_exit(&msipic_list_lock);
		kmem_free(pic, sizeof(*pic));
		kmem_free(msipic, sizeof(*msipic));
		return NULL;
	}

	memcpy(pic, pic_tmpl, sizeof(*pic));
	pic->pic_edge_stubs = x2apic_mode ? x2apic_edge_stubs : ioapic_edge_stubs,
	pic->pic_msipic = msipic;
	msipic->mp_pic = pic;
	pci_decompose_tag(pa->pa_pc, pa->pa_tag,
	    &msipic->mp_bus, &msipic->mp_dev, &msipic->mp_fun);
	memcpy(&msipic->mp_pa, pa, sizeof(msipic->mp_pa));
	msipic->mp_devid = devid;
	/*
	 * pci_msi{,x}_alloc() must be called only once in the device driver.
	 */
	KASSERT(msipic_find_msi_pic_locked(msipic->mp_devid) == NULL);

	LIST_INSERT_HEAD(&msipic_list, msipic, mp_list);

	mutex_exit(&msipic_list_lock);

	return pic;
}

static void
msipic_destruct_common_msi_pic(struct pic *msi_pic)
{
	struct msipic *msipic;

	if (msi_pic == NULL)
		return;

	msipic = msi_pic->pic_msipic;
	mutex_enter(&msipic_list_lock);
	LIST_REMOVE(msipic, mp_list);
	msipic_release_common_msi_devid(msipic->mp_devid);
	mutex_exit(&msipic_list_lock);

	kmem_free(msipic, sizeof(*msipic));
	kmem_free(msi_pic, sizeof(*msi_pic));
}

/*
 * The pic is MSI/MSI-X pic or not.
 */
bool
msipic_is_msi_pic(struct pic *pic)
{

	return (pic->pic_msipic != NULL);
}

/*
 * Return the MSI/MSI-X devid which is unique for each devices.
 */
int
msipic_get_devid(struct pic *pic)
{

	KASSERT(msipic_is_msi_pic(pic));

	return pic->pic_msipic->mp_devid;
}

#define MSI_MSICTL_ENABLE 1
#define MSI_MSICTL_DISABLE 0
static void
msi_set_msictl_enablebit(struct pic *pic, int msi_vec, int flag)
{
	pci_chipset_tag_t pc;
	struct pci_attach_args *pa;
	pcitag_t tag;
	pcireg_t ctl;
	int off, err __diagused;

	pc = NULL;
	pa = &pic->pic_msipic->mp_pa;
	tag = pa->pa_tag;
	err = pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL);
	KASSERT(err != 0);

	/*
	 * MSI can establish only one vector at once.
	 * So, use whole device mask bit instead of a vector mask bit.
	 */
	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	if (flag == MSI_MSICTL_ENABLE)
		ctl |= PCI_MSI_CTL_MSI_ENABLE;
	else
		ctl &= ~PCI_MSI_CTL_MSI_ENABLE;

	pci_conf_write(pc, tag, off, ctl);
}

static void
msi_hwmask(struct pic *pic, int msi_vec)
{

	msi_set_msictl_enablebit(pic, msi_vec, MSI_MSICTL_DISABLE);
}

/*
 * Do not use pic->hwunmask() immediately after pic->delroute().
 * It is required to use pic->addroute() before pic->hwunmask().
 */
static void
msi_hwunmask(struct pic *pic, int msi_vec)
{

	msi_set_msictl_enablebit(pic, msi_vec, MSI_MSICTL_ENABLE);
}

static void
msi_addroute(struct pic *pic, struct cpu_info *ci,
	     int unused, int idt_vec, int type)
{
	pci_chipset_tag_t pc;
	struct pci_attach_args *pa;
	pcitag_t tag;
	pcireg_t addr, data, ctl;
	int off, err __diagused;

	pc = NULL;
	pa = &pic->pic_msipic->mp_pa;
	tag = pa->pa_tag;
	err = pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL);
	KASSERT(err != 0);

	/*
	 * See Intel 64 and IA-32 Architectures Software Developer's Manual
	 * Volume 3 10.11 Message Signalled Interrupts.
	 */
	/*
	 * "cpuid" for MSI address is local APIC ID. In NetBSD, the ID is
	 * the same as ci->ci_cpuid.
	 */
	addr = LAPIC_MSIADDR_BASE | __SHIFTIN(ci->ci_cpuid,
	    LAPIC_MSIADDR_DSTID_MASK);
	/* If trigger mode is edge, it don't care level for trigger mode. */
	data = __SHIFTIN(idt_vec, LAPIC_MSIDATA_VECTOR_MASK)
		| LAPIC_MSIDATA_TRGMODE_EDGE | LAPIC_MSIDATA_DM_FIXED;

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	if (ctl & PCI_MSI_CTL_64BIT_ADDR) {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_LO, addr);
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_HI, 0);
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA64, data);
	} else {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR, addr);
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA, data);
	}
	ctl |= PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

/*
 * Do not use pic->hwunmask() immediately after pic->delroute().
 * It is required to use pic->addroute() before pic->hwunmask().
 */
static void
msi_delroute(struct pic *pic, struct cpu_info *ci,
	     int msi_vec, int idt_vec, int type)
{

	msi_hwmask(pic, msi_vec);
}

/*
 * Template for MSI pic.
 * .pic_msipic is set later in construct_msi_pic().
 */
static struct pic msi_pic_tmpl = {
	.pic_type = PIC_MSI,
	.pic_vecbase = 0,
	.pic_apicid = 0,
	.pic_lock = __SIMPLELOCK_UNLOCKED, /* not used for msi_pic */
	.pic_hwmask = msi_hwmask,
	.pic_hwunmask = msi_hwunmask,
	.pic_addroute = msi_addroute,
	.pic_delroute = msi_delroute,
};

/*
 * Create pseudo pic for a MSI device.
 */
struct pic *
msipic_construct_msi_pic(const struct pci_attach_args *pa)
{
	struct pic *msi_pic;
	char pic_name_buf[MSIPICNAMEBUF];

	msi_pic = msipic_construct_common_msi_pic(pa, &msi_pic_tmpl);
	if (msi_pic == NULL) {
		DPRINTF(("cannot allocate MSI pic.\n"));
		return NULL;
	}

	memset(pic_name_buf, 0, MSIPICNAMEBUF);
	snprintf(pic_name_buf, MSIPICNAMEBUF, "msi%d",
	    msi_pic->pic_msipic->mp_devid);
	strncpy(msi_pic->pic_msipic->mp_pic_name, pic_name_buf,
	    MSIPICNAMEBUF - 1);
	msi_pic->pic_name = msi_pic->pic_msipic->mp_pic_name;

	return msi_pic;
}

/*
 * Delete pseudo pic for a MSI device.
 */
void
msipic_destruct_msi_pic(struct pic *msi_pic)
{

	msipic_destruct_common_msi_pic(msi_pic);
}

#define MSIX_VECCTL_HWMASK 1
#define MSIX_VECCTL_HWUNMASK 0
static void
msix_set_vecctl_mask(struct pic *pic, int msix_vec, int flag)
{
	bus_space_tag_t bstag;
	bus_space_handle_t bshandle;
	uint64_t entry_base;
	uint32_t vecctl;

	if (msix_vec < 0) {
		DPRINTF(("%s: invalid MSI-X table index, devid=%d vecid=%d",
			__func__, msipic_get_devid(pic), msix_vec));
		return;
	}

	entry_base = PCI_MSIX_TABLE_ENTRY_SIZE * msix_vec;

	bstag = pic->pic_msipic->mp_bstag;
	bshandle = pic->pic_msipic->mp_bshandle;
	vecctl = bus_space_read_4(bstag, bshandle,
	    entry_base + PCI_MSIX_TABLE_ENTRY_VECTCTL);
	if (flag == MSIX_VECCTL_HWMASK)
		vecctl |= PCI_MSIX_VECTCTL_MASK;
	else
		vecctl &= ~PCI_MSIX_VECTCTL_MASK;

	bus_space_write_4(bstag, bshandle,
	    entry_base + PCI_MSIX_TABLE_ENTRY_VECTCTL, vecctl);
	BUS_SPACE_WRITE_FLUSH(bstag, bshandle);
}

static void
msix_hwmask(struct pic *pic, int msix_vec)
{

	msix_set_vecctl_mask(pic, msix_vec, MSIX_VECCTL_HWMASK);
}

/*
 * Do not use pic->hwunmask() immediately after pic->delroute().
 * It is required to use pic->addroute() before pic->hwunmask().
 */
static void
msix_hwunmask(struct pic *pic, int msix_vec)
{

	msix_set_vecctl_mask(pic, msix_vec, MSIX_VECCTL_HWUNMASK);
}

static void
msix_addroute(struct pic *pic, struct cpu_info *ci,
	     int msix_vec, int idt_vec, int type)
{
	pci_chipset_tag_t pc;
	struct pci_attach_args *pa;
	pcitag_t tag;
	bus_space_tag_t bstag;
	bus_space_handle_t bshandle;
	uint64_t entry_base;
	pcireg_t addr, data, ctl;
	int off, err __diagused;

	if (msix_vec < 0) {
		DPRINTF(("%s: invalid MSI-X table index, devid=%d vecid=%d",
			__func__, msipic_get_devid(pic), msix_vec));
		return;
	}

	pa = &pic->pic_msipic->mp_pa;
	pc = pa->pa_pc;
	tag = pa->pa_tag;
	err = pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL);
	KASSERT(err != 0);

	entry_base = PCI_MSIX_TABLE_ENTRY_SIZE * msix_vec;

	/*
	 * See Intel 64 and IA-32 Architectures Software Developer's Manual
	 * Volume 3 10.11 Message Signalled Interrupts.
	 */
	/*
	 * "cpuid" for MSI-X address is local APIC ID. In NetBSD, the ID is
	 * the same as ci->ci_cpuid.
	 */
	addr = LAPIC_MSIADDR_BASE | __SHIFTIN(ci->ci_cpuid,
	    LAPIC_MSIADDR_DSTID_MASK);
	/* If trigger mode is edge, it don't care level for trigger mode. */
	data = __SHIFTIN(idt_vec, LAPIC_MSIDATA_VECTOR_MASK)
		| LAPIC_MSIDATA_TRGMODE_EDGE | LAPIC_MSIDATA_DM_FIXED;

	bstag = pic->pic_msipic->mp_bstag;
	bshandle = pic->pic_msipic->mp_bshandle;
	bus_space_write_4(bstag, bshandle,
	    entry_base + PCI_MSIX_TABLE_ENTRY_ADDR_LO, addr);
	bus_space_write_4(bstag, bshandle,
	    entry_base + PCI_MSIX_TABLE_ENTRY_ADDR_HI, 0);
	bus_space_write_4(bstag, bshandle,
	    entry_base + PCI_MSIX_TABLE_ENTRY_DATA, data);
	bus_space_write_4(bstag, bshandle,
	    entry_base + PCI_MSIX_TABLE_ENTRY_VECTCTL, 0);
	BUS_SPACE_WRITE_FLUSH(bstag, bshandle);

	ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
	ctl |= PCI_MSIX_CTL_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);
}

/*
 * Do not use pic->hwunmask() immediately after pic->delroute().
 * It is required to use pic->addroute() before pic->hwunmask().
 */
static void
msix_delroute(struct pic *pic, struct cpu_info *ci,
	     int msix_vec, int vec, int type)
{

	msix_hwmask(pic, msix_vec);
}

/*
 * Template for MSI-X pic.
 * .pic_msipic is set later in construct_msix_pic().
 */
static struct pic msix_pic_tmpl = {
	.pic_type = PIC_MSIX,
	.pic_vecbase = 0,
	.pic_apicid = 0,
	.pic_lock = __SIMPLELOCK_UNLOCKED, /* not used for msix_pic */
	.pic_hwmask = msix_hwmask,
	.pic_hwunmask = msix_hwunmask,
	.pic_addroute = msix_addroute,
	.pic_delroute = msix_delroute,
};

struct pic *
msipic_construct_msix_pic(const struct pci_attach_args *pa)
{
	struct pic *msix_pic;
	pci_chipset_tag_t pc;
	pcitag_t tag;
	pcireg_t tbl;
	bus_space_tag_t bstag;
	bus_space_handle_t bshandle;
	bus_size_t bssize;
	size_t table_size;
	uint32_t table_offset;
	u_int memtype;
	bus_addr_t memaddr;
	int flags;
	int bir, bar, err, off, table_nentry;
	char pic_name_buf[MSIPICNAMEBUF];

	table_nentry = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (table_nentry == 0) {
		DPRINTF(("MSI-X table entry is 0.\n"));
		return NULL;
	}

	pc = pa->pa_pc;
	tag = pa->pa_tag;
	if (pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL) == 0) {
		DPRINTF(("%s: no msix capability", __func__));
		return NULL;
	}

	msix_pic = msipic_construct_common_msi_pic(pa, &msix_pic_tmpl);
	if (msix_pic == NULL) {
		DPRINTF(("cannot allocate MSI-X pic.\n"));
		return NULL;
	}

	memset(pic_name_buf, 0, MSIPICNAMEBUF);
	snprintf(pic_name_buf, MSIPICNAMEBUF, "msix%d",
	    msix_pic->pic_msipic->mp_devid);
	strncpy(msix_pic->pic_msipic->mp_pic_name, pic_name_buf,
	    MSIPICNAMEBUF - 1);
	msix_pic->pic_name = msix_pic->pic_msipic->mp_pic_name;

	tbl = pci_conf_read(pc, tag, off + PCI_MSIX_TBLOFFSET);
	table_offset = tbl & PCI_MSIX_TBLOFFSET_MASK;
	bir = tbl & PCI_MSIX_PBABIR_MASK;
	switch(bir) {
	case 0:
		bar = PCI_BAR0;
		break;
	case 1:
		bar = PCI_BAR1;
		break;
	case 2:
		bar = PCI_BAR2;
		break;
	case 3:
		bar = PCI_BAR3;
		break;
	case 4:
		bar = PCI_BAR4;
		break;
	case 5:
		bar = PCI_BAR5;
		break;
	default:
		aprint_error("detect an illegal device! The device use reserved BIR values.\n");
		msipic_destruct_common_msi_pic(msix_pic);
		return NULL;
	}
	memtype = pci_mapreg_type(pc, tag, bar);
	 /*
	  * PCI_MSIX_TABLE_ENTRY_SIZE consists below
	  *     - Vector Control (32bit)
	  *     - Message Data (32bit)
	  *     - Message Upper Address (32bit)
	  *     - Message Lower Address (32bit)
	  */
	table_size = table_nentry * PCI_MSIX_TABLE_ENTRY_SIZE;
#if 0
	err = pci_mapreg_submap(pa, bar, memtype, BUS_SPACE_MAP_LINEAR,
	    roundup(table_size, PAGE_SIZE), table_offset,
	    &bstag, &bshandle, NULL, &bssize);
#else
	/*
	 * Workaround for PCI prefetchable bit. Some chips (e.g. Intel 82599)
	 * report SERR and MSI-X doesn't work. This problem might not be the
	 * driver's bug but our PCI common part or VMs' bug. Until we find a
	 * real reason, we ignore the prefetchable bit.
	 */
	if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, bar, memtype,
		&memaddr, NULL, &flags) != 0) {
		DPRINTF(("cannot get a map info.\n"));
		msipic_destruct_common_msi_pic(msix_pic);
		return NULL;
	}
	if ((flags & BUS_SPACE_MAP_PREFETCHABLE) != 0) {
		DPRINTF(( "clear prefetchable bit\n"));
		flags &= ~BUS_SPACE_MAP_PREFETCHABLE;
	}
	bssize = roundup(table_size, PAGE_SIZE);
	err = bus_space_map(pa->pa_memt, memaddr + table_offset, bssize, flags,
	    &bshandle);
	bstag = pa->pa_memt;
#endif
	if (err) {
		DPRINTF(("cannot map msix table.\n"));
		msipic_destruct_common_msi_pic(msix_pic);
		return NULL;
	}
	msix_pic->pic_msipic->mp_bstag = bstag;
	msix_pic->pic_msipic->mp_bshandle = bshandle;
	msix_pic->pic_msipic->mp_bssize = bssize;

	return msix_pic;
}

/*
 * Delete pseudo pic for a MSI-X device.
 */
void
msipic_destruct_msix_pic(struct pic *msix_pic)
{
	struct msipic *msipic;

	KASSERT(msipic_is_msi_pic(msix_pic));
	KASSERT(msix_pic->pic_type == PIC_MSIX);

	msipic = msix_pic->pic_msipic;
	bus_space_unmap(msipic->mp_bstag, msipic->mp_bshandle,
	    msipic->mp_bssize);

	msipic_destruct_common_msi_pic(msix_pic);
}

/*
 * Set the number of MSI vectors for pseudo MSI pic.
 */
int
msipic_set_msi_vectors(struct pic *msi_pic, pci_intr_handle_t *pihs,
    int count)
{

	KASSERT(msipic_is_msi_pic(msi_pic));

	msi_pic->pic_msipic->mp_veccnt = count;
	return 0;
}

/*
 * Initialize the system to use MSI/MSI-X.
 */
void
msipic_init(void)
{

	mutex_init(&msipic_list_lock, MUTEX_DEFAULT, IPL_NONE);
}
