/*	$NetBSD: veritefb.c,v 1.2 2026/07/15 20:53:22 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * Rendition Verite V2100/V2200 driver.
 *
 * Influenced by xf86-video-rendition.
 *
 * The on-board RISC boots from the card ROM at PCI reset and parks in 
 * a loop. We reset it and hold before any other access to the card.
 * The console runs unaccelerated from autoconf on until 2D microcode is 
 * loaded. 
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: veritefb.c,v 1.2 2026/07/15 20:53:22 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#ifdef VERITEFB_DEBUG
#include <sys/callout.h>
#endif
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/conf.h>
#include <sys/extent.h>

#include <sys/exec_elf.h>

#include <dev/firmload.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

#include <dev/pci/veritefbreg.h>
#include <dev/pci/veritefb_ucode.h>
#include <dev/pci/veritefbio.h>
#include <dev/pci/verite3dio.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>
#include <dev/pci/wsdisplay_pci.h>

#include "opt_wsemul.h"
#include "opt_veritefb.h"
#include "opt_ddb.h"

#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <dev/i2c/ddcvar.h>

#include <ddb/db_active.h>
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#endif

#define VFB_MAXPOLL	100000	/* status/hold polls */
#define VFB_VSYNCPOLL	40000	/* vsync wait, ~2 frames in us */
#define VFB_SHORTPOLL	100	/* polls in the RISC debug port */
#define VFB_FIFOPOLL	100000	/* FIFO waits, us */
#define VFB_DRAINPOLL	10000	/* output FIFO drains */
#define VFB_LAZY_LIMIT	16	/* queued ops between forced syncs */

#define VFB_LAZY_AREA	307200	/* fill at the engine's pixel rate */

#define VFB_PROBE_PATTERN	0xf5faaf5fU
#define VFB_PROBE_START		0x12345678U
#define VFB_MAXVRAM		(16 * 1024 * 1024)
#define VFB_MAXUCODE		(1024 * 1024)	/* sanity cap for firmware */

#define VFB_PLL_REF		1431818	/* 14.31818 MHz in units of 10 Hz */

#define VFB_FIRMWARE_NAME	"v20002d.uc"

#define VFB_GC_GAP		5	/* scanlines between fb and glyph cache */
#define VFB_UNDERLINE_OFF	2	/* underline offset from cell bottom */
#define VFB_GETPIXEL_PATTERN	0x5a5a5a5aU /* handshake test pixels */
#define VFB_CMD_BOGUS		50	/* unassigned cmd for fault injection */

#define VFB_ACCEL_OFF	0	/* no microcode loaded */
#define VFB_ACCEL_SW	1	/* degraded after a fault; never retried */
#define VFB_ACCEL_ON	2	/* RISC running, handshake passed */

#define VFB_OWNER_CONSOLE	VERITEFB_OWNER_CONSOLE
#define VFB_OWNER_PARKED	VERITEFB_OWNER_PARKED
#define VFB_OWNER_FOREIGN	VERITEFB_OWNER_FOREIGN

struct veritefb_softc {
	device_t		sc_dev;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;

	bus_space_tag_t		sc_iot;		/* I/O registers (BAR1) */
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_io_paddr;
	bus_size_t		sc_ios;
	bus_space_tag_t		sc_mmiot;	/* MMIO registers (BAR2) */
	bus_space_handle_t	sc_mmioh;
	bus_addr_t		sc_mmio_paddr;
	bus_size_t		sc_mmios;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	bus_size_t		sc_regoff;
	bus_space_tag_t		sc_memt;	/* fb aperture (BAR0) */
	bus_space_handle_t	sc_memh;
	bus_addr_t		sc_fb_paddr;
	bus_size_t		sc_apsize;	/* mapped aperture size */
	bus_size_t		sc_memsize;	/* probed VRAM size */
	bus_size_t		sc_fb_offset;	/* fb start (microcode area) */

	/* RISC / acceleration state */
	int			sc_accel;	/* VFB_ACCEL_* */
	int			sc_risc_owner;	/* VFB_OWNER_* */
	int			sc_unsynced;	/* ops queued since last sync */
	uint32_t		sc_unsynced_area; /* pixels queued likewise */
	uint32_t		sc_ucode_entry;
	uint8_t			*sc_ucode;	/* firmware image copy */
	size_t			sc_ucode_size;

	glyphcache		sc_gc;		/* VRAM glyph cache */
	bool			sc_gc_initted;
	/* bus-master FIFO feed (descriptor list page + bounce buffer) */
#define VFB_DMA_BOUNCE		65536
#define VFB_DMA_TIMEOUT_US	200000
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dma_list_map;
	bus_dmamap_t		sc_dma_buf_map;
	bus_dma_segment_t	sc_dma_list_seg;
	bus_dma_segment_t	sc_dma_buf_seg;
	uint32_t		*sc_dma_list;	/* data entry + stop entry */
	uint8_t			*sc_dma_buf;
	/*
	 * Async submit
	 */
	bool			sc_dma_pending;
	bus_dmamap_t		sc_dma_pending_map; /* owed POSTWRITE */
	uint32_t		sc_dma_pending_len;

	/*
	 * Vblank-queued flip
	 */
	void			*sc_ih;
	bool			sc_flip_intr_ok;
	volatile bool		sc_flip_pending;
	volatile uint32_t	sc_flip_base;

	/* /dev/verite3d: exclusive 3D client state */
	bool			sc_v3d_open;
	bool			sc_v3d_dead;	/* EIO until close */
	bool			sc_v3d_inited;	/* client ucode running */
	struct extent		*sc_v3d_ext;	/* VRAM pool */
	uint32_t		sc_v3d_pool_base;
#define V3D_MAX_REGIONS	1024
	struct {
		uint32_t	addr;
		uint32_t	size;
	}			*sc_v3d_reg;	/* live allocations */
	int			sc_v3d_nreg;
	bus_dmamap_t		sc_v3d_slot_map[V3D_RING_SLOTS];
	bus_dma_segment_t	sc_v3d_slot_seg[V3D_RING_SLOTS];
	uint8_t			*sc_v3d_slot[V3D_RING_SLOTS];

#ifdef VERITEFB_DEBUG
	/* last words written to the input FIFO */
#define VFB_RING_SIZE	128		/* power of two */
	uint32_t		sc_ring[VFB_RING_SIZE];
	unsigned		sc_ring_count;
	struct veritefb_dbg_stats sc_stats;

	/* RISC PC sampling profiler (veritefb_pcsample_tick) */
	struct callout		sc_pcsample_ch;
	bool			sc_pcsample_on;
	uint32_t		sc_pcsample_hz;
	uint32_t		sc_pchist_base;
	uint32_t		sc_pchist_shift;
	uint32_t		sc_pchist_min;
	uint32_t		sc_pchist_max;
	uint64_t		sc_pchist_samples;
	uint64_t		sc_pchist_missed;
	uint32_t		sc_pchist[VFB_PCHIST_BUCKETS];
#endif

	/* software rendering ops, the permanent fallback */
	void (*sc_orig_eraserows)(void *, int, int, long);
	void (*sc_orig_erasecols)(void *, int, int, int, long);
	void (*sc_orig_copyrows)(void *, int, int, int);
	void (*sc_orig_copycols)(void *, int, int, int, int);
	void (*sc_orig_putchar)(void *, int, int, u_int, long);

	int			sc_width;
	int			sc_height;
	int			sc_depth;
	int			sc_linebytes;
	uint8_t			sc_stride0;	/* pixel engine stride */
	uint8_t			sc_stride1;
	const struct videomode	*sc_videomode;	/* the mode in use */

	/* DDC/EDID */
	struct i2c_controller	sc_i2c;
	uint32_t		sc_ddc_base;	/* CRTCCTL sans DDC bits */
	uint8_t			sc_edid[128];
	struct edid_info	sc_ei;
	bool			sc_edid_valid;

	int			sc_mode;	/* WSDISPLAYIO_MODE_* */
	struct vcons_data	vd;
	struct vcons_screen	sc_console_screen;
	struct wsscreen_descr	sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list	sc_screenlist;
	u_char			sc_cmap_red[256];
	u_char			sc_cmap_green[256];
	u_char			sc_cmap_blue[256];
};

static const struct veritefb_stride {
	uint16_t linebytes;
	uint8_t stride0, stride1;
} veritefb_stride_table[] = {
	{  640, 2, 4 },
	{  704, 6, 4 },
	{  768, 5, 0 },
	{  784, 5, 1 },
	{  800, 5, 2 },
	{  832, 5, 3 },
	{  896, 5, 4 },
	{ 1024, 3, 0 },
	{ 1040, 3, 1 },
	{ 1056, 3, 2 },
	{ 1088, 3, 3 },
	{ 1152, 3, 4 },
	{ 1168, 7, 1 },
	{ 1184, 7, 2 },
	{ 1216, 7, 3 },
	{ 1280, 1, 5 },
	{ 1536, 2, 5 },
	{ 1600, 6, 5 },
	{ 1792, 5, 5 },
	{ 2048, 0, 6 },
	{ 0, 0, 0 }
};

static int	veritefb_match(device_t, cfdata_t, void *);
static void	veritefb_attach(device_t, device_t, void *);

static void	veritefb_risc_softreset(struct veritefb_softc *);
static void	veritefb_risc_hold(struct veritefb_softc *);
static void	veritefb_risc_continue(struct veritefb_softc *);
static void	veritefb_risc_forcestep(struct veritefb_softc *, uint32_t);
static void	veritefb_risc_writerf(struct veritefb_softc *, uint8_t,
		    uint32_t);
static uint32_t	veritefb_risc_readrf(struct veritefb_softc *, uint8_t);
static uint32_t	veritefb_risc_readmem(struct veritefb_softc *, uint32_t);
static void	veritefb_risc_writemem(struct veritefb_softc *, uint32_t,
		    uint32_t);
static void	veritefb_risc_flushicache(struct veritefb_softc *);
static void	veritefb_risc_start(struct veritefb_softc *, uint32_t);

static uint32_t	veritefb_risc_samplepc(struct veritefb_softc *);
#ifdef VERITEFB_DEBUG
static void	veritefb_pcsample_tick(void *);
#endif
static void	veritefb_accel_fail(struct veritefb_softc *, const char *);
static int	veritefb_waitfifo(struct veritefb_softc *, int);
static int	veritefb_drain_outfifo(struct veritefb_softc *);
static int	veritefb_read_outfifo(struct veritefb_softc *, uint32_t *);
static void	veritefb_load_firmware(device_t);
static bool	veritefb_ucode_load(struct veritefb_softc *, const uint8_t *,
		    size_t, uint32_t, uint32_t, bool, uint32_t *);
static bool	veritefb_2d_handshake(struct veritefb_softc *);
static int	veritefb_waitfifo_raw(struct veritefb_softc *, int, uint32_t);
static int	veritefb_read_outfifo_raw(struct veritefb_softc *, uint32_t *,
		    uint32_t);
static int	veritefb_suspend2d(struct veritefb_softc *);
static int	veritefb_resume2d(struct veritefb_softc *);
static void	veritefb_mux_redraw(struct veritefb_softc *);
static int	veritefb_set_scan_depth(struct veritefb_softc *, int);
static int	veritefb_dma_alloc(struct veritefb_softc *);
static int	veritefb_dma_drain(struct veritefb_softc *);
static int	veritefb_intr(void *);
static void	veritefb_flip_cancel(struct veritefb_softc *);
static void	veritefb_flip_wait(struct veritefb_softc *);
static int	veritefb_dma_submit(struct veritefb_softc *, bus_dmamap_t,
		    uint32_t, uint32_t);

dev_type_open(verite3d_open);
dev_type_close(verite3d_close);
dev_type_ioctl(verite3d_ioctl);
dev_type_mmap(verite3d_mmap);
static void	verite3d_teardown(struct veritefb_softc *);
static int	verite3d_ring_alloc(struct veritefb_softc *);
static bool	veritefb_risc_init(struct veritefb_softc *);
static size_t	veritefb_mem_size(struct veritefb_softc *);

static bool	veritefb_calc_pclk(int, int *, int *, int *);
static const struct veritefb_stride *veritefb_stride_for(int);
static bool	veritefb_set_mode(struct veritefb_softc *,
		    const struct videomode *);

static void	veritefb_i2cbb_set_bits(void *, uint32_t);
static void	veritefb_i2cbb_set_dir(void *, uint32_t);
static uint32_t	veritefb_i2cbb_read_bits(void *);
static int	veritefb_i2c_send_start(void *, int);
static int	veritefb_i2c_send_stop(void *, int);
static int	veritefb_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int	veritefb_i2c_read_byte(void *, uint8_t *, int);
static int	veritefb_i2c_write_byte(void *, uint8_t, int);
static void	veritefb_ddc_read(struct veritefb_softc *);
static void	veritefb_pick_mode(struct veritefb_softc *);
static void	veritefb_init_dac(struct veritefb_softc *);
static void	veritefb_wait_vsync(struct veritefb_softc *);
static void	veritefb_set_dac_entry(struct veritefb_softc *, int, uint8_t,
		    uint8_t, uint8_t);
static void	veritefb_init_palette(struct veritefb_softc *);
static int	veritefb_getcmap(struct veritefb_softc *,
		    struct wsdisplay_cmap *);
static int	veritefb_putcmap(struct veritefb_softc *,
		    struct wsdisplay_cmap *);

static void	veritefb_init_screen(void *, struct vcons_screen *, int,
		    long *);
static paddr_t	veritefb_mmap(void *, void *, off_t, int);
static int	veritefb_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);

static void	veritefb_sync(struct veritefb_softc *);
static bool	veritefb_rectfill(struct veritefb_softc *, int, int, int,
		    int, uint32_t);
static bool	veritefb_bitblt(struct veritefb_softc *, int, int, int, int,
		    int, int);
static void	veritefb_eraserows(void *, int, int, long);
static void	veritefb_erasecols(void *, int, int, int, long);
static void	veritefb_copyrows(void *, int, int, int);
static void	veritefb_copycols(void *, int, int, int, int);
static void	veritefb_putchar(void *, int, int, u_int, long);
static void	veritefb_gc_bitblt(void *, int, int, int, int, int, int,
		    int);

#if defined(DDB) && defined(VERITEFB_DEBUG)
static void	veritefb_ddb_attach(struct veritefb_softc *);
#endif

extern struct cfdriver veritefb_cd;

CFATTACH_DECL_NEW(veritefb, sizeof(struct veritefb_softc),
    veritefb_match, veritefb_attach, NULL, NULL);

static struct wsdisplay_accessops veritefb_accessops = {
	.ioctl = veritefb_ioctl,
	.mmap = veritefb_mmap,
};

static inline uint8_t
vfb_read1(struct veritefb_softc *sc, bus_size_t reg)
{
	if (reg >= VFB_IOONLY_BASE)
		return bus_space_read_1(sc->sc_iot, sc->sc_ioh, reg);
	return bus_space_read_1(sc->sc_regt, sc->sc_regh,
	    sc->sc_regoff + reg);
}

static inline void
vfb_write1(struct veritefb_softc *sc, bus_size_t reg, uint8_t val)
{
	if (reg >= VFB_IOONLY_BASE) {
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, reg, val);
		return;
	}
	bus_space_write_1(sc->sc_regt, sc->sc_regh, sc->sc_regoff + reg,
	    val);
}

static inline uint32_t
vfb_read4(struct veritefb_softc *sc, bus_size_t reg)
{
	if (reg >= VFB_IOONLY_BASE)
		return bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg);
	return bus_space_read_4(sc->sc_regt, sc->sc_regh,
	    sc->sc_regoff + reg);
}

static inline void
vfb_write4(struct veritefb_softc *sc, bus_size_t reg, uint32_t val)
{
	if (reg >= VFB_IOONLY_BASE) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, val);
		return;
	}
	bus_space_write_4(sc->sc_regt, sc->sc_regh, sc->sc_regoff + reg,
	    val);
}

static inline uint32_t
vfb_fb_read4(struct veritefb_softc *sc, bus_size_t off)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_memh, off);
}

static inline void
vfb_fb_write4(struct veritefb_softc *sc, bus_size_t off, uint32_t val)
{
	bus_space_write_4(sc->sc_memt, sc->sc_memh, off, val);
}

/*
 * The FIFO window: input FIFO on write, output FIFO on read.
 */
