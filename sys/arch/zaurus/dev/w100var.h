/* $NetBSD: w100var.h,v 1.1.6.2 2012/04/17 00:07:13 yamt Exp $ */

#ifndef _ZAURUS_DEV_W100VAR_H_
#define _ZAURUS_DEV_W100VAR_H_

#include <dev/rasops/rasops.h>
#include <sys/bus.h>

struct w100_screen {
	LIST_ENTRY(w100_screen) link;

	/* Frame buffer */
	void 	*buf_va;
	int     depth;

	/* rasterop */
	struct rasops_info rinfo;
};

struct w100_softc {
	device_t	dev;
	/* control register */
	bus_space_tag_t	iot;
	bus_space_handle_t	ioh_cfg; /* config */
	bus_space_handle_t	ioh_reg; /* register */
	bus_space_handle_t      ioh_vram; /* video memory */

	const struct w100_panel_geometry *geometry;

	short display_width;
	short display_height;

	int n_screens;
	LIST_HEAD(, w100_screen) screens;
	struct w100_screen *active;
};

struct w100_panel_geometry {
	short panel_width;
	short panel_height;

	short rotate;
#define W100_PANEL_ROTATE_CW	1	/* quarter clockwise */
#define W100_PANEL_ROTATE_CCW	2	/* quarter counter-clockwise */
#define W100_PANEL_ROTATE_UD	3	/* upside-down */
};

struct w100_wsscreen_descr {
	struct wsscreen_descr  c;	/* standard descriptor */
	int depth;			/* bits per pixel */
	int flags;			/* rasops flags */
};

#define W100_CFG_OFFSET     (0x00000000)
#define W100_REG_OFFSET     (0x00010000)
#define W100_INTMEM_OFFSET  (0x00100000)
#define W100_EXTMEM_OFFSET  (0x00800000)

#define W100_BASE_ADDRESS   (0x08000000)
#define W100_CFG_ADDRESS    (W100_BASE_ADDRESS + W100_CFG_OFFSET)
#define W100_REG_ADDRESS    (W100_BASE_ADDRESS + W100_REG_OFFSET)
#define W100_INTMEM_ADDRESS (W100_BASE_ADDRESS + W100_INTMEM_OFFSET)
#define W100_EXTMEM_ADDRESS (W100_BASE_ADDRESS + W100_EXTMEM_OFFSET)

#define W100_CFG_SIZE       (0x00000010)
#define W100_REG_SIZE       (0x00002000)
#define W100_INTMEM_SIZE    (0x00060000)
#define W100_EXTMEM_SIZE    (0x00160000)

void    w100_attach_subr(struct w100_softc *, bus_space_tag_t,
             const struct w100_panel_geometry *);
int     w100_cnattach(struct w100_wsscreen_descr *,
             const struct w100_panel_geometry *);
void    w100_suspend(struct w100_softc *);
void    w100_resume(struct w100_softc *);
void    w100_power(int, void *);
int     w100_show_screen(void *, void *, int, void (*)(void *, int, int),
             void *);
int     w100_alloc_screen(void *, const struct wsscreen_descr *, void **,
             int *, int *, long *);
void    w100_free_screen(void *, void *);
int     w100_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t w100_mmap(void *, void *, off_t, int);

extern const struct wsdisplay_emulops w100_emulops;


#endif /* _ZAURUS_DEV_W100VAR_H_ */