static void
vfb_fifo_write(struct veritefb_softc *sc, uint32_t word)
{
#ifdef VERITEFB_DEBUG
	sc->sc_ring[sc->sc_ring_count++ & (VFB_RING_SIZE - 1)] = word;
#endif
	bus_space_write_4(sc->sc_regt, sc->sc_regh, VFB_FIFO_SWAP_NO, word);
}

static inline uint32_t
vfb_fifo_read(struct veritefb_softc *sc)
{
	return bus_space_read_4(sc->sc_regt, sc->sc_regh, VFB_FIFO_SWAP_NO);
}

static void
vfb_pacepoll4(struct veritefb_softc *sc, bus_size_t reg, uint32_t data,
    uint32_t mask)
{
	int i;

	for (i = 0; i < VFB_SHORTPOLL; i++)
		if ((vfb_read4(sc, reg) & mask) == (data & mask))
			break;
}

static void
vfb_pacepoll1(struct veritefb_softc *sc, bus_size_t reg, uint8_t data,
    uint8_t mask)
{
	int i;

	for (i = 0; i < VFB_SHORTPOLL; i++)
		if ((vfb_read1(sc, reg) & mask) == (data & mask))
			break;
}

static int
veritefb_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RENDITION &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RENDITION_V2X00)
		return 100;	/* ahead of genfb(4) */

	return 0;
}

static void
veritefb_attach(device_t parent, device_t self, void *aux)
{
	struct veritefb_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args ws_aa;
	struct rasops_info *ri;
	const struct pci_attach_args *pa = aux;
	pcireg_t screg;
	bool console;
	long defattr;

#ifdef VERITEFB_CONSOLE
	console = true;
#else
	console = false;
	prop_dictionary_get_bool(device_properties(self), "is_console",
	    &console);
#endif

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;

	screg = pci_conf_read(sc->sc_pc, sc->sc_pcitag,
	    PCI_COMMAND_STATUS_REG);
	screg |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG,
	    screg);

	pci_aprint_devinfo(pa, NULL);

	if (pci_mapreg_map(pa, VFB_IO_BAR, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, &sc->sc_io_paddr, &sc->sc_ios) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map I/O registers\n");
		return;
	}
	sc->sc_regt = sc->sc_iot;
	sc->sc_regh = sc->sc_ioh;
	sc->sc_regoff = 0;

	/*
	 * Reset and hold it the RISC before touching the rest of the card.
	 */
	veritefb_risc_softreset(sc);
	veritefb_risc_hold(sc);

	if (pci_mapreg_map(pa, VFB_MMIO_BAR, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_mmiot, &sc->sc_mmioh, &sc->sc_mmio_paddr,
	    &sc->sc_mmios) == 0) {
		sc->sc_regt = sc->sc_mmiot;
		sc->sc_regh = sc->sc_mmioh;
		sc->sc_regoff = VFB_MMIO_REG_BASE;
	} else {
		aprint_normal_dev(sc->sc_dev,
		    "MMIO BAR unmappable, all registers via I/O\n");
	}

	if (pci_mapreg_map(pa, VFB_FB_BAR, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_memt, &sc->sc_memh,
	    &sc->sc_fb_paddr, &sc->sc_apsize) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to map framebuffer aperture\n");
		return;
	}

	if (sc->sc_regoff != 0)
		aprint_normal_dev(sc->sc_dev,
		    "fb at 0x%08x, MMIO registers at 0x%08x, "
		    "I/O registers at 0x%04x\n",
		    (uint32_t)sc->sc_fb_paddr, (uint32_t)sc->sc_mmio_paddr,
		    (uint32_t)sc->sc_io_paddr);
	else
		aprint_normal_dev(sc->sc_dev,
		    "fb at 0x%08x, I/O registers at 0x%04x\n",
		    (uint32_t)sc->sc_fb_paddr, (uint32_t)sc->sc_io_paddr);

	sc->sc_memsize = veritefb_mem_size(sc);
	if (sc->sc_memsize == 0) {
		aprint_error_dev(sc->sc_dev, "VRAM probe failed\n");
		return;
	}

	aprint_normal_dev(sc->sc_dev, "%zu MB video memory present\n",
	    sc->sc_memsize / 1024 / 1024);

	/*
	 * The first VFB_MC_SIZE bytes of VRAM are reserved for the 2D
	 * microcode, the framebuffer lives above it.
	 */
	sc->sc_fb_offset = VFB_MC_SIZE;
	sc->sc_accel = VFB_ACCEL_OFF;
	sc->sc_risc_owner = VFB_OWNER_CONSOLE;
#ifdef VERITEFB_DEBUG
	callout_init(&sc->sc_pcsample_ch, 0);
	callout_setfunc(&sc->sc_pcsample_ch, veritefb_pcsample_tick, sc);
#endif

	veritefb_ddc_read(sc);
	veritefb_pick_mode(sc);

	sc->sc_width = sc->sc_videomode->hdisplay;
	sc->sc_height = sc->sc_videomode->vdisplay;
	sc->sc_depth = 8;

	{
		const struct veritefb_stride *st;

		st = veritefb_stride_for(sc->sc_width * (sc->sc_depth / 8));
		if (st == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "no stride encoding for %d bytes/line\n",
			    sc->sc_width * (sc->sc_depth / 8));
			return;
		}
		sc->sc_linebytes = st->linebytes;
		sc->sc_stride0 = st->stride0;
		sc->sc_stride1 = st->stride1;
	}

	aprint_normal_dev(sc->sc_dev, "setting %dx%d %d bpp resolution\n",
	    sc->sc_width, sc->sc_height, sc->sc_depth);

	if (!veritefb_set_mode(sc, sc->sc_videomode)) {
		aprint_error_dev(sc->sc_dev, "mode set failed\n");
		return;
	}

	bus_space_set_region_4(sc->sc_memt, sc->sc_memh, sc->sc_fb_offset, 0,
	    (sc->sc_linebytes * sc->sc_height) / 4);

	veritefb_init_palette(sc);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &veritefb_accessops);
	sc->vd.init_screen = veritefb_init_screen;

	/* Glyph cache in the VRAM above the visible framebuffer. */
	sc->sc_gc.gc_bitblt = veritefb_gc_bitblt;
	sc->sc_gc.gc_rectfill = NULL;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = VFB_ROP_COPY;
	sc->vd.show_screen_cookie = &sc->sc_gc;
	sc->vd.show_screen_cb = glyphcache_adapt;

	ri = &sc->sc_console_screen.scr_ri;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	sc->sc_gc_initted = glyphcache_init(&sc->sc_gc,
	    sc->sc_height + VFB_GC_GAP,
	    (int)((sc->sc_memsize - sc->sc_fb_offset) / sc->sc_linebytes) -
		sc->sc_height - VFB_GC_GAP,
	    sc->sc_width,
	    ri->ri_font->fontwidth,
	    ri->ri_font->fontheight,
	    defattr) == 0;
	if (!sc->sc_gc_initted)
		aprint_error_dev(sc->sc_dev, "glyph cache init failed\n");

	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;

	vcons_redraw_screen(&sc->sc_console_screen);

	if (console) {
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	}

	/*
	 * Interrupt for the vblank flip.
	 */
	{
		pci_intr_handle_t ih;
		char intrbuf[PCI_INTRSTR_LEN];
		const char *intrstr;

		vfb_write1(sc, VFB_INTREN, 0);
		vfb_write1(sc, VFB_INTR, 0xff);
		if (pci_intr_map(pa, &ih) == 0) {
			intrstr = pci_intr_string(sc->sc_pc, ih, intrbuf,
			    sizeof(intrbuf));
			sc->sc_ih = pci_intr_establish_xname(sc->sc_pc, ih,
			    IPL_VM, veritefb_intr, sc,
			    device_xname(sc->sc_dev));
			if (sc->sc_ih != NULL) {
				sc->sc_flip_intr_ok = true;
				aprint_normal_dev(sc->sc_dev,
				    "interrupting at %s\n", intrstr);
			}
		}
		if (!sc->sc_flip_intr_ok)
			aprint_normal_dev(sc->sc_dev,
			    "no interrupt, flips will poll\n");
	}

	ws_aa.console = console;
	ws_aa.scrdata = &sc->sc_screenlist;
	ws_aa.accessops = &veritefb_accessops;
	ws_aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &ws_aa, wsemuldisplaydevprint, CFARGS_NONE);

#if defined(DDB) && defined(VERITEFB_DEBUG)
	veritefb_ddb_attach(sc);
#endif

	/* Firmware needs a mounted root filesystem. */
	config_mountroot(self, veritefb_load_firmware);
}

/*
 * Reset the chip, leaving the RISC held.
 */
static void
veritefb_risc_softreset(struct veritefb_softc *sc)
{
	int i;

	vfb_write1(sc, VFB_DEBUG, VFB_DEBUG_SOFTRESET | VFB_DEBUG_HOLDRISC);
	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_PC);
	for (i = 0; i < 3; i++)
		(void)vfb_read4(sc, VFB_STATEDATA);

	vfb_write1(sc, VFB_DEBUG, VFB_DEBUG_HOLDRISC);
	for (i = 0; i < 3; i++)
		(void)vfb_read4(sc, VFB_STATEDATA);

	/* Clear any pending interrupts, no byte swapping. */
	vfb_write1(sc, VFB_INTR, 0xff);
	vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_NO);
}

/*
 * Make sure the RISC is held.
 */
static void
veritefb_risc_hold(struct veritefb_softc *sc)
{
	uint8_t debugreg;
	int i;

	for (i = 0; i < VFB_MAXPOLL; i++) {
		if ((vfb_read1(sc, VFB_STATUS) & VFB_STATUS_HOLD_MASK) ==
		    VFB_STATUS_HOLD_MASK)
			break;
		delay(1);
	}
	if (i == VFB_MAXPOLL)
		aprint_debug_dev(sc->sc_dev,
		    "timeout waiting for idle status before hold\n");

	debugreg = vfb_read1(sc, VFB_DEBUG);
	vfb_write1(sc, VFB_DEBUG, debugreg | VFB_DEBUG_HOLDRISC);

	for (i = 0; i < VFB_MAXPOLL; i++) {
		if (vfb_read1(sc, VFB_STATUS) & VFB_STATUS_HELD)
			break;
		delay(1);
	}
	if (i == VFB_MAXPOLL)
		aprint_debug_dev(sc->sc_dev,
		    "timeout waiting for hold confirmation\n");
}

/*
 * Probe the amount of VRAM by write/readback at 1 MB steps...
 */
static size_t
veritefb_mem_size(struct veritefb_softc *sc)
{
	const bus_size_t onemeg = 1024 * 1024;
	bus_size_t offset, maxvram;
	uint32_t pattern, start;
	uint8_t modereg, memendian;
	size_t memsize;

	maxvram = MIN(VFB_MAXVRAM, sc->sc_apsize);

	modereg = vfb_read1(sc, VFB_MODE);
	vfb_write1(sc, VFB_MODE, VFB_MODE_NATIVE);
	memendian = vfb_read1(sc, VFB_MEMENDIAN);
	vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_NO);

	start = vfb_fb_read4(sc, 0);
	vfb_fb_write4(sc, 0, VFB_PROBE_START);
	for (offset = onemeg; offset < maxvram; offset += onemeg) {
		pattern = vfb_fb_read4(sc, offset);
		if (pattern == VFB_PROBE_START)
			break;	/* wrapped around, back at offset 0 */

		pattern ^= VFB_PROBE_PATTERN;
		vfb_fb_write4(sc, offset, pattern);
		if (vfb_fb_read4(sc, offset) != pattern) {
			offset -= onemeg;
			break;
		}
		vfb_fb_write4(sc, offset, pattern ^ VFB_PROBE_PATTERN);
	}
	vfb_fb_write4(sc, 0, start);

	if (offset >= maxvram)
		memsize = 4 * onemeg;
	else
		memsize = offset;

	vfb_write1(sc, VFB_MEMENDIAN, memendian);
	vfb_write1(sc, VFB_MODE, modereg);

	return memsize;
}

/*
 * Find PLL parameters for the requested pixel clock
 */
static bool
veritefb_calc_pclk(int kHz, int *m, int *n, int *p)
{
	int64_t target, vco, pcf, freq, diff, mindiff;
	int mm, nn, pp;

	target = (int64_t)kHz * 100;
	mindiff = INT64_MAX;
	*m = *n = *p = 0;

	for (pp = 1; pp <= VFB_PLL_P_MAX; pp++) {
		for (nn = 1; nn <= VFB_PLL_N_MAX; nn++) {
			pcf = VFB_PLL_REF / nn;
			if (pcf < VFB_PLL_PCF_MIN || pcf > VFB_PLL_PCF_MAX)
				continue;
			for (mm = 1; mm <= VFB_PLL_M_MAX; mm++) {
				vco = (int64_t)VFB_PLL_REF * mm / nn;
				if (vco < VFB_PLL_VCO_MIN ||
				    vco > VFB_PLL_VCO_MAX)
					continue;
				freq = vco / pp;
				diff = freq > target ?
				    freq - target : target - freq;
				if (diff < mindiff) {
					*m = mm;
					*n = nn;
					*p = pp;
					mindiff = diff;
				}
			}
		}
	}

	return *m != 0;
}

/*
 * Smallest pixel-engine stride encoding that fits a line of the given
 * width... widths with no dense encoding get a padded framebuffer.
 */
static const struct veritefb_stride *
veritefb_stride_for(int linebytes)
{
	const struct veritefb_stride *st;

	for (st = veritefb_stride_table; st->linebytes != 0; st++)
		if (st->linebytes >= linebytes)
			return st;
	return NULL;
}

/*
 * Program a native (non-VGA) mode: memory/system clocks, pixel clock
 * PLL, RAMDAC, CRTC timing, frame base and stride, then enable video.
 */
static bool
veritefb_set_mode(struct veritefb_softc *sc, const struct videomode *vm)
{
	uint32_t memctl, crtcctl, offset, screenwidth;
	int m, n, p;

	if (!veritefb_calc_pclk(vm->dot_clock, &m, &n, &p)) {
		aprint_error_dev(sc->sc_dev, "no PLL solution for %d kHz\n",
		    vm->dot_clock);
		return false;
	}
	aprint_debug_dev(sc->sc_dev, "PLL M=%d N=%d P=%d for %d kHz\n",
	    m, n, p, vm->dot_clock);

	/* Leave VGA emulation; no legacy 0xA0000 window. */
	vfb_write1(sc, VFB_MODE, VFB_MODE_NATIVE);

	/*
	 * 8bpp this does not matter...
	 * TODO: Revisit for 16/32bpp.
	 */
	switch (sc->sc_depth) {
	case 8:
		vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_END);
		break;
	case 16:
		vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_HW);
		break;
	case 32:
		vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_NO);
		break;
	}

	/* System/memory clock: MClk 110 MHz, SClk 55 MHz. */
	vfb_write4(sc, VFB_SCLKPLL, VFB_SCLKPLL_DEFAULT);
	delay(VFB_PLL_STABILIZE_US);

	/* Resume memory refresh, default write refresh period. */
	memctl = vfb_read4(sc, VFB_MEMCTL) & ~VFB_MEMCTL_HOLDREFRESH;
	vfb_write4(sc, VFB_MEMCTL, memctl | VFB_MEMCTL_WREFRESH_DEFAULT);

	/* Native mode wants swizzled memory addressing. */
	memctl = vfb_read4(sc, VFB_MEMCTL) & ~VFB_MEMCTL_ADRSWIZZLE_MASK;
	vfb_write4(sc, VFB_MEMCTL, memctl);

	vfb_write4(sc, VFB_PCLKPLL,
	    __SHIFTIN((uint32_t)n, VFB_PCLKPLL_N_MASK) |
	    __SHIFTIN((uint32_t)p, VFB_PCLKPLL_P_MASK) |
	    __SHIFTIN((uint32_t)m, VFB_PCLKPLL_M_MASK));
	delay(VFB_PLL_STABILIZE_US);

	veritefb_init_dac(sc);

	vfb_write4(sc, VFB_CRTCHORZ,
	    __SHIFTIN((uint32_t)(vm->hsync_start - vm->hdisplay) / 8 - 1,
		VFB_CRTCHORZ_FRONTPORCH_MASK) |
	    __SHIFTIN((uint32_t)(vm->hsync_end - vm->hsync_start) / 8 - 1,
		VFB_CRTCHORZ_SYNC_MASK) |
	    __SHIFTIN((uint32_t)(vm->htotal - vm->hsync_end) / 8 - 1,
		VFB_CRTCHORZ_BACKPORCH_MASK) |
	    __SHIFTIN((uint32_t)vm->hdisplay / 8 - 1,
		VFB_CRTCHORZ_ACTIVE_MASK));
	vfb_write4(sc, VFB_CRTCVERT,
	    __SHIFTIN((uint32_t)(vm->vsync_start - vm->vdisplay) - 1,
		VFB_CRTCVERT_FRONTPORCH_MASK) |
	    __SHIFTIN((uint32_t)(vm->vsync_end - vm->vsync_start) - 1,
		VFB_CRTCVERT_SYNC_MASK) |
	    __SHIFTIN((uint32_t)(vm->vtotal - vm->vsync_end) - 1,
		VFB_CRTCVERT_BACKPORCH_MASK) |
	    __SHIFTIN((uint32_t)vm->vdisplay - 1,
		VFB_CRTCVERT_ACTIVE_MASK));

	screenwidth = (uint32_t)sc->sc_width * (sc->sc_depth / 8);
	offset = sc->sc_linebytes - screenwidth +
	    screenwidth % VFB_VIDEOFIFO_BYTES;
	if (screenwidth % VFB_VIDEOFIFO_BYTES == 0)
		offset += VFB_VIDEOFIFO_BYTES;
	vfb_write4(sc, VFB_FRAMEBASEA, (uint32_t)sc->sc_fb_offset);
	vfb_write4(sc, VFB_CRTCOFFSET, offset & VFB_CRTCOFFSET_MASK);

	crtcctl = (sc->sc_depth == 16 ? VFB_PIXFMT_565 : VFB_PIXFMT_8I) |
	    VFB_CRTCCTL_VIDEOFIFOSIZE128 |
	    ((vm->flags & VID_PHSYNC) ? VFB_CRTCCTL_HSYNCHI : 0) |
	    ((vm->flags & VID_PVSYNC) ? VFB_CRTCCTL_VSYNCHI : 0) |
	    VFB_CRTCCTL_HSYNCENABLE |
	    VFB_CRTCCTL_VSYNCENABLE |
	    VFB_CRTCCTL_VIDEOENABLE;
	vfb_write4(sc, VFB_CRTCCTL, crtcctl);

	return true;
}

#define VFB_DDC_PACE_US		5	/* between line transitions */
#define VFB_DDC_STRETCH_US	1000	/* max tolerated clock stretch */

static const struct i2c_bitbang_ops veritefb_i2cbb_ops = {
	veritefb_i2cbb_set_bits,
	veritefb_i2cbb_set_dir,
	veritefb_i2cbb_read_bits,
	{
		VFB_CRTCCTL_DDCDATA,	/* SDA */
		VFB_CRTCCTL_DDCOUTPUT,	/* SCL */
		0,			/* open-drain: no direction flip */
		0
	}
};

/*
 * SDA in output mode is push-pull, so open-drain is emulated
 */
static void
veritefb_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct veritefb_softc *sc = cookie;
	uint32_t v;

	v = sc->sc_ddc_base & ~(VFB_CRTCCTL_DDCDATA |
	    VFB_CRTCCTL_DDCOUTPUT | VFB_CRTCCTL_ENABLEDDC);
	if (bits & VFB_CRTCCTL_DDCOUTPUT)
		v |= VFB_CRTCCTL_DDCOUTPUT;	/* SCL: release */
	if (bits & VFB_CRTCCTL_DDCDATA)
		v |= VFB_CRTCCTL_DDCDATA;	/* SDA high: mirror to latch */
	else
		v |= VFB_CRTCCTL_ENABLEDDC;	/* SDA: drive low (b7=0) */

	sc->sc_ddc_base = v;
	vfb_write4(sc, VFB_CRTCCTL, v);
	delay(VFB_DDC_PACE_US);
}

static void
veritefb_i2cbb_set_dir(void *cookie, uint32_t dir)
{
	/* open-drain emulation: direction is part of set_bits */
}

static uint32_t
veritefb_i2cbb_read_bits(void *cookie)
{
	struct veritefb_softc *sc = cookie;

	return vfb_read4(sc, VFB_CRTCCTL);
}

/*
 * I2C START, including a properly shaped repeated START.
 */
static int
veritefb_i2c_send_start(void *cookie, int flags)
{
	struct veritefb_softc *sc = cookie;
	int bail;

	if ((veritefb_i2cbb_read_bits(sc) & VFB_CRTCCTL_DDCOUTPUT) == 0) {
		veritefb_i2cbb_set_bits(sc, VFB_CRTCCTL_DDCDATA);
		delay(VFB_DDC_PACE_US);	/* SDA settle while SCL still low */
		veritefb_i2cbb_set_bits(sc,
		    VFB_CRTCCTL_DDCDATA | VFB_CRTCCTL_DDCOUTPUT);
		for (bail = 0; bail < VFB_DDC_STRETCH_US; bail++) {
			if (veritefb_i2cbb_read_bits(sc) &
			    VFB_CRTCCTL_DDCOUTPUT)
				break;
			delay(1);
		}
		delay(VFB_DDC_PACE_US);	/* START setup time (4.7 us) */
	}
	return i2c_bitbang_send_start(cookie, flags, &veritefb_i2cbb_ops);
}

static int
veritefb_i2c_send_stop(void *cookie, int flags)
{
	return i2c_bitbang_send_stop(cookie, flags, &veritefb_i2cbb_ops);
}

static int
veritefb_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return i2c_bitbang_initiate_xfer(cookie, addr, flags,
	    &veritefb_i2cbb_ops);
}

static int
veritefb_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return i2c_bitbang_read_byte(cookie, valp, flags,
	    &veritefb_i2cbb_ops);
}

static int
veritefb_i2c_write_byte(void *cookie, uint8_t val, int flags)
{
	return i2c_bitbang_write_byte(cookie, val, flags,
	    &veritefb_i2cbb_ops);
}

static void
veritefb_ddc_read(struct veritefb_softc *sc)
{
	int i;

	/*
	 * Release both lines (SDA as input, SCL high-Z) before
	 * starting the controller.
	 */
	sc->sc_ddc_base = vfb_read4(sc, VFB_CRTCCTL) &
	    ~(VFB_CRTCCTL_DDCDATA | VFB_CRTCCTL_DDCOUTPUT |
	      VFB_CRTCCTL_ENABLEDDC);
	vfb_write4(sc, VFB_CRTCCTL,
	    sc->sc_ddc_base | VFB_CRTCCTL_DDCOUTPUT);
	sc->sc_ddc_base |= VFB_CRTCCTL_DDCOUTPUT;

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_send_start = veritefb_i2c_send_start;
	sc->sc_i2c.ic_send_stop = veritefb_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = veritefb_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = veritefb_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = veritefb_i2c_write_byte;

	/* Some monitors do not respond on the first attempt. */
	sc->sc_edid_valid = false;
	memset(sc->sc_edid, 0, sizeof(sc->sc_edid));
	for (i = 0; i < 3; i++) {
		if (ddc_read_edid(&sc->sc_i2c, sc->sc_edid,
		    sizeof(sc->sc_edid)) == 0 && sc->sc_edid[1] != 0)
			break;
		memset(sc->sc_edid, 0, sizeof(sc->sc_edid));
	}

	if (sc->sc_edid[1] == 0) {
		aprint_normal_dev(sc->sc_dev, "DDC: no EDID response\n");
		return;
	}

	if (edid_parse(sc->sc_edid, &sc->sc_ei) != 0) {
		aprint_error_dev(sc->sc_dev, "DDC: EDID parse failed\n");
		return;
	}
	sc->sc_edid_valid = true;
#ifdef VERITEFB_DEBUG
	edid_print(&sc->sc_ei);
#endif
}

/*
 * Can the hardware and this driver do the given mode?
 */
static bool
veritefb_mode_usable(struct veritefb_softc *sc, const struct videomode *m)
{
	const struct veritefb_stride *st;

	if (m->dot_clock > 170000)
		return false;
	if (m->flags & (VID_INTERLACE | VID_DBLSCAN))
		return false;
	if (m->hdisplay > 2048 || (m->hdisplay & 7) != 0 ||
	    m->vdisplay > 2047)
		return false;
	if ((m->hsync_start - m->hdisplay) / 8 - 1 > 0x7 ||
	    (m->hsync_end - m->hsync_start) / 8 - 1 > 0x1f ||
	    (m->htotal - m->hsync_end) / 8 - 1 > 0x3f ||
	    ((m->hsync_start - m->hdisplay) & 7) != 0 ||
	    ((m->hsync_end - m->hsync_start) & 7) != 0 ||
	    ((m->htotal - m->hsync_end) & 7) != 0)
		return false;
	if (m->vsync_start - m->vdisplay < 1 ||
	    m->vsync_start - m->vdisplay - 1 > 0x3f ||
	    m->vsync_end - m->vsync_start - 1 > 0x7 ||
	    m->vtotal - m->vsync_end - 1 > 0x3f)
		return false;
	/* one byte per pixel at 8bpp, the stride may be padded */
	st = veritefb_stride_for(m->hdisplay);
	if (st == NULL)
		return false;
	if ((bus_size_t)st->linebytes * m->vdisplay >
	    sc->sc_memsize - sc->sc_fb_offset)
		return false;
	return true;
}

/*
 * Choose the mode: the monitor's EDID preferred mode when usable,
 * 640x480@60 from the modes database otherwise.
 */
static void
veritefb_pick_mode(struct veritefb_softc *sc)
{
	const struct videomode *m;

	sc->sc_videomode = pick_mode_by_ref(640, 480, 60);
	KASSERT(sc->sc_videomode != NULL);

	if (!sc->sc_edid_valid || sc->sc_ei.edid_preferred_mode == NULL)
		return;

	m = sc->sc_ei.edid_preferred_mode;
	if (!veritefb_mode_usable(sc, m)) {
		aprint_normal_dev(sc->sc_dev,
		    "EDID preferred mode %dx%d (%d kHz) not usable, "
		    "using default\n",
		    m->hdisplay, m->vdisplay, m->dot_clock);
		return;
	}

	aprint_normal_dev(sc->sc_dev, "using EDID mode %dx%d (%d kHz)\n",
	    m->hdisplay, m->vdisplay, m->dot_clock);
	sc->sc_videomode = m;
}

/*
 * Initialize the Bt485-compatible RAMDAC core
 */
static void
veritefb_init_dac(struct veritefb_softc *sc)
{
	uint8_t cmd1;

	if (sc->sc_depth == 16)
		cmd1 = VFB_DACCMD1_16BPP | VFB_DACCMD1_BYPASS_CLUT |
		    VFB_DACCMD1_565 | VFB_DACCMD1_PORT_AB;
	else
		cmd1 = VFB_DACCMD1_8BPP | VFB_DACCMD1_PORT_AB;

	vfb_write1(sc, VFB_DACCOMMAND0,
	    VFB_DACCMD0_EXTENDED | VFB_DACCMD0_8BITDAC);
	vfb_write1(sc, VFB_DACCOMMAND1, cmd1);
	vfb_write1(sc, VFB_DACCOMMAND2,
	    VFB_DACCMD2_PIXEL_INPUT_GATE | VFB_DACCMD2_DISABLE_CURSOR);

	/* Command register 3 is indexed through the write address. */
	vfb_write1(sc, VFB_DACRAMWRITEADR, VFB_DACCMD3_INDEX);
	vfb_write1(sc, VFB_DACCOMMAND3, 0);

	vfb_write1(sc, VFB_DACPIXELMSK, 0xff);
}

/*
 * Wait for vertical sync so palette updates do not tear.
 */
static void
veritefb_wait_vsync(struct veritefb_softc *sc)
{
	int i;

	for (i = 0; i < VFB_VSYNCPOLL; i++) {
		if ((vfb_read4(sc, VFB_CRTCSTATUS) &
		    VFB_CRTCSTATUS_VERT_MASK) == VFB_CRTCSTATUS_VERT_SYNC)
			break;
		delay(1);
	}
}

static void
veritefb_set_dac_entry(struct veritefb_softc *sc, int index,
    uint8_t r, uint8_t g, uint8_t b)
{
	vfb_write1(sc, VFB_DACRAMWRITEADR, index);
	vfb_write1(sc, VFB_DACRAMDATA, r);
	vfb_write1(sc, VFB_DACRAMDATA, g);
	vfb_write1(sc, VFB_DACRAMDATA, b);
}

static void
veritefb_init_palette(struct veritefb_softc *sc)
{
	int i, j;

	j = 0;
	veritefb_wait_vsync(sc);
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		veritefb_set_dac_entry(sc, i, rasops_cmap[j],
		    rasops_cmap[j + 1], rasops_cmap[j + 2]);
		j += 3;
	}
}

static int
veritefb_getcmap(struct veritefb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index], cm->red, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index], cm->blue, count);
	if (error)
		return error;

	return 0;
}

static int
veritefb_putcmap(struct veritefb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char rbuf[256], gbuf[256], bbuf[256];
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	veritefb_wait_vsync(sc);
	for (i = index; i < index + count; i++)
		veritefb_set_dac_entry(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);

	return 0;
}

static void
veritefb_init_screen(void *cookie, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct veritefb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	wsfont_init();

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_linebytes;
	ri->ri_flg = RI_CENTER;

	ri->ri_bits = (char *)bus_space_vaddr(sc->sc_memt, sc->sc_memh) +
	    sc->sc_fb_offset;

	scr->scr_flags |= VCONS_NO_CURSOR;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;

	sc->sc_orig_eraserows = ri->ri_ops.eraserows;
	sc->sc_orig_erasecols = ri->ri_ops.erasecols;
	sc->sc_orig_copyrows = ri->ri_ops.copyrows;
	sc->sc_orig_copycols = ri->ri_ops.copycols;
	sc->sc_orig_putchar = ri->ri_ops.putchar;
	ri->ri_ops.eraserows = veritefb_eraserows;
	ri->ri_ops.erasecols = veritefb_erasecols;
	ri->ri_ops.copyrows = veritefb_copyrows;
	ri->ri_ops.copycols = veritefb_copycols;
	ri->ri_ops.putchar = veritefb_putchar;
}

static int
veritefb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd;
	struct veritefb_softc *sc;
	struct wsdisplay_fbinfo *wsfbi;
	struct vcons_screen *ms;

	vd = v;
	sc = vd->cookie;
	ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_pcitag,
		    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
		    sc->sc_pcitag, data);

	case WSDISPLAYIO_GINFO:
		if (ms == NULL)
			return ENODEV;

		wsfbi = (void *)data;
		wsfbi->height = ms->scr_ri.ri_height;
		wsfbi->width = ms->scr_ri.ri_width;
		wsfbi->depth = ms->scr_ri.ri_depth;
		wsfbi->cmsize = 256;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_linebytes;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return veritefb_getcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return veritefb_putcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int *)data;

			/* sync/hold/reload flows below PIO the FIFO */
			(void)veritefb_dma_drain(sc);
			if (new_mode != sc->sc_mode) {
				sc->sc_mode = new_mode;
				if (new_mode == WSDISPLAYIO_MODE_EMUL) {
					/*
					 * reload and restart the microcode
					 */
					if (sc->sc_accel == VFB_ACCEL_ON ||
					    (sc->sc_accel == VFB_ACCEL_OFF &&
					     sc->sc_ucode != NULL))
						(void)veritefb_risc_init(sc);
					if (sc->sc_gc_initted)
						glyphcache_wipe(&sc->sc_gc);
					veritefb_init_palette(sc);
					vcons_redraw_screen(ms);
				} else {
					if (sc->sc_accel == VFB_ACCEL_ON) {
						veritefb_sync(sc);
						(void)veritefb_drain_outfifo(
						    sc);
						veritefb_risc_hold(sc);
						sc->sc_accel = VFB_ACCEL_OFF;
					} else if (sc->sc_risc_owner !=
					    VFB_OWNER_CONSOLE) {
						veritefb_risc_hold(sc);
						sc->sc_risc_owner =
						    VFB_OWNER_CONSOLE;
					}
				}
			}
			return 0;
		}

	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			struct rasops_info *ri;

			ri = &sc->vd.active->scr_ri;
			return wsdisplayio_get_fbinfo(ri, fbi);
		}

#ifdef VERITEFB_DEBUG
	/* RISC debug surface, VERITEFB_DEBUG kernels only. */
	case VERITEFB_DBG_DIAG:
		{
			struct veritefb_dbg_diag *dd = data;
			unsigned i, n;

			dd->vd_accel = sc->sc_accel;
			dd->vd_owner = sc->sc_risc_owner;
			dd->vd_pc = veritefb_risc_samplepc(sc);
			dd->vd_fifoinfree = vfb_read1(sc, VFB_FIFOINFREE) &
			    VFB_FIFOINFREE_MASK;
			dd->vd_fifooutvalid = vfb_read1(sc,
			    VFB_FIFOOUTVALID) & VFB_FIFOOUTVALID_MASK;
			dd->vd_debugreg = vfb_read1(sc, VFB_DEBUG);
			dd->vd_ringcount = sc->sc_ring_count;
			n = MIN(sc->sc_ring_count, VERITEFB_DIAG_RING);
			memset(dd->vd_ring, 0, sizeof(dd->vd_ring));
			for (i = 0; i < n; i++)
				dd->vd_ring[VERITEFB_DIAG_RING - n + i] =
				    sc->sc_ring[(sc->sc_ring_count - n + i) &
				    (VFB_RING_SIZE - 1)];

			dd->vd_heartbeat = 1;
			if (sc->sc_accel == VFB_ACCEL_ON) {
				uint32_t word = 0;

				veritefb_sync(sc);
				dd->vd_heartbeat = 2;
				for (i = 0; i < VFB_DRAINPOLL; i++) {
					if ((vfb_read1(sc, VFB_FIFOOUTVALID) &
					    VFB_FIFOOUTVALID_MASK) == 0)
						break;
					(void)vfb_fifo_read(sc);
				}
				if ((vfb_read1(sc, VFB_FIFOINFREE) &
				    VFB_FIFOINFREE_MASK) >= 1) {
					vfb_fifo_write(sc,
					    VFB_CMDW(0, VCMD_PIXENGSYNC));
					for (i = 0; i < VFB_FIFOPOLL; i++) {
						if ((vfb_read1(sc,
						    VFB_FIFOOUTVALID) &
						    VFB_FIFOOUTVALID_MASK)
						    != 0) {
							word =
							    vfb_fifo_read(sc);
							break;
						}
						delay(1);
					}
					if (word == VFB_SYNC_TOKEN)
						dd->vd_heartbeat = 0;
				}
			} else if (sc->sc_risc_owner == VFB_OWNER_PARKED) {
				uint32_t word;

				/*
				 * XXX: foreign microcode would eat the ping 
				 * as a command!
				 */
				(void)veritefb_dma_drain(sc);
				dd->vd_heartbeat = 2;
				if (veritefb_drain_outfifo(sc) == 0 &&
				    veritefb_waitfifo_raw(sc, 1,
				      VFB_FIFOPOLL) == 0) {
					vfb_fifo_write(sc, VFB_CSUCODE_PING);
					if (veritefb_read_outfifo_raw(sc,
					      &word, VFB_FIFOPOLL) == 0 &&
					    word == VFB_CSUCODE_PING)
						dd->vd_heartbeat = 0;
				}
			}
			return 0;
		}

	case VERITEFB_DBG_HOLD:
		veritefb_risc_hold(sc);
		return 0;

	case VERITEFB_DBG_CONT:
		veritefb_risc_continue(sc);
		return 0;

	case VERITEFB_DBG_RDREG:
		{
			struct veritefb_dbg_rw *vr = data;

			if ((vfb_read1(sc, VFB_DEBUG) &
			    VFB_DEBUG_HOLDRISC) == 0)
				return EBUSY;	/* hold first */
			if (vr->vr_addr > 255)
				return EINVAL;
			vr->vr_val = veritefb_risc_readrf(sc, vr->vr_addr);
			return 0;
		}

	case VERITEFB_DBG_RDMEM:
		{
			struct veritefb_dbg_rw *vr = data;

			if ((vfb_read1(sc, VFB_DEBUG) &
			    VFB_DEBUG_HOLDRISC) == 0)
				return EBUSY;	/* hold first */
			vr->vr_val = veritefb_risc_readmem(sc, vr->vr_addr);
			return 0;
		}

	case VERITEFB_DBG_FAULT:
		/*
		 * Only against the 2D loop
		 */
		if (sc->sc_risc_owner != VFB_OWNER_CONSOLE)
			return EBUSY;
		aprint_normal_dev(sc->sc_dev,
		    "debug: deliberately sending an invalid command\n");
		vfb_fifo_write(sc, VFB_CMDW(0, VFB_CMD_BOGUS));
		return 0;

	case VERITEFB_DBG_RESET:
		/* Full recovery includes the console scanout mode. */
		if (sc->sc_depth != 8)
			(void)veritefb_set_scan_depth(sc, 8);
		*(int *)data = veritefb_risc_init(sc) ? 1 : 0;
		return 0;

	case VERITEFB_DBG_STATS:
		memcpy(data, &sc->sc_stats, sizeof(sc->sc_stats));
		return 0;

	case VERITEFB_DBG_STATCLR:
		memset(&sc->sc_stats, 0, sizeof(sc->sc_stats));
		return 0;

	case VERITEFB_DBG_PCSAMPLE:
		{
			struct veritefb_dbg_pcsample *ps = data;

			if (ps->vp_shift >= 32)
				return EINVAL;
			sc->sc_pcsample_on = false;
			callout_stop(&sc->sc_pcsample_ch);
			/* reset the histogram on every (re)arm */
			memset(sc->sc_pchist, 0, sizeof(sc->sc_pchist));
			sc->sc_pchist_samples = 0;
			sc->sc_pchist_missed = 0;
			sc->sc_pchist_min = ~0u;
			sc->sc_pchist_max = 0;
			if (ps->vp_hz == 0)
				return 0;	/* stop only */
			sc->sc_pchist_base = ps->vp_base;
			sc->sc_pchist_shift = ps->vp_shift;
			sc->sc_pcsample_hz = ps->vp_hz > (uint32_t)hz ?
			    (uint32_t)hz : ps->vp_hz;
			sc->sc_pcsample_on = true;
			callout_schedule(&sc->sc_pcsample_ch, 1);
			return 0;
		}

	case VERITEFB_DBG_PCHIST:
		{
			struct veritefb_dbg_pchist *ph = data;

			ph->vp_samples = sc->sc_pchist_samples;
			ph->vp_missed = sc->sc_pchist_missed;
			ph->vp_base = sc->sc_pchist_base;
			ph->vp_shift = sc->sc_pchist_shift;
			ph->vp_min = sc->sc_pchist_min;
			ph->vp_max = sc->sc_pchist_max;
			memcpy(ph->vp_hist, sc->sc_pchist,
			    sizeof(ph->vp_hist));
			return 0;
		}

	case VERITEFB_DBG_RDIO:
		{
			struct veritefb_dbg_rw *vr = data;

			if ((vr->vr_addr &
			    ~(uint32_t)VERITEFB_DBG_IO_IOSPACE) > 0xff)
				return EINVAL;
			if (vr->vr_addr & VERITEFB_DBG_IO_IOSPACE)
				vr->vr_val = bus_space_read_1(sc->sc_iot,
				    sc->sc_ioh, vr->vr_addr & 0xff);
			else
				vr->vr_val = vfb_read1(sc, vr->vr_addr);
			return 0;
		}

	case VERITEFB_DBG_WRIO:
		{
			struct veritefb_dbg_rw *vr = data;

			if ((vr->vr_addr &
			    ~(uint32_t)VERITEFB_DBG_IO_IOSPACE) > 0xff)
				return EINVAL;
			if (vr->vr_addr & VERITEFB_DBG_IO_IOSPACE)
				bus_space_write_1(sc->sc_iot, sc->sc_ioh,
				    vr->vr_addr & 0xff, vr->vr_val);
			else
				vfb_write1(sc, vr->vr_addr, vr->vr_val);
			return 0;
		}

	/* RISC multiplexing experiment. */
	case VERITEFB_DBG_SUSPEND2D:
		if (sc->sc_v3d_open)
			return EBUSY;	/* /dev/verite3d owns the mux */
		return veritefb_suspend2d(sc);

	case VERITEFB_DBG_RESUME2D:
		if (sc->sc_v3d_open)
			return EBUSY;
		return veritefb_resume2d(sc);

	case VERITEFB_DBG_UCLOAD:
		{
			struct veritefb_dbg_ucload *vu = data;
			uint8_t *img;
			uint32_t entry;
			int error;
			bool loaded;

			if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL ||
			    sc->sc_ucode == NULL)
				return EBUSY;
			if (vu->vu_size < sizeof(Elf32_Ehdr) ||
			    vu->vu_size > VFB_MAXUCODE)
				return EINVAL;

			/* All user-memory access before any hw touch. */
			img = kmem_alloc(vu->vu_size, KM_SLEEP);
			error = copyin(vu->vu_data, img, vu->vu_size);
			if (error != 0) {
				kmem_free(img, vu->vu_size);
				return error;
			}

			/*
			 * Write the image while the 2D blob still runs
			 */
			loaded = veritefb_ucode_load(sc, img, vu->vu_size,
			    VFB_UCF_BASE, VFB_UCF_END, false, &entry);
			kmem_free(img, vu->vu_size);
			if (!loaded)
				return EINVAL;
			if (!veritefb_risc_init(sc))
				return EIO;
			vu->vu_entry = entry;
			return 0;
		}

	case VERITEFB_DBG_FIFOWR:
		{
			struct veritefb_dbg_fifowr *vf = data;
			uint32_t i;
			int error;

			if (sc->sc_risc_owner == VFB_OWNER_CONSOLE ||
			    sc->sc_accel == VFB_ACCEL_SW)
				return EBUSY;
			if (vf->vf_count < 1 ||
			    vf->vf_count > VFB_DBG_FIFO_MAX)
				return EINVAL;
			if (vf->vf_setowner != VERITEFB_SETOWNER_KEEP &&
			    vf->vf_setowner != VFB_OWNER_PARKED &&
			    vf->vf_setowner != VFB_OWNER_FOREIGN)
				return EINVAL;

			/* PIO into the FIFO: settle any DBG DMA first */
			(void)veritefb_dma_drain(sc);
			error = veritefb_waitfifo_raw(sc, vf->vf_count,
			    VFB_FIFOPOLL);
			if (error != 0)
				return error;
			for (i = 0; i < vf->vf_count; i++)
				vfb_fifo_write(sc, vf->vf_words[i]);
			if (vf->vf_setowner != VERITEFB_SETOWNER_KEEP)
				sc->sc_risc_owner = vf->vf_setowner;
			return 0;
		}

	case VERITEFB_DBG_FIFORD:
		{
			struct veritefb_dbg_fiford *vf = data;

			if (sc->sc_risc_owner == VFB_OWNER_CONSOLE)
				return EBUSY;
			return veritefb_read_outfifo_raw(sc, &vf->vf_word,
			    MIN(vf->vf_timo_us, VFB_FIFOPOLL));
		}

	case VERITEFB_DBG_SETDEPTH:
		/* Never yank the scanout under the live console. */
		if (sc->sc_risc_owner == VFB_OWNER_CONSOLE ||
		    sc->sc_v3d_open)
			return EBUSY;
		return veritefb_set_scan_depth(sc,
		    (int)*(const uint32_t *)data);

	case VERITEFB_DBG_DMASUBMIT:
		{
			struct veritefb_dbg_dma *vdd = data;
			int error;

			/* exclusion rule: no console PIO while DMABusy */
			if (sc->sc_risc_owner == VFB_OWNER_CONSOLE ||
			    sc->sc_v3d_open)
				return EBUSY;
			if (vdd->vd_len == 0 ||
			    vdd->vd_len > VFB_DMA_BOUNCE ||
			    (vdd->vd_len & 3) != 0 || vdd->vd_swap > 3)
				return EINVAL;
			error = veritefb_dma_alloc(sc);
			if (error)
				return error;
			error = copyin((const void *)(uintptr_t)vdd->vd_buf,
			    sc->sc_dma_buf, vdd->vd_len);
			if (error)
				return error;
			return veritefb_dma_submit(sc, sc->sc_dma_buf_map,
			    vdd->vd_len, vdd->vd_swap);
		}

	case VERITEFB_DBG_RDVRAM:
		{
			struct veritefb_dbg_rw *vr = data;
			uint8_t memendian;

			if (vr->vr_addr >= VFB_MC_SIZE ||
			    (vr->vr_addr & 3) != 0)
				return EINVAL;
			memendian = vfb_read1(sc, VFB_MEMENDIAN);
			vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_NO);
			vr->vr_val = vfb_fb_read4(sc, vr->vr_addr);
			vfb_write1(sc, VFB_MEMENDIAN, memendian);
			return 0;
		}

#endif /* VERITEFB_DEBUG */
	}
	return EPASSTHROUGH;
}

static paddr_t
veritefb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct veritefb_softc *sc;

	vd = v;
	sc = vd->cookie;

	if (offset >= 0 && offset < sc->sc_memsize - sc->sc_fb_offset)
		return bus_space_mmap(sc->sc_memt,
		    sc->sc_fb_paddr + sc->sc_fb_offset + offset,
		    0, prot, BUS_SPACE_MAP_LINEAR);

	return -1;
}

static void
veritefb_risc_continue(struct veritefb_softc *sc)
{
	uint8_t debugreg;

	debugreg = vfb_read1(sc, VFB_DEBUG);
	vfb_write1(sc, VFB_DEBUG, debugreg & ~VFB_DEBUG_HOLDRISC);
	vfb_pacepoll4(sc, VFB_STATEDATA, 0, 0);
}

/*
 * Force one instruction into the RISC decoder and single-step it
 */
static void
veritefb_risc_forcestep(struct veritefb_softc *sc, uint32_t insn)
{
	uint8_t debugreg, stateindex;
	int i;

	debugreg = vfb_read1(sc, VFB_DEBUG);
	stateindex = vfb_read1(sc, VFB_STATEINDEX);

	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_IR);
	vfb_pacepoll1(sc, VFB_STATEINDEX, VFB_STATEINDEX_IR, 0xff);
	vfb_write4(sc, VFB_STATEDATA, insn);
	vfb_pacepoll4(sc, VFB_STATEDATA, insn, 0xffffffff);
	vfb_write1(sc, VFB_DEBUG,
	    debugreg | VFB_DEBUG_HOLDRISC | VFB_DEBUG_STEPRISC);
	vfb_pacepoll4(sc, VFB_STATEDATA, 0, 0);

	for (i = 0; i < VFB_SHORTPOLL; i++)
		if ((vfb_read1(sc, VFB_DEBUG) &
		    (VFB_DEBUG_HOLDRISC | VFB_DEBUG_STEPRISC)) ==
		    VFB_DEBUG_HOLDRISC)
			break;

	vfb_write1(sc, VFB_STATEINDEX, stateindex);
}

/*
 * Set a register-file entry by force-feeding load-immediate sequences
 */
static void
veritefb_risc_writerf(struct veritefb_softc *sc, uint8_t idx, uint32_t data)
{
	uint8_t special = 0;

	if (idx < 64) {
		special = idx;
		idx = VRISC_SP;
	}

	if ((data & 0xff000000) == 0) {
		veritefb_risc_forcestep(sc,
		    VRISC_LI(VRISC_LI_OP, idx, data & 0xffff));
		if (data & 0x00ff0000)
			veritefb_risc_forcestep(sc,
			    VRISC_INT(VRISC_ADDIFI_OP, idx, idx, data >> 16));
	} else {
		veritefb_risc_forcestep(sc,
		    VRISC_LI(VRISC_LUI_OP, idx, data >> 16));
		veritefb_risc_forcestep(sc,
		    VRISC_INT(VRISC_ADDSL8_OP, idx, idx, (data >> 8) & 0xff));
		veritefb_risc_forcestep(sc,
		    VRISC_INT(VRISC_ADDI_OP, idx, idx, data & 0xff));
	}

	if (special) {
		veritefb_risc_forcestep(sc,
		    VRISC_INT(VRISC_ADD_OP, special, 0, VRISC_SP));
		veritefb_risc_forcestep(sc, VRISC_NOP);
		veritefb_risc_forcestep(sc, VRISC_NOP);
		veritefb_risc_forcestep(sc, VRISC_NOP);
	}
}

/* Read a register-file entry through the S1 operand bus */
static uint32_t
veritefb_risc_readrf(struct veritefb_softc *sc, uint8_t idx)
{
	uint32_t data, insn;
	uint8_t debugreg, stateindex;

	debugreg = vfb_read1(sc, VFB_DEBUG);
	stateindex = vfb_read1(sc, VFB_STATEINDEX);

	vfb_write1(sc, VFB_DEBUG, debugreg | VFB_DEBUG_HOLDRISC);

	/* add zero, zero, idx: puts RF[idx] on the S1 bus, no step needed */
	insn = VRISC_INT(VRISC_ADD_OP, 0, 0, idx);
	vfb_write4(sc, VFB_STATEDATA, insn);

	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_IR);
	vfb_pacepoll4(sc, VFB_STATEDATA, insn, 0xffffffff);

	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_S1);
	vfb_pacepoll4(sc, VFB_STATEINDEX, 0, 0);
	data = vfb_read4(sc, VFB_STATEDATA);

	vfb_write1(sc, VFB_STATEINDEX, stateindex);
	vfb_write1(sc, VFB_DEBUG, debugreg);

	return data;
}

/* Word read/write through the RISC's own address space, RISC held. */
static uint32_t
veritefb_risc_readmem(struct veritefb_softc *sc, uint32_t addr)
{
	veritefb_risc_writerf(sc, VRISC_RA, addr);
	veritefb_risc_forcestep(sc, VRISC_LD(VRISC_LW_OP, VRISC_SP, 0,
	    VRISC_RA));
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);
	return veritefb_risc_readrf(sc, VRISC_SP);
}

static void
veritefb_risc_writemem(struct veritefb_softc *sc, uint32_t addr,
    uint32_t data)
{
	veritefb_risc_writerf(sc, VRISC_RA, addr);
	veritefb_risc_writerf(sc, VRISC_FP, data);
	veritefb_risc_forcestep(sc, VRISC_ST(VRISC_SW_OP, 0, VRISC_FP,
	    VRISC_RA));
}

/*
 * Flush the icache (and the pixel-engine line buffers in the
 * dcache), returns with the icache enabled.
 */
static void
veritefb_risc_flushicache(struct veritefb_softc *sc)
{
	uint32_t c, p1, p2;

	/* flush store accumulation buffers */
	p1 = veritefb_risc_readmem(sc, 0);
	p2 = veritefb_risc_readmem(sc, 8);
	veritefb_risc_writemem(sc, 0, p1);
	veritefb_risc_writemem(sc, 8, p2);
	(void)veritefb_risc_readmem(sc, 0);
	(void)veritefb_risc_readmem(sc, 8);

	/* spri Sync, zero: flush pixel-engine line buffers */
	veritefb_risc_forcestep(sc, VRISC_INT(VRISC_SPRI_OP, 0, 0, 31));
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);

	/* set icache-off bits in the flag register */
	veritefb_risc_writerf(sc, VRISC_RA, VRISC_ICACHE_ONOFF_MASK);
	veritefb_risc_forcestep(sc,
	    VRISC_INT(VRISC_OR_OP, VRISC_FLAG, VRISC_FLAG, VRISC_RA));
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);

	/* jump through two icache's worth of lines to flush it */
	for (c = 0; c < VRISC_ICACHESIZE * 2; c += VRISC_ICACHELINESIZE)
		veritefb_risc_forcestep(sc, VRISC_JMP(c >> 2));

	/* clear the icache-off bits again */
	veritefb_risc_writerf(sc, VRISC_RA, VRISC_ICACHE_ONOFF_MASK);
	veritefb_risc_forcestep(sc,
	    VRISC_INT(VRISC_ANDN_OP, VRISC_FLAG, VRISC_FLAG, VRISC_RA));
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_JMP(0));
	veritefb_risc_forcestep(sc, VRISC_NOP);
}

/*
 * Start the RISC at pc: hold it, force-feed a jump (with NOPs for the
 * pipeline and the delay slot), release.
 */
static void
veritefb_risc_start(struct veritefb_softc *sc, uint32_t pc)
{
	veritefb_risc_hold(sc);
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_forcestep(sc, VRISC_JMP(pc >> 2));
	veritefb_risc_forcestep(sc, VRISC_NOP);
	veritefb_risc_continue(sc);
}

/*
 * Sample the RISC program counter.
 */
static uint32_t
veritefb_risc_samplepc(struct veritefb_softc *sc)
{
	uint32_t pc;
	uint8_t debugreg, stateindex;
	bool washeld;

	debugreg = vfb_read1(sc, VFB_DEBUG);
	washeld = (debugreg & VFB_DEBUG_HOLDRISC) != 0;
	if (!washeld)
		veritefb_risc_hold(sc);

	stateindex = vfb_read1(sc, VFB_STATEINDEX);
	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_PC);
	vfb_pacepoll1(sc, VFB_STATEINDEX, VFB_STATEINDEX_PC, 0xff);
	pc = vfb_read4(sc, VFB_STATEDATA);
	vfb_write1(sc, VFB_STATEINDEX, stateindex);

	if (!washeld)
		veritefb_risc_continue(sc);
	return pc;
}

#ifdef VERITEFB_DEBUG
#define VFB_PCSAMPLE_POLL	200	/* us budget to confirm a sampling hold */
/*
 * Lightweight PC sampler for the profiler callout.
 */
static bool
veritefb_risc_samplepc_fast(struct veritefb_softc *sc, uint32_t *pcp)
{
	uint8_t debugreg, stateindex;
	bool washeld;
	int i;

	debugreg = vfb_read1(sc, VFB_DEBUG);
	washeld = (debugreg & VFB_DEBUG_HOLDRISC) != 0;
	if (!washeld) {
		vfb_write1(sc, VFB_DEBUG, debugreg | VFB_DEBUG_HOLDRISC);
		for (i = 0; i < VFB_PCSAMPLE_POLL; i++) {
			if (vfb_read1(sc, VFB_STATUS) & VFB_STATUS_HELD)
				break;
			delay(1);
		}
		if (i == VFB_PCSAMPLE_POLL) {
			veritefb_risc_continue(sc);	/* did not halt - skip */
			return false;
		}
	}

	stateindex = vfb_read1(sc, VFB_STATEINDEX);
	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_PC);
	vfb_pacepoll1(sc, VFB_STATEINDEX, VFB_STATEINDEX_PC, 0xff);
	*pcp = vfb_read4(sc, VFB_STATEDATA);
	vfb_write1(sc, VFB_STATEINDEX, stateindex);

	if (!washeld)
		veritefb_risc_continue(sc);
	return true;
}

/*
 * RISC PC sampling profiler callout.
 */
static void
veritefb_pcsample_tick(void *arg)
{
	struct veritefb_softc *sc = arg;
	uint32_t pc;
	int ticks;

	if (!sc->sc_pcsample_on)
		return;
	if (sc->sc_risc_owner == VFB_OWNER_FOREIGN && sc->sc_v3d_open &&
	    veritefb_risc_samplepc_fast(sc, &pc)) {
		sc->sc_pchist_samples++;
		if (pc < sc->sc_pchist_min)
			sc->sc_pchist_min = pc;
		if (pc > sc->sc_pchist_max)
			sc->sc_pchist_max = pc;
		if (pc >= sc->sc_pchist_base) {
			uint32_t b = (pc - sc->sc_pchist_base) >>
			    sc->sc_pchist_shift;

			if (b < VFB_PCHIST_BUCKETS)
				sc->sc_pchist[b]++;
			else
				sc->sc_pchist_missed++;
		} else
			sc->sc_pchist_missed++;
	}

	ticks = hz / sc->sc_pcsample_hz;
	if (ticks < 1)
		ticks = 1;
	callout_schedule(&sc->sc_pcsample_ch, ticks);
}
#endif /* VERITEFB_DEBUG */

/*
 * The hang-signature catalog: classify a sampled PC against the known
 * layout of the V2x00 2D blob (loaded at its link address).
 */
static const char *
veritefb_pc_signature(uint32_t pc)
{
	if (pc >= VFB_UC_TRAP && pc < VFB_UC_TRAP_END)
		return "invalid-command trap (host sent a bad command or "
		    "the stream desynced)";
	if (pc >= VFB_UC_BASE && pc <= VFB_UC_DISPATCH_END)
		return "dispatch loop (idle, waiting for commands)";
	if (pc >= VFB_CSUCODE_BASE && pc < VFB_UC_BASE)
		return "csucode monitor (parked/suspended)";
	if (pc >= VFB_UC_BASE && pc < VFB_UC_END)
		return "inside a command handler";
	if (pc >= VFB_UCF_BASE && pc < VFB_UCF_END)
		return "foreign ucode region";
	if (pc >= VFB_RISC_ROM_BASE)
		return "boot ROM region";
	return "unknown region";
}

/*
 * Permanently degrade to software rendering, duh.
 */
static void
veritefb_accel_fail(struct veritefb_softc *sc, const char *what)
{
	uint32_t pc;
#ifdef VERITEFB_DEBUG
	unsigned i, n;
#endif

	if (sc->sc_accel == VFB_ACCEL_SW)
		return;

	/* never hold the RISC mid-DMA */
	(void)veritefb_dma_drain(sc);

	sc->sc_accel = VFB_ACCEL_SW;
	sc->sc_unsynced = 0;
	sc->sc_unsynced_area = 0;
	pc = veritefb_risc_samplepc(sc);
	aprint_error_dev(sc->sc_dev,
	    "%s; disabling acceleration until reboot\n", what);
	aprint_error_dev(sc->sc_dev, "RISC PC 0x%08x: %s\n", pc,
	    veritefb_pc_signature(pc));

#ifdef VERITEFB_DEBUG
	n = MIN(sc->sc_ring_count, 8);
	for (i = 0; i < n; i++)
		aprint_error_dev(sc->sc_dev, "  fifo[-%u] = 0x%08x\n",
		    n - i, sc->sc_ring[(sc->sc_ring_count - n + i) &
		    (VFB_RING_SIZE - 1)]);
#endif

	veritefb_risc_hold(sc);
}

/* wait for n free input FIFO entries */
static int
veritefb_waitfifo(struct veritefb_softc *sc, int n)
{
	int i;

	for (i = 0; i < VFB_FIFOPOLL; i++) {
		if ((vfb_read1(sc, VFB_FIFOINFREE) & VFB_FIFOINFREE_MASK) >=
		    n)
			return 0;
		delay(1);
	}
	veritefb_accel_fail(sc, "input FIFO timeout");
	return EBUSY;
}

/* Discard stale output FIFO words. */
static int
veritefb_drain_outfifo(struct veritefb_softc *sc)
{
	int i;

	for (i = 0; i < VFB_FIFOPOLL; i++) {
		if ((vfb_read1(sc, VFB_FIFOOUTVALID) &
		    VFB_FIFOOUTVALID_MASK) == 0)
			return 0;
		(void)vfb_fifo_read(sc);
	}
	veritefb_accel_fail(sc, "output FIFO never drained");
	return EBUSY;
}

/* Wait for one word from the output FIFO. */
static int
veritefb_read_outfifo(struct veritefb_softc *sc, uint32_t *wordp)
{
	int i;

	for (i = 0; i < VFB_FIFOPOLL; i++) {
		if ((vfb_read1(sc, VFB_FIFOOUTVALID) &
		    VFB_FIFOOUTVALID_MASK) != 0) {
			*wordp = vfb_fifo_read(sc);
			return 0;
		}
		delay(1);
	}
	veritefb_accel_fail(sc, "output FIFO timeout");
	return EBUSY;
}

/*
 * Copy an ELF microcode image into its VRAM slot
 */
static bool
veritefb_ucode_load(struct veritefb_softc *sc, const uint8_t *u,
    size_t size, uint32_t lo, uint32_t hi, bool install_csucode,
    uint32_t *entryp)
{
	uint32_t entry, phoff, filesz, memsz, off, paddr, word;
	uint16_t phentsize, phnum, ph;
	uint8_t memendian;
	size_t i;

	if (size < sizeof(Elf32_Ehdr) ||
	    memcmp(u, ELFMAG, SELFMAG) != 0 ||
	    u[EI_CLASS] != ELFCLASS32 || u[EI_DATA] != ELFDATA2MSB) {
		aprint_error_dev(sc->sc_dev, "microcode is not a "
		    "big-endian ELF32 image\n");
		return false;
	}

	entry = be32dec(u + offsetof(Elf32_Ehdr, e_entry));
	phoff = be32dec(u + offsetof(Elf32_Ehdr, e_phoff));
	phentsize = be16dec(u + offsetof(Elf32_Ehdr, e_phentsize));
	phnum = be16dec(u + offsetof(Elf32_Ehdr, e_phnum));

	if (phnum == 0 || phentsize < sizeof(Elf32_Phdr) ||
	    (uint64_t)phoff + (uint64_t)phnum * phentsize > size) {
		aprint_error_dev(sc->sc_dev,
		    "microcode program headers out of bounds\n");
		return false;
	}

	if (entry < lo || entry >= hi) {
		aprint_error_dev(sc->sc_dev,
		    "microcode entry 0x%x outside slot [0x%x, 0x%x)\n",
		    entry, lo, hi);
		return false;
	}

	memendian = vfb_read1(sc, VFB_MEMENDIAN);
	vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_NO);

	if (install_csucode) {
		/* Context-switch monitor and its semaphores. */
		for (i = 0; i < __arraycount(veritefb_csucode); i++)
			vfb_fb_write4(sc, VFB_CSUCODE_BASE + i * 4,
			    veritefb_csucode[i]);
		vfb_fb_write4(sc, VFB_CSUCODE_SEM0, 0);
		vfb_fb_write4(sc, VFB_CSUCODE_SEM1, 0);
	}

	for (ph = 0; ph < phnum; ph++) {
		const uint8_t *p = u + phoff + (uint32_t)ph * phentsize;

		if (be32dec(p + offsetof(Elf32_Phdr, p_type)) != PT_LOAD)
			continue;
		off = be32dec(p + offsetof(Elf32_Phdr, p_offset));
		filesz = be32dec(p + offsetof(Elf32_Phdr, p_filesz));
		memsz = be32dec(p + offsetof(Elf32_Phdr, p_memsz));
		paddr = be32dec(p + offsetof(Elf32_Phdr, p_paddr));

		if (memsz < filesz ||
		    (uint64_t)off + filesz > size ||
		    paddr < lo || (paddr & 3) != 0 ||
		    (uint64_t)paddr + roundup(memsz, 4) > hi) {
			aprint_error_dev(sc->sc_dev,
			    "microcode segment out of bounds "
			    "(paddr 0x%x filesz 0x%x memsz 0x%x)\n",
			    paddr, filesz, memsz);
			vfb_write1(sc, VFB_MEMENDIAN, memendian);
			return false;
		}

		for (i = 0; i + 4 <= filesz; i += 4)
			vfb_fb_write4(sc, paddr + i, be32dec(u + off + i));
		if (i < filesz) {
			word = 0;
			for (; i < filesz; i++)
				word = (word << 8) | u[off + i];
			word <<= 8 * (4 - (filesz & 3));
			vfb_fb_write4(sc, paddr + (filesz & ~3U), word);
			i = (filesz & ~3U) + 4;
		}
		/* Zero the bss tail. */
		for (; i < memsz; i += 4)
			vfb_fb_write4(sc, paddr + i, 0);
	}

	vfb_write1(sc, VFB_MEMENDIAN, memendian);
	*entryp = entry;
	return true;
}

/*
 * Full RISC bring-up: 
 * - load microcode
 * - start the csucode monitor
 * - feed it the init sequence
 * - validate the command protocol
 */
static bool
veritefb_risc_init(struct veritefb_softc *sc)
{

	/*
	 * Settle any in-flight DMA first!
	 */
	(void)veritefb_dma_drain(sc);

	if (sc->sc_ucode == NULL)
		return false;

	sc->sc_accel = VFB_ACCEL_OFF;
	sc->sc_risc_owner = VFB_OWNER_CONSOLE;

	veritefb_risc_hold(sc);
	if (!veritefb_ucode_load(sc, sc->sc_ucode, sc->sc_ucode_size,
	    VFB_UC2D_LO, VFB_UC2D_HI, true, &sc->sc_ucode_entry))
		return false;

	/*
	 * same VRAM words read through the host aperture and through 
	 * injected RISC loads must agree
	 */
	{
		static const uint32_t testaddr[] =
		    { VFB_UC_BASE, VFB_CSUCODE_BASE };
		uint32_t hostv, riscv;
		uint8_t memendian;
		size_t t;

		memendian = vfb_read1(sc, VFB_MEMENDIAN);
		vfb_write1(sc, VFB_MEMENDIAN, VFB_MEMENDIAN_NO);
		for (t = 0; t < __arraycount(testaddr); t++) {
			hostv = vfb_fb_read4(sc, testaddr[t]);
			riscv = veritefb_risc_readmem(sc, testaddr[t]);
			if (hostv != riscv) {
				vfb_write1(sc, VFB_MEMENDIAN, memendian);
				aprint_error_dev(sc->sc_dev,
				    "dual-view self-test failed @0x%x: "
				    "host 0x%08x vs RISC 0x%08x\n",
				    testaddr[t], hostv, riscv);
				return false;
			}
		}
		vfb_write1(sc, VFB_MEMENDIAN, memendian);
		aprint_debug_dev(sc->sc_dev, "dual-view self-test passed\n");
	}

	veritefb_risc_flushicache(sc);
	veritefb_risc_start(sc, VFB_CSUCODE_BASE);

	if (veritefb_waitfifo(sc, 4) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CSUCODE_INIT);
	vfb_fifo_write(sc, 0);		/* context store area */
	vfb_fifo_write(sc, 0);
	vfb_fifo_write(sc, sc->sc_ucode_entry);

	if (!veritefb_2d_handshake(sc))
		return false;

	aprint_normal_dev(sc->sc_dev,
	    "RISC running 2D microcode (entry 0x%x), handshake passed\n",
	    sc->sc_ucode_entry);
	return true;
}

/*
 * Post-start validation and pixel-engine setup
 */
static bool
veritefb_2d_handshake(struct veritefb_softc *sc)
{
	uint32_t word, saved;

	if (veritefb_drain_outfifo(sc) != 0)
		return false;
	if (veritefb_waitfifo(sc, 1) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_PIXENGSYNC));
	if (veritefb_read_outfifo(sc, &word) != 0)
		return false;
	if (word != VFB_SYNC_TOKEN) {
		veritefb_accel_fail(sc, "bad sync token from microcode");
		return false;
	}

	if (veritefb_waitfifo(sc, 6) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_SETUP));
	/*
	 * Word 1 programs the pixel-engine scissor.
	 * It MUST span the whole VRAM working area.
	 */
	vfb_fifo_write(sc, VFB_P2(sc->sc_width,
	    (sc->sc_memsize - sc->sc_fb_offset) / sc->sc_linebytes));
	vfb_fifo_write(sc, VFB_P2(sc->sc_depth, VFB_PIXFMT_8I));
	vfb_fifo_write(sc, (uint32_t)sc->sc_fb_offset);
	vfb_fifo_write(sc, sc->sc_linebytes);
	vfb_fifo_write(sc, ((uint32_t)sc->sc_stride1 << 12) |
	    ((uint32_t)sc->sc_stride0 << 8));

	/* Second sync proves Setup consumed exactly six words. */
	if (veritefb_waitfifo(sc, 1) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_PIXENGSYNC));
	if (veritefb_read_outfifo(sc, &word) != 0)
		return false;
	if (word != VFB_SYNC_TOKEN) {
		veritefb_accel_fail(sc, "FIFO desync after Setup");
		return false;
	}

	/*
	 * GetPixel round-trip: place the same magic byte in the first
	 * four framebuffer pixels through the aperture (immune to the
	 * MEMENDIAN byte-lane setting) and ask the RISC for pixel (0,0).
	 */
	saved = vfb_fb_read4(sc, sc->sc_fb_offset);
	vfb_fb_write4(sc, sc->sc_fb_offset, VFB_GETPIXEL_PATTERN);
	if (veritefb_waitfifo(sc, 2) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_GETPIXEL));
	vfb_fifo_write(sc, VFB_P2(0, 0));
	if (veritefb_read_outfifo(sc, &word) != 0)
		return false;
	vfb_fb_write4(sc, sc->sc_fb_offset, saved);
	if ((word & 0xff) != (VFB_GETPIXEL_PATTERN & 0xff)) {
		veritefb_accel_fail(sc, "GetPixel round-trip mismatch");
		return false;
	}

	sc->sc_accel = VFB_ACCEL_ON;
	sc->sc_unsynced = 0;
	sc->sc_unsynced_area = 0;
	return true;
}

static int
veritefb_waitfifo_raw(struct veritefb_softc *sc, int n, uint32_t timo_us)
{
	uint32_t i;

	for (i = 0; i < timo_us; i++) {
		if ((vfb_read1(sc, VFB_FIFOINFREE) & VFB_FIFOINFREE_MASK) >=
		    n)
			return 0;
		delay(1);
	}
	return ETIMEDOUT;
}

static int
veritefb_read_outfifo_raw(struct veritefb_softc *sc, uint32_t *wordp,
    uint32_t timo_us)
{
	uint32_t i;

	for (i = 0; i < timo_us; i++) {
		if ((vfb_read1(sc, VFB_FIFOOUTVALID) &
		    VFB_FIFOOUTVALID_MASK) != 0) {
			*wordp = vfb_fifo_read(sc);
			return 0;
		}
		delay(1);
	}
	return ETIMEDOUT;
}

/* Park the 2D microcode in the csucode monitor. */
static int
veritefb_suspend2d(struct veritefb_softc *sc)
{
	uint32_t word;

	if (sc->sc_accel != VFB_ACCEL_ON ||
	    sc->sc_mode != WSDISPLAYIO_MODE_EMUL ||
	    sc->sc_risc_owner != VFB_OWNER_CONSOLE)
		return EBUSY;

	/* PIO FIFO words follow */
	(void)veritefb_dma_drain(sc);

	/*
	 * Console text ops fall back to software from here on
	 */
	sc->sc_accel = VFB_ACCEL_OFF;
	sc->sc_risc_owner = VFB_OWNER_PARKED;
	sc->sc_unsynced = 0;
	sc->sc_unsynced_area = 0;

	/*
	 * Pixel engine MUST be idle before another microcode
	 * takes over...
	 */
	if (veritefb_drain_outfifo(sc) != 0 ||
	    veritefb_waitfifo(sc, 2) != 0)
		goto fail;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_PIXENGSYNC));
	if (veritefb_read_outfifo(sc, &word) != 0 ||
	    word != VFB_SYNC_TOKEN)
		goto fail;

	/* Blob saves its state and parks. */
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_SUSPEND));

	/* Only the monitor echoes pings */
	if (veritefb_waitfifo(sc, 1) != 0)
		goto fail;
	vfb_fifo_write(sc, VFB_CSUCODE_PING);
	if (veritefb_read_outfifo(sc, &word) != 0 ||
	    word != VFB_CSUCODE_PING)
		goto fail;

	aprint_debug_dev(sc->sc_dev, "2D microcode parked\n");
	return 0;
fail:
	aprint_error_dev(sc->sc_dev, "2D suspend failed, reloading\n");
	(void)veritefb_risc_init(sc);
	return EIO;
}

/*
 * DMA engine pulls a word stream into the same input FIFO PIO writes 
 * feed.
 */
static int
veritefb_dma_alloc(struct veritefb_softc *sc)
{
	int nsegs, error;

	if (sc->sc_dma_buf != NULL)
		return 0;

	/* descriptor block: one page satisfies the 16KB-block rule */
	error = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, PAGE_SIZE, 0,
	    &sc->sc_dma_list_seg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dma_list_seg, 1,
	    PAGE_SIZE, (void **)&sc->sc_dma_list, BUS_DMA_WAITOK);
	if (error)
		goto fail;
	error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &sc->sc_dma_list_map);
	if (error)
		goto fail;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dma_list_map,
	    sc->sc_dma_list, PAGE_SIZE, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail;

	error = bus_dmamem_alloc(sc->sc_dmat, VFB_DMA_BOUNCE, PAGE_SIZE, 0,
	    &sc->sc_dma_buf_seg, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		goto fail;
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dma_buf_seg, 1,
	    VFB_DMA_BOUNCE, (void **)&sc->sc_dma_buf, BUS_DMA_WAITOK);
	if (error)
		goto fail;
	error = bus_dmamap_create(sc->sc_dmat, VFB_DMA_BOUNCE, 1,
	    VFB_DMA_BOUNCE, 0, BUS_DMA_WAITOK, &sc->sc_dma_buf_map);
	if (error)
		goto fail;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dma_buf_map,
	    sc->sc_dma_buf, VFB_DMA_BOUNCE, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail;

	aprint_debug_dev(sc->sc_dev, "DMA bounce %u bytes at pa 0x%lx, "
	    "list at pa 0x%lx\n", VFB_DMA_BOUNCE,
	    (u_long)sc->sc_dma_buf_map->dm_segs[0].ds_addr,
	    (u_long)sc->sc_dma_list_map->dm_segs[0].ds_addr);
	return 0;
fail:
	/* one-shot lazy init */
	aprint_error_dev(sc->sc_dev, "DMA setup failed: %d\n", error);
	sc->sc_dma_buf = NULL;
	return error;
}

#ifdef VERITEFB_DEBUG
static void
vfb_stat(struct veritefb_softc *sc, int idx, const struct timeval *t0)
{
	struct timeval t1;

	microuptime(&t1);
	sc->sc_stats.vs_count[idx]++;
	sc->sc_stats.vs_us[idx] +=
	    (uint64_t)(t1.tv_sec - t0->tv_sec) * 1000000 +
	    (t1.tv_usec - t0->tv_usec);
}
#define VFB_T0()	struct timeval t0_; microuptime(&t0_)
#define VFB_STAT(sc, idx)	vfb_stat(sc, idx, &t0_)
#else
#define VFB_T0()	do { } while (0)
#define VFB_STAT(sc, idx)	do { } while (0)
#endif

/*
 * Settle the in-flight descriptor DMA
 */
static int
veritefb_dma_drain(struct veritefb_softc *sc)
{
	uint32_t i;
	VFB_T0();

	if (!sc->sc_dma_pending)
		return 0;

#ifdef VERITEFB_DEBUG
	/* overlap metric: did the engine finish during CPU build? */
	sc->sc_stats.vs_count[
	    (vfb_read4(sc, VFB_DMACMDPTR) & VFB_DMACMDPTR_BUSY) != 0 ?
	    VFB_STAT_V3D_DRAIN_BUSY : VFB_STAT_V3D_DRAIN_IDLE]++;
#endif
	for (i = 0; i < VFB_DMA_TIMEOUT_US; i++) {
		if ((vfb_read4(sc, VFB_DMACMDPTR) & VFB_DMACMDPTR_BUSY) == 0)
			break;
		delay(1);
	}
	VFB_STAT(sc, VFB_STAT_V3D_SPIN);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_pending_map, 0,
	    sc->sc_dma_pending_len, BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_list_map, 0, 16,
	    BUS_DMASYNC_POSTWRITE);
	sc->sc_dma_pending = false;

	if (i == VFB_DMA_TIMEOUT_US) {
		/* stop a wedged master before it holds the bus forever */
		vfb_write1(sc, VFB_MODE, VFB_MODE_NATIVE);
		aprint_error_dev(sc->sc_dev,
		    "DMA drain timeout, engine off\n");
		return EIO;
	}
	return 0;
}

static int
veritefb_intr(void *arg)
{
	struct veritefb_softc *sc = arg;
	uint8_t st;

	st = vfb_read1(sc, VFB_INTR);
	if (st == 0)
		return 0;
	vfb_write1(sc, VFB_INTR, st);		/* W1C ack */
	if ((st & VFB_INTR_VERT) != 0 && sc->sc_flip_pending) {
		vfb_write4(sc, VFB_FRAMEBASEA, sc->sc_flip_base);
		sc->sc_flip_pending = false;
		vfb_write1(sc, VFB_INTREN, 0);
	}
	return 1;
}

/* Abandon a queued flip (teardown/modeset take scanout over). */
static void
veritefb_flip_cancel(struct veritefb_softc *sc)
{

	if (!sc->sc_flip_intr_ok)
		return;
	sc->sc_flip_pending = false;
	vfb_write1(sc, VFB_INTREN, 0);
}

static void
veritefb_flip_wait(struct veritefb_softc *sc)
{
	uint32_t i;

	if (!sc->sc_flip_pending)
		return;
	for (i = 0; i < 2 * VFB_VSYNCPOLL; i++) {
		if (!sc->sc_flip_pending)
			return;
		delay(1);
	}
	vfb_write4(sc, VFB_FRAMEBASEA, sc->sc_flip_base);
	sc->sc_flip_pending = false;
	vfb_write1(sc, VFB_INTREN, 0);
	aprint_error_dev(sc->sc_dev, "queued flip lost, forced\n");
}

static int
veritefb_dma_submit(struct veritefb_softc *sc, bus_dmamap_t map,
    uint32_t len, uint32_t swap)
{
	uint32_t *dl = sc->sc_dma_list;
	int error;
	VFB_T0();

	/* queued flip MUST retire before commands can exec */
	veritefb_flip_wait(sc);

	/* never kick (or rewrite Mode) while the master runs */
	error = veritefb_dma_drain(sc);
	if (error)
		return error;

	/* data entry: len is a byte count = (words << 2) at b23:2 */
	dl[0] = htole32((uint32_t)map->dm_segs[0].ds_addr);
	dl[1] = htole32(len | swap);
	/* stop-DMA entry: link flag with a null address */
	dl[2] = htole32(0);
	dl[3] = htole32(VFB_DMALEN_LINK);

	bus_dmamap_sync(sc->sc_dmat, map, 0, len,
	    BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_list_map, 0, 16,
	    BUS_DMASYNC_PREWRITE);

	vfb_write1(sc, VFB_LOWWATERMARK, VFB_DMA_LOWWATER);
	vfb_write1(sc, VFB_MODE, VFB_MODE_NATIVE | VFB_MODE_DMAEN);
	vfb_write4(sc, VFB_DMACMDPTR,
	    (uint32_t)sc->sc_dma_list_map->dm_segs[0].ds_addr);

	/* async: the drain (here or at any drain point) settles it */
	sc->sc_dma_pending = true;
	sc->sc_dma_pending_map = map;
	sc->sc_dma_pending_len = len;

	VFB_STAT(sc, VFB_STAT_V3D_SUBMIT);
#ifdef VERITEFB_DEBUG
	sc->sc_stats.vs_count[VFB_STAT_V3D_BYTES] += len;
#endif
	return 0;
}

/*
 * TODO: Consider WSDISPLAYIO_SET_MODE in the future
 */
static int
veritefb_set_scan_depth(struct veritefb_softc *sc, int depth)
{
	const struct veritefb_stride *st;

	if (depth != 8 && depth != 16)
		return EINVAL;
	if (depth == sc->sc_depth)
		return 0;
	st = veritefb_stride_for(sc->sc_width * (depth / 8));
	if (st == NULL)
		return EINVAL;
	/* the modeset rewrites Mode, which would abort a live master */
	(void)veritefb_dma_drain(sc);
	veritefb_flip_cancel(sc);
	sc->sc_depth = depth;
	sc->sc_linebytes = st->linebytes;
	return veritefb_set_mode(sc, sc->sc_videomode) ? 0 : EIO;
}

/* Resume the parked 2D microcode from the csucode monitor. */
static int
veritefb_resume2d(struct veritefb_softc *sc)
{
	uint32_t word;

	if (sc->sc_risc_owner == VFB_OWNER_CONSOLE ||
	    sc->sc_mode != WSDISPLAYIO_MODE_EMUL ||
	    sc->sc_ucode == NULL)
		return EBUSY;

	/* modeset + PIO FIFO words follow */
	(void)veritefb_dma_drain(sc);

	if (sc->sc_depth != 8)
		(void)veritefb_set_scan_depth(sc, 8);
	vfb_write4(sc, VFB_FRAMEBASEA, (uint32_t)sc->sc_fb_offset);

	if (veritefb_drain_outfifo(sc) != 0 ||
	    veritefb_waitfifo(sc, 1) != 0)
		goto fail;
	vfb_fifo_write(sc, VFB_CSUCODE_PING);
	if (veritefb_read_outfifo(sc, &word) != 0 ||
	    word != VFB_CSUCODE_PING)
		goto fail;

	if (veritefb_waitfifo(sc, 4) != 0)
		goto fail;
	vfb_fifo_write(sc, VFB_CSUCODE_RESUME);
	vfb_fifo_write(sc, 0);		/* context store area */
	vfb_fifo_write(sc, 0);
	vfb_fifo_write(sc, sc->sc_ucode_entry);

	/* PE state did not survive; the handshake replays Setup. */
	if (!veritefb_2d_handshake(sc))
		goto fail;
	sc->sc_risc_owner = VFB_OWNER_CONSOLE;

	if (sc->sc_gc_initted)
		glyphcache_wipe(&sc->sc_gc);
	veritefb_mux_redraw(sc);

	aprint_debug_dev(sc->sc_dev, "2D microcode resumed\n");
	return 0;
fail:
	aprint_error_dev(sc->sc_dev, "2D resume failed, reloading\n");
	if (veritefb_risc_init(sc)) {
		if (sc->sc_gc_initted)
			glyphcache_wipe(&sc->sc_gc);
		veritefb_mux_redraw(sc);
	}
	return EIO;
}

/*
 * Full-screen redraw after a resume
 */
static void
veritefb_mux_redraw(struct veritefb_softc *sc)
{

	if (db_active)
		return;
	if (sc->vd.active != NULL)
		vcons_redraw_screen(sc->vd.active);
}

/*
 * /dev/verite3d — exclusive 3D client interface
 */

const struct cdevsw verite3d_cdevsw = {
	.d_open = verite3d_open,
	.d_close = verite3d_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = verite3d_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = verite3d_mmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int
verite3d_ring_alloc(struct veritefb_softc *sc)
{
	int s, nsegs, error;

	if (sc->sc_v3d_slot[0] != NULL)
		return 0;
	for (s = 0; s < V3D_RING_SLOTS; s++) {
		error = bus_dmamem_alloc(sc->sc_dmat, V3D_SLOT_SIZE,
		    PAGE_SIZE, 0, &sc->sc_v3d_slot_seg[s], 1, &nsegs,
		    BUS_DMA_WAITOK);
		if (error)
			return error;
		error = bus_dmamem_map(sc->sc_dmat, &sc->sc_v3d_slot_seg[s],
		    1, V3D_SLOT_SIZE, (void **)&sc->sc_v3d_slot[s],
		    BUS_DMA_WAITOK);
		if (error)
			return error;
		error = bus_dmamap_create(sc->sc_dmat, V3D_SLOT_SIZE, 1,
		    V3D_SLOT_SIZE, 0, BUS_DMA_WAITOK,
		    &sc->sc_v3d_slot_map[s]);
		if (error)
			return error;
		error = bus_dmamap_load(sc->sc_dmat, sc->sc_v3d_slot_map[s],
		    sc->sc_v3d_slot[s], V3D_SLOT_SIZE, NULL, BUS_DMA_WAITOK);
		if (error)
			return error;
	}
	return 0;
}

int
verite3d_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct veritefb_softc *sc;
	int error;

	sc = device_lookup_private(&veritefb_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_v3d_open)
		return EBUSY;

	/* console must be healthy and in EMUL; parking happens at INIT */
	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL || sc->sc_ucode == NULL ||
	    sc->sc_risc_owner != VFB_OWNER_CONSOLE)
		return EBUSY;

	error = veritefb_dma_alloc(sc);	/* descriptor list page */
	if (error)
		return error;
	error = verite3d_ring_alloc(sc);
	if (error)
		return error;

	if (sc->sc_v3d_ext == NULL) {
		sc->sc_v3d_pool_base = (uint32_t)(sc->sc_fb_offset +
		    (bus_size_t)sc->sc_linebytes * sc->sc_height);
		sc->sc_v3d_ext = extent_create("verite3d",
		    sc->sc_v3d_pool_base, sc->sc_memsize - 1, NULL, 0,
		    EX_WAITOK);
	}
	if (sc->sc_v3d_reg == NULL)
		sc->sc_v3d_reg = kmem_zalloc(V3D_MAX_REGIONS *
		    sizeof(sc->sc_v3d_reg[0]), KM_SLEEP);
	sc->sc_v3d_nreg = 0;

	sc->sc_v3d_open = true;
	sc->sc_v3d_dead = false;
	sc->sc_v3d_inited = false;
	return 0;
}

/* Everything back to the console, whatever the client left behind. */
static void
verite3d_teardown(struct veritefb_softc *sc)
{

	(void)veritefb_dma_drain(sc);
	veritefb_flip_cancel(sc);

	if (sc->sc_v3d_dead || sc->sc_v3d_inited) {
		/*
		 * full reload is the only safe route back
		 */
		if (sc->sc_depth != 8)
			(void)veritefb_set_scan_depth(sc, 8);
		vfb_write4(sc, VFB_FRAMEBASEA, (uint32_t)sc->sc_fb_offset);
		if (veritefb_risc_init(sc)) {
			if (sc->sc_gc_initted)
				glyphcache_wipe(&sc->sc_gc);
			veritefb_mux_redraw(sc);
		}
	}
	/* else: INIT never ran, the console was never parked */
	if (sc->sc_v3d_ext != NULL) {
		extent_destroy(sc->sc_v3d_ext);
		sc->sc_v3d_ext = NULL;
	}
	sc->sc_v3d_open = false;
	sc->sc_v3d_dead = false;
	sc->sc_v3d_inited = false;
}

int
verite3d_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct veritefb_softc *sc;

	sc = device_lookup_private(&veritefb_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
#ifdef VERITEFB_DEBUG
	/* the GL client is going away: stop sampling its PC */
	sc->sc_pcsample_on = false;
	callout_stop(&sc->sc_pcsample_ch);
#endif
	if (sc->sc_v3d_open)
		verite3d_teardown(sc);
	return 0;
}

paddr_t
verite3d_mmap(dev_t dev, off_t off, int prot)
{
	struct veritefb_softc *sc;
	int s;

	sc = device_lookup_private(&veritefb_cd, minor(dev));
	if (sc == NULL || !sc->sc_v3d_open)
		return (paddr_t)-1;
	if (off < 0)
		return (paddr_t)-1;
	if (off >= V3D_VRAM_MMAP_OFF) {
		off_t voff = off - V3D_VRAM_MMAP_OFF;

		if (voff >= (off_t)sc->sc_memsize)
			return (paddr_t)-1;
		return bus_space_mmap(sc->sc_memt, sc->sc_fb_paddr + voff,
		    0, prot, BUS_SPACE_MAP_LINEAR);
	}
	s = (int)(off / V3D_SLOT_SIZE);
	if (sc->sc_v3d_slot[s] == NULL)
		return (paddr_t)-1;
	return bus_dmamem_mmap(sc->sc_dmat, &sc->sc_v3d_slot_seg[s], 1,
	    off % V3D_SLOT_SIZE, prot, 0);
}

int
verite3d_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct veritefb_softc *sc;
	int error;

	sc = device_lookup_private(&veritefb_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	if (!sc->sc_v3d_open)
		return EBADF;
	if (sc->sc_v3d_dead && cmd != V3D_MODE)
		return EIO;

	switch (cmd) {
	case V3D_INIT:
		{
			struct v3d_init *vi = data;
			uint8_t *img;
			uint32_t entry;
			bool ok;

			if (sc->sc_v3d_inited)
				return EBUSY;
			if (vi->vi_size < sizeof(Elf32_Ehdr) ||
			    vi->vi_size > VFB_MAXUCODE)
				return EINVAL;
			img = kmem_alloc(vi->vi_size, KM_SLEEP);
			error = copyin((const void *)(uintptr_t)vi->vi_ucode,
			    img, vi->vi_size);
			if (error) {
				kmem_free(img, vi->vi_size);
				return error;
			}

			ok = veritefb_ucode_load(sc, img, vi->vi_size,
			    VFB_UCF_BASE, VFB_UCF_END, false, &entry);
			kmem_free(img, vi->vi_size);
			if (!ok)
				return EINVAL;
			if (!veritefb_risc_init(sc))
				goto dead;
			error = veritefb_suspend2d(sc);
			if (error)
				return error;
			if (veritefb_waitfifo_raw(sc, 4, 100000) != 0)
				goto dead;
			sc->sc_risc_owner = VFB_OWNER_FOREIGN;
			vfb_fifo_write(sc, VFB_CSUCODE_INIT);
			vfb_fifo_write(sc, VFB_CTX_BASE);
			vfb_fifo_write(sc, 0);
			vfb_fifo_write(sc, entry);

			sc->sc_v3d_inited = true;
			vi->vi_ctx_base = VFB_CTX_BASE;
			vi->vi_memsize = (uint32_t)sc->sc_memsize;
			vi->vi_pool_base = sc->sc_v3d_pool_base;
			return 0;
		}

	case V3D_SUBMIT:
		{
			struct v3d_submit *vs = data;

			if (!sc->sc_v3d_inited)
				return EINVAL;
			if (vs->vs_slot >= V3D_RING_SLOTS ||
			    vs->vs_len == 0 || vs->vs_len > V3D_SLOT_SIZE ||
			    (vs->vs_len & 3) != 0 || vs->vs_swap > 3)
				return EINVAL;
			error = veritefb_dma_submit(sc,
			    sc->sc_v3d_slot_map[vs->vs_slot], vs->vs_len,
			    vs->vs_swap);
			if (error)
				goto dead;
			return 0;
		}

	case V3D_SYNC:
		{
			uint32_t word = *(uint32_t *)data;
			VFB_T0();

			if (!sc->sc_v3d_inited)
				return EINVAL;

			if (veritefb_dma_drain(sc) != 0)
				goto dead;
#ifdef VERITEFB_DEBUG
			microuptime(&t0_);	/* round-trip time only */
#endif
			if (veritefb_waitfifo_raw(sc, 1, 100000) != 0)
				goto dead;
			vfb_fifo_write(sc, word);
			if (veritefb_read_outfifo_raw(sc, &word,
			    500000) != 0)
				goto dead;
			*(uint32_t *)data = word;
			VFB_STAT(sc, VFB_STAT_V3D_SYNC);
			return 0;
		}

	case V3D_ALLOC:
		{
			struct v3d_alloc *va = data;
			u_long result;

			if (va->va_size == 0 || sc->sc_v3d_ext == NULL ||
			    sc->sc_v3d_nreg >= V3D_MAX_REGIONS)
				return EINVAL;
			error = extent_alloc(sc->sc_v3d_ext, va->va_size,
			    va->va_align ? va->va_align : 4, 0, EX_NOWAIT,
			    &result);
			if (error)
				return error;
			sc->sc_v3d_reg[sc->sc_v3d_nreg].addr =
			    (uint32_t)result;
			sc->sc_v3d_reg[sc->sc_v3d_nreg].size = va->va_size;
			sc->sc_v3d_nreg++;
			va->va_addr = (uint32_t)result;
			return 0;
		}

	case V3D_FREE:
		{
			uint32_t addr = *(const uint32_t *)data;
			int i;

			for (i = 0; i < sc->sc_v3d_nreg; i++)
				if (sc->sc_v3d_reg[i].addr == addr)
					break;
			if (i == sc->sc_v3d_nreg)
				return EINVAL;
			(void)extent_free(sc->sc_v3d_ext, addr,
			    sc->sc_v3d_reg[i].size, EX_NOWAIT);
			sc->sc_v3d_reg[i] =
			    sc->sc_v3d_reg[--sc->sc_v3d_nreg];
			return 0;
		}

	case V3D_MODE:
		{
			struct v3d_mode *vm = data;
			const struct veritefb_stride *st;

			if (vm->vm_depth != 0) {
				error = veritefb_set_scan_depth(sc,
				    (int)vm->vm_depth);
				if (error)
					return error;
			}
			if (vm->vm_frame_base != 0) {
				veritefb_flip_cancel(sc);
				veritefb_wait_vsync(sc);
				vfb_write4(sc, VFB_FRAMEBASEA,
				    vm->vm_frame_base);
			}
			vm->vm_width = (uint32_t)sc->sc_width;
			vm->vm_height = (uint32_t)sc->sc_height;
			vm->vm_stride = (uint32_t)sc->sc_linebytes;
			st = veritefb_stride_for(sc->sc_width *
			    (sc->sc_depth / 8));
			vm->vm_pe_stride = st == NULL ? 0 :
			    ((uint32_t)st->stride1 << 12) |
			    ((uint32_t)st->stride0 << 8);
			return 0;
		}

	case V3D_FLIP:
		{
			uint32_t base = *(uint32_t *)data;
			VFB_T0();

			if ((base & V3D_FLIP_NOWAIT) != 0) {
				VFB_STAT(sc, VFB_STAT_V3D_FLIP);
				vfb_write4(sc, VFB_FRAMEBASEA,
				    base & ~V3D_FLIP_NOWAIT);
				return 0;
			}
			if (sc->sc_flip_intr_ok) {
				veritefb_flip_wait(sc);
				sc->sc_flip_base = base;
				sc->sc_flip_pending = true;
				/* arm on the NEXT vblank edge only */
				vfb_write1(sc, VFB_INTR, VFB_INTR_VERT);
				vfb_write1(sc, VFB_INTREN, VFB_INTR_VERT);
				VFB_STAT(sc, VFB_STAT_V3D_FLIP);
				return 0;
			}
			veritefb_wait_vsync(sc);
			VFB_STAT(sc, VFB_STAT_V3D_FLIP);
			vfb_write4(sc, VFB_FRAMEBASEA, base);
			return 0;
		}

	default:
		return EPASSTHROUGH;
	}

dead:
	/*
	 * The stream or the engine is borked.
	 */
	sc->sc_v3d_dead = true;
	if (sc->sc_depth != 8)
		(void)veritefb_set_scan_depth(sc, 8);
	(void)veritefb_risc_init(sc);
	sc->sc_risc_owner = VFB_OWNER_CONSOLE;
	return EIO;
}

/*
 * The microcode ships as a firmware file, so it can only be pulled in
 * once the root filesystem exists.
 */
static void
veritefb_load_firmware(device_t self)
{
	struct veritefb_softc *sc = device_private(self);
	firmware_handle_t fh;
	size_t size;
	int error;

	error = firmware_open("veritefb", VFB_FIRMWARE_NAME, &fh);
	if (error != 0) {
		aprint_normal_dev(sc->sc_dev,
		    "no microcode (firmware veritefb/%s), "
		    "running unaccelerated\n", VFB_FIRMWARE_NAME);
		return;
	}

	size = firmware_get_size(fh);
	if (size == 0 || size > VFB_MAXUCODE) {
		aprint_error_dev(sc->sc_dev,
		    "implausible microcode size %zu\n", size);
		firmware_close(fh);
		return;
	}

	sc->sc_ucode = firmware_malloc(size);
	sc->sc_ucode_size = size;
	error = firmware_read(fh, 0, sc->sc_ucode, size);
	firmware_close(fh);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "microcode read failed: %d\n",
		    error);
		firmware_free(sc->sc_ucode, size);
		sc->sc_ucode = NULL;
		return;
	}

	(void)veritefb_risc_init(sc);
}

/*
 * no-op until operations have been queued, then a PixengSync round-trip
 */
static void
veritefb_sync(struct veritefb_softc *sc)
{
	uint32_t word;
	VFB_T0();

	if (sc->sc_unsynced == 0)
		return;
	sc->sc_unsynced = 0;
	sc->sc_unsynced_area = 0;
	if (veritefb_drain_outfifo(sc) != 0)
		return;
	if (veritefb_waitfifo(sc, 1) != 0)
		return;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_PIXENGSYNC));
	if (veritefb_read_outfifo(sc, &word) != 0)
		return;
	if (word != VFB_SYNC_TOKEN) {
		veritefb_accel_fail(sc, "bad sync token after operation");
		return;
	}
	VFB_STAT(sc, VFB_STAT_SYNC);
}

/* FillRectSolidRop */
static bool
veritefb_rectfill(struct veritefb_softc *sc, int x, int y, int w, int h,
    uint32_t color)
{
	if (veritefb_waitfifo(sc, 4) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CMDW(VFB_ROP_COPY, VCMD_FILLRECTSOLIDROP));
	vfb_fifo_write(sc, color);
	vfb_fifo_write(sc, VFB_P2(x, y));
	vfb_fifo_write(sc, VFB_P2(w, h));
	sc->sc_unsynced_area += (uint32_t)w * h;
	if (++sc->sc_unsynced >= VFB_LAZY_LIMIT ||
	    sc->sc_unsynced_area >= VFB_LAZY_AREA)
		veritefb_sync(sc);
	return sc->sc_accel == VFB_ACCEL_ON;
}

/* ScreenBlt */
static bool
veritefb_bitblt(struct veritefb_softc *sc, int sx, int sy, int dx, int dy,
    int w, int h)
{
	if (veritefb_waitfifo(sc, 5) != 0)
		return false;
	vfb_fifo_write(sc, VFB_CMDW(0, VCMD_SCREENBLT));
	vfb_fifo_write(sc, VFB_ROP_COPY);
	vfb_fifo_write(sc, VFB_P2(sx, sy));
	vfb_fifo_write(sc, VFB_P2(w, h));
	vfb_fifo_write(sc, VFB_P2(dx, dy));
	sc->sc_unsynced_area += (uint32_t)w * h;
	if (++sc->sc_unsynced >= VFB_LAZY_LIMIT ||
	    sc->sc_unsynced_area >= VFB_LAZY_AREA)
		veritefb_sync(sc);
	return sc->sc_accel == VFB_ACCEL_ON;
}

static inline bool
veritefb_accel_op_ok(struct veritefb_softc *sc)
{
	return sc->sc_accel == VFB_ACCEL_ON &&
	    sc->sc_mode == WSDISPLAYIO_MODE_EMUL;
}

static void
veritefb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct veritefb_softc *sc = scr->scr_cookie;
	int x, y, w, h;
	VFB_T0();

	if (!veritefb_accel_op_ok(sc)) {
		sc->sc_orig_eraserows(cookie, row, nrows, fillattr);
		VFB_STAT(sc, VFB_STAT_FILL);
		return;
	}

	if (row == 0 && nrows == ri->ri_rows) {
		x = y = 0;
		w = ri->ri_width;
		h = ri->ri_height;
	} else {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + row * ri->ri_font->fontheight;
		w = ri->ri_emuwidth;
		h = nrows * ri->ri_font->fontheight;
	}
	if (!veritefb_rectfill(sc, x, y, w, h,
	    ri->ri_devcmap[(fillattr >> 16) & 0xf]))
		sc->sc_orig_eraserows(cookie, row, nrows, fillattr);
	VFB_STAT(sc, VFB_STAT_FILL);
}

static void
veritefb_erasecols(void *cookie, int row, int startcol, int ncols,
    long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct veritefb_softc *sc = scr->scr_cookie;
	int x, y, w, h;

	if (!veritefb_accel_op_ok(sc)) {
		sc->sc_orig_erasecols(cookie, row, startcol, ncols,
		    fillattr);
		return;
	}

	x = ri->ri_xorigin + startcol * ri->ri_font->fontwidth;
	y = ri->ri_yorigin + row * ri->ri_font->fontheight;
	w = ncols * ri->ri_font->fontwidth;
	h = ri->ri_font->fontheight;
	if (!veritefb_rectfill(sc, x, y, w, h,
	    ri->ri_devcmap[(fillattr >> 16) & 0xf]))
		sc->sc_orig_erasecols(cookie, row, startcol, ncols,
		    fillattr);
}

static void
veritefb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct veritefb_softc *sc = scr->scr_cookie;
	int x, sy, dy, w, h;
	VFB_T0();

	if (!veritefb_accel_op_ok(sc)) {
		sc->sc_orig_copyrows(cookie, srcrow, dstrow, nrows);
		VFB_STAT(sc, VFB_STAT_BLT);
		return;
	}

	x = ri->ri_xorigin;
	sy = ri->ri_yorigin + srcrow * ri->ri_font->fontheight;
	dy = ri->ri_yorigin + dstrow * ri->ri_font->fontheight;
	w = ri->ri_emuwidth;
	h = nrows * ri->ri_font->fontheight;
	if (!veritefb_bitblt(sc, x, sy, x, dy, w, h))
		sc->sc_orig_copyrows(cookie, srcrow, dstrow, nrows);
	VFB_STAT(sc, VFB_STAT_BLT);
}

static void
veritefb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct veritefb_softc *sc = scr->scr_cookie;
	int sx, dx, y, w, h;

	if (!veritefb_accel_op_ok(sc)) {
		sc->sc_orig_copycols(cookie, row, srccol, dstcol, ncols);
		return;
	}

	sx = ri->ri_xorigin + srccol * ri->ri_font->fontwidth;
	dx = ri->ri_xorigin + dstcol * ri->ri_font->fontwidth;
	y = ri->ri_yorigin + row * ri->ri_font->fontheight;
	w = ncols * ri->ri_font->fontwidth;
	h = ri->ri_font->fontheight;
	if (!veritefb_bitblt(sc, sx, y, dx, y, w, h))
		sc->sc_orig_copycols(cookie, row, srccol, dstcol, ncols);
}

/* Blit-within-VRAM for the glyph cache. */
static void
veritefb_gc_bitblt(void *cookie, int xs, int ys, int xd, int yd, int wi,
    int he, int rop)
{
	struct veritefb_softc *sc = cookie;

	(void)veritefb_bitblt(sc, xs, ys, xd, yd, wi, he);
}

/*
 * Glyph-cached putchar
 */
static void
veritefb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct veritefb_softc *sc = scr->scr_cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	uint32_t fg, bg;
	int x, y, wi, he, rv;
	VFB_T0();

	/* Anything drawn before firmload lands here. */
	if (!veritefb_accel_op_ok(sc) || !sc->sc_gc_initted) {
		veritefb_sync(sc);
		sc->sc_orig_putchar(cookie, row, col, c, attr);
		VFB_STAT(sc, VFB_STAT_CHAR_SW);
		return;
	}

	if (!CHAR_IN_FONT(c, font))
		return;

	wi = font->fontwidth;
	he = font->fontheight;
	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;
	fg = ri->ri_devcmap[(attr >> 24) & 0xf];
	bg = ri->ri_devcmap[(attr >> 16) & 0xf];

	if (c == ' ') {
		if (!veritefb_rectfill(sc, x, y, wi, he, bg)) {
			sc->sc_orig_putchar(cookie, row, col, c, attr);
			return;
		}
		if (attr & WSATTR_UNDERLINE)
			(void)veritefb_rectfill(sc,
			    x, y + he - VFB_UNDERLINE_OFF, wi, 1, fg);
		VFB_STAT(sc, VFB_STAT_CHAR_SPACE);
		return;
	}

	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv == GC_OK) {
		VFB_STAT(sc, VFB_STAT_CHAR_HIT);
		return;
	}

	/*
	 * The engine may still be executing queued operations...
	 */
	veritefb_sync(sc);
	sc->sc_orig_putchar(cookie, row, col, c, attr &
	    ~(long)(WSATTR_REVERSE | WSATTR_HILIT | WSATTR_BLINK |
	    WSATTR_UNDERLINE));

	if (rv == GC_ADD) {
		glyphcache_add(&sc->sc_gc, c, x, y);
	} else if (attr & WSATTR_UNDERLINE)
		(void)veritefb_rectfill(sc,
		    x, y + he - VFB_UNDERLINE_OFF, wi, 1, fg);
	VFB_STAT(sc, VFB_STAT_CHAR_ADD);
}

#if defined(DDB) && defined(VERITEFB_DEBUG)
/*
 * ddb 'verite*' commands, wrappers over the RISC debug port.
 * 'veriteregs' leaves the RISC held and clobbers the decoder IR,
 * resume with 'veritereset', not 'veritecont'.
 */

static struct veritefb_softc *veritefb_ddb_sc;

static const char *
veritefb_db_regname(int idx)
{
	switch (idx) {
	case VRISC_FLAG:	return "flag";
	case 176:		return "cmd-param";
	case 177:		return "cmd-index";
	case 224:		return "DispatchTable";
	case 225:		return "code-base";
	case 227:		return "shadow/fb-base";
	case 235:		return "load-bias";
	case VRISC_SP:		return "SP(dbg-scratch)";
	case VRISC_RA:		return "RA(dbg-scratch)";
	case VRISC_FP:		return "FP(dbg-scratch)";
	default:		return NULL;
	}
}

static void
veritefb_db_diag(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;
	uint32_t pc, word;
	unsigned i, n;

	if (sc == NULL) {
		db_printf("veritefb not attached\n");
		return;
	}

	db_printf("accel state: %s, RISC owner: %s\n",
	    sc->sc_accel == VFB_ACCEL_ON ? "ON" :
	    sc->sc_accel == VFB_ACCEL_SW ? "SW (degraded)" : "OFF",
	    sc->sc_risc_owner == VFB_OWNER_CONSOLE ? "console" :
	    sc->sc_risc_owner == VFB_OWNER_PARKED ? "parked" : "foreign");
	db_printf("FIFOINFREE %u/31, FIFOOUTVALID %u, DEBUG 0x%02x\n",
	    vfb_read1(sc, VFB_FIFOINFREE) & VFB_FIFOINFREE_MASK,
	    vfb_read1(sc, VFB_FIFOOUTVALID) & VFB_FIFOOUTVALID_MASK,
	    vfb_read1(sc, VFB_DEBUG));

	pc = veritefb_risc_samplepc(sc);
	db_printf("RISC PC 0x%08x: %s\n", pc, veritefb_pc_signature(pc));

	n = MIN(sc->sc_ring_count, 16);
	db_printf("last %u FIFO words (oldest first):\n", n);
	for (i = 0; i < n; i++)
		db_printf("  [-%2u] 0x%08x\n", n - i,
		    sc->sc_ring[(sc->sc_ring_count - n + i) &
		    (VFB_RING_SIZE - 1)]);

	/* Heartbeat, only if we believe the engine is alive. */
	if (sc->sc_accel == VFB_ACCEL_ON) {
		for (i = 0; i < VFB_DRAINPOLL; i++) {
			if ((vfb_read1(sc, VFB_FIFOOUTVALID) &
			    VFB_FIFOOUTVALID_MASK) == 0)
				break;
			(void)vfb_fifo_read(sc);
			delay(1);
		}
		if ((vfb_read1(sc, VFB_FIFOINFREE) &
		    VFB_FIFOINFREE_MASK) < 1) {
			db_printf("heartbeat: input FIFO full - RISC "
			    "not consuming (wedged)\n");
			return;
		}
		vfb_fifo_write(sc, VFB_CMDW(0, VCMD_PIXENGSYNC));
		for (i = 0; i < VFB_FIFOPOLL; i++) {
			if ((vfb_read1(sc, VFB_FIFOOUTVALID) &
			    VFB_FIFOOUTVALID_MASK) != 0)
				break;
			delay(1);
		}
		if (i == VFB_FIFOPOLL) {
			db_printf("heartbeat: no sync token (RISC or "
			    "pixel engine wedged)\n");
		} else {
			word = vfb_fifo_read(sc);
			db_printf("heartbeat: token 0x%08x (%s)\n", word,
			    word == VFB_SYNC_TOKEN ? "healthy" : "BAD");
		}
	} else if (sc->sc_risc_owner == VFB_OWNER_PARKED) {
		/* PARKED = the DBG-DMA configuration; the ping is PIO */
		(void)veritefb_dma_drain(sc);
		if (veritefb_drain_outfifo(sc) != 0 ||
		    veritefb_waitfifo_raw(sc, 1, VFB_FIFOPOLL) != 0) {
			db_printf("heartbeat: monitor FIFO stuck\n");
			return;
		}
		vfb_fifo_write(sc, VFB_CSUCODE_PING);
		if (veritefb_read_outfifo_raw(sc, &word, VFB_FIFOPOLL) == 0)
			db_printf("heartbeat: monitor ping 0x%08x (%s)\n",
			    word, word == VFB_CSUCODE_PING ?
			    "parked, healthy" : "BAD");
		else
			db_printf("heartbeat: monitor ping timed out\n");
	}
}

static void
veritefb_db_suspend(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;

	if (sc == NULL) {
		db_printf("veritefb not attached\n");
		return;
	}
	db_printf("suspend2d: %d\n", veritefb_suspend2d(sc));
}

static void
veritefb_db_resume(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;

	if (sc == NULL) {
		db_printf("veritefb not attached\n");
		return;
	}
	db_printf("resume2d: %d\n", veritefb_resume2d(sc));
}

static void
veritefb_db_regs(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;
	const char *name;
	uint32_t val;
	int i;

	if (sc == NULL) {
		db_printf("veritefb not attached\n");
		return;
	}

	veritefb_risc_hold(sc);
	db_printf("register file snapshot (RISC left held; IR clobbered - "
	    "use veritereset to resume):\n");
	for (i = 0; i < 256; i++) {
		val = veritefb_risc_readrf(sc, i);
		name = veritefb_db_regname(i);
		db_printf("%%%-3d 0x%08x%s%s%s", i, val,
		    name ? " (" : "", name ? name : "", name ? ")" : "");
		db_printf((i & 3) == 3 ? "\n" : "  ");
	}
	vfb_write1(sc, VFB_STATEINDEX, VFB_STATEINDEX_PC);
	db_printf("PC 0x%08x\n", vfb_read4(sc, VFB_STATEDATA));
}

static void
veritefb_db_reset(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;

	if (sc == NULL || sc->sc_ucode == NULL) {
		db_printf("veritefb not attached or no microcode\n");
		return;
	}

	db_printf("reloading microcode and restarting RISC...\n");
	db_printf("%s\n", veritefb_risc_init(sc) ?
	    "handshake passed, acceleration restored" :
	    "bring-up FAILED, staying in software rendering");
}

static void
veritefb_db_fault(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;

	if (sc == NULL) {
		db_printf("veritefb not attached\n");
		return;
	}

	db_printf("deliberately sending an invalid command (trap slot), "
	    "run veritediag to observe, veritereset to recover\n");
	(void)veritefb_dma_drain(sc);
	vfb_fifo_write(sc, VFB_CMDW(0, VFB_CMD_BOGUS));
}

static void
veritefb_db_cont(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	struct veritefb_softc *sc = veritefb_ddb_sc;

	if (sc == NULL) {
		db_printf("veritefb not attached\n");
		return;
	}
	veritefb_risc_continue(sc);
	db_printf("RISC released\n");
}

static const struct db_command veritefb_db_commands[] = {
	{ DDB_ADD_CMD("veritediag", veritefb_db_diag, 0,
	    "veritefb: PC signature, FIFO gauges, ring, heartbeat",
	    NULL, NULL) },
	{ DDB_ADD_CMD("veriteregs", veritefb_db_regs, 0,
	    "veritefb: RISC register file snapshot (leaves RISC held)",
	    NULL, NULL) },
	{ DDB_ADD_CMD("veritereset", veritefb_db_reset, 0,
	    "veritefb: reload microcode, restart RISC, re-handshake",
	    NULL, NULL) },
	{ DDB_ADD_CMD("veritefault", veritefb_db_fault, 0,
	    "veritefb: deliberately wedge the RISC (recovery test)",
	    NULL, NULL) },
	{ DDB_ADD_CMD("veritesuspend", veritefb_db_suspend, 0,
	    "veritefb: park the 2D microcode in the csucode monitor",
	    NULL, NULL) },
	{ DDB_ADD_CMD("veriteresume", veritefb_db_resume, 0,
	    "veritefb: resume the parked 2D microcode",
	    NULL, NULL) },
	{ DDB_ADD_CMD("veritecont", veritefb_db_cont, 0,
	    "veritefb: release the RISC hold bit",
	    NULL, NULL) },
	{ DDB_END_CMD },
};

static void
veritefb_ddb_attach(struct veritefb_softc *sc)
{
	if (veritefb_ddb_sc != NULL)
		return;
	veritefb_ddb_sc = sc;
	(void)db_register_tbl(DDB_BASE_CMD, veritefb_db_commands);
}
#endif /* DDB && VERITEFB_DEBUG */

