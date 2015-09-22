/*$Header: /cvsroot/src/sys/arch/vax/uba/qv.c,v 1.30.4.1 2015/09/22 12:05:53 skrll Exp $*/
/*
 * Copyright (c) 2015 Charles H. Dickman. All rights reserved.
 * Derived from smg.c
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
/*       1         2         3         4         5         6         7        */
/*3456789012345678901234567890123456789012345678901234567890123456789012345678*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$Header: /cvsroot/src/sys/arch/vax/uba/qv.c,v 1.30.4.1 2015/09/22 12:05:53 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/extent.h>		/***/
#include <sys/time.h>
#include <sys/bus.h>
#include <vax/include/pte.h>                /* temporary */
#include <machine/sid.h>
#include <dev/cons.h>
#include <dev/qbus/ubavar.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wsfb/genfbvar.h>
#include <vax/include/sgmap.h> /***/

#include "uba_common.h"		/***/
#include "qv.h"
#include "qv_ic.h"
#include "qvaux.h"
#include "opt_wsfont.h"

#define QMEMBASE        0x30000000      
#define QVSIZE          0x40000
#define QV_SCANMAP      0x3F800
#define QV_CURSRAM      0x3FFE0

#define QV_CSR          0
#define QV_CSR_1        (1 << 2)
#define QV_CSR_2        (1 << 3)
#define QV_CSR_BANK     (15 << 11)
#define QV_CUR_X        2
#define QV_CRTC_AR      8
#define QV_CRTC_DR      10
#define QV_IC           12
#define QV_ICDR         QV_ICDR
#define QV_ICSR         (QV_ICDR + 2)

/* Screen hardware defs */
#define QV_COLS		128	/* char width of screen */
#define QV_ROWS		57	/* rows of char on screen */
#define QV_CHEIGHT	15	/* lines a char consists of */
#define QV_NEXTROW	(QV_COLS * QV_CHEIGHT)
#define	QV_YWIDTH	864
#define QV_XWIDTH	1024

/* hardware cursor */
#define CUR_BLINKN      0x00
#define CUR_BLANK       0x20
#define CUR_BLINKS      0x40
#define CUR_BLINKF      0x60
#define CUR_BLINKM      0x60
#define CUR_OFF         CUR_BLANK
#define CUR_ON          CUR_BLINKS
#define CUR_START       10
#define CUR_END         11
#define CUR_HI          14
#define CUR_LO          15

//static	uint16_t curcmd, curx, cury, hotX, hotY;
static	int     bgmask, fgmask; 

static	int     qv_match(device_t, cfdata_t, void *);
static	void    qv_attach(device_t, device_t, void *);

static void	qv_cursor(void *, int, int, int);
static int	qv_mapchar(void *, int, unsigned int *);
static void	qv_putchar(void *, int, int, u_int, long);
static void	qv_copycols(void *, int, int, int,int);
static void	qv_erasecols(void *, int, int, int, long);
static void	qv_copyrows(void *, int, int, int);
static void	qv_eraserows(void *, int, int, long);
static int	qv_allocattr(void *, int, int, int, long *);

const struct wsdisplay_emulops qv_emulops = {
	.cursor = qv_cursor,
	.mapchar = qv_mapchar,
	.putchar = qv_putchar,
	.copycols = qv_copycols,
	.erasecols = qv_erasecols,
	.copyrows = qv_copyrows,
	.eraserows = qv_eraserows,
	.allocattr = qv_allocattr
};

struct _wsscreen_descr {
        const struct wsscreen_descr qv_stdscreen;       /* MUST BE FIRST */
        const uint16_t qv_crtc_param[16];
};

/*
 * Notes from the original Ultrix drivers
 *
 * Screen controller initialization parameters. The definations [sic] and use
 * of these parameters can be found in the Motorola 68045 [sic] crtc specs. In
 * essence they set the display parameters for the chip. The first set is
 * for the 15" screen and the second is for the 19" separate sync. There
 * is also a third set for a 19" composite sync monitor which we have not
 * tested and which is not supported.
 */

const struct _wsscreen_descr qv_stdscreen[] = {
        { { "80x30", 80, 30, &qv_emulops, 8, 
                QV_CHEIGHT, WSSCREEN_UNDERLINE|WSSCREEN_REVERSE },
            { 31, 25, 27, 0142, 31, 13, 30, 31, 4, 15, 040, 0, 0, 0, 0, 0 } },
        { { "120x55", 120, 55, &qv_emulops, 8, 
                QV_CHEIGHT, WSSCREEN_UNDERLINE|WSSCREEN_REVERSE },
            { 39, 30, 32, 0262, 55, 5, 54, 54, 4, 15, 040, 0, 0, 0, 0, 0 } },
        { { "128x57", QV_COLS, QV_ROWS, &qv_emulops, 8, 
                QV_CHEIGHT, WSSCREEN_UNDERLINE|WSSCREEN_REVERSE },
            { 39, 32, 33, 0264, 56, 5, 54, 54, 4, 15, 040, 0, 0, 0, 0, 0 } },
};

const struct wsscreen_descr *_qv_scrlist[] = {
	&qv_stdscreen[2].qv_stdscreen,	/* default */
	&qv_stdscreen[1].qv_stdscreen,
	&qv_stdscreen[0].qv_stdscreen, 
};

const struct wsscreen_list qv_screenlist = {
	.nscreens = __arraycount(_qv_scrlist),
	.screens = _qv_scrlist,
};

struct qv_softc {
        device_t sc_dev;                /* device pointer */
        bus_space_tag_t sc_iot;         /* register base */
        bus_space_handle_t sc_ioh;
        bus_space_tag_t sc_fbt;         /* frame buffer base */
        bus_space_handle_t sc_fbh;
        paddr_t sc_fbphys;              /* frame buffer phys addr */
        char *sc_fb;                    /* frame buffer virt addr */
	uint16_t *sc_scanmap;		/* scan map virt addr */
        char *sc_font;                  /* font glyph table */
        
        uint8_t sc_curon;               /* cursor on */
        uint16_t sc_curx;               /* cursor x position */
        uint16_t sc_cury;               /* cursor y position */
        uint16_t sc_curhotX;            /* cursor x hot spot */
        uint16_t sc_curhotY;            /* cursor y hot spot */

        struct qv_screen *sc_curscr;    /* current screen */
};

#if 0
struct genfb_qv_softc {
	struct genfb_softc      sc_gen;
	bus_space_tag_t         sc_iot;
	bus_space_handle_t	sc_ioh;
	uint32_t		sc_wstype;
};
#endif

static void     qv_crtc_wr(struct qv_softc *, uint16_t, uint16_t);
static void     qv_setcrtc(struct qv_softc *, const uint16_t *);

static int	qv_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	qv_mmap(void *, void *, off_t, int);
static int	qv_alloc_screen(void *, const struct wsscreen_descr *,
                         void **, int *, int *, long *);
static void	qv_free_screen(void *, void *);
static int	qv_show_screen(void *, void *, int,
				     void (*) (void *, int, int), void *);
//static void	qv_crsr_blink(void *);

int             qvauxprint(void *, const char *);

const struct wsdisplay_accessops qv_accessops = {
	.ioctl = qv_ioctl,
	.mmap = qv_mmap,
	.alloc_screen = qv_alloc_screen,
	.free_screen = qv_free_screen,
	.show_screen = qv_show_screen,
};

struct	qv_screen {
        struct qv_softc *ss_sc;
        const struct wsscreen_descr *ss_type;
	int	        ss_curx;
	int	        ss_cury;
	u_char	        ss_image[QV_ROWS][QV_COLS];	/* Image of screen */
	u_char	        ss_attr[QV_ROWS][QV_COLS];	/* Reversed etc... */
};

static	struct qv_screen qv_conscreen; /* XXX no console support */

static	callout_t qv_cursor_ch;

CFATTACH_DECL_NEW(qv, sizeof(struct qv_softc),
    qv_match, qv_attach, NULL, NULL);
#if 0
static int	genfb_match_qv(device_t, cfdata_t, void *);
static void	genfb_attach_qv(device_t, device_t, void *);
static int	genfb_ioctl_qv(void *, void *, u_long, void *, int, struct lwp*);
static paddr_t	genfb_mmap_qv(void *, void *, off_t, int);
static int      genfb_borrow_qv(void *, bus_addr_t, bus_space_handle_t *);

CFATTACH_DECL_NEW(genfb_qv, sizeof(struct genfb_qv_softc),
    genfb_match_qv, genfb_attach_qv, NULL, NULL);
#endif

/*
 * autoconf match function
 */
int
qv_match(device_t parent, cfdata_t match, void *aux)
{
        struct uba_attach_args *ua = aux;
        struct uba_softc *uh = device_private(parent);

        /* set up interrupts so the vector can be detected */

 	/* initialize qv interrupt controller */
 	qv_ic_init(ua, QV_IC);
 	
        /* set vertical retrace interrupt */ 	
        qv_ic_setvec(ua, QV_IC, QV_SYNC_VEC, uh->uh_lastiv - 4);
        qv_ic_enable(ua, QV_IC, QV_SYNC_VEC, QV_IC_ENA);
        qv_ic_arm(ua, QV_IC, QV_SYNC_VEC);

        /* enable interrupts */
	bus_space_write_2(ua->ua_iot, ua->ua_ioh, QV_CSR,
	        bus_space_read_2(ua->ua_iot, ua->ua_ioh, QV_CSR) | (1 << 6));

        qv_ic_force(ua, QV_IC, QV_SYNC_VEC);

	DELAY(20000);
	
        /* disable interrupts */
        qv_ic_enable(ua, QV_IC, QV_SYNC_VEC, QV_IC_DIS);

        return 1;
}

/* controller register write helper function */
static inline void
qv_reg_wr(struct qv_softc *sc, uint16_t addr, uint16_t data)
{
        bus_space_write_2(sc->sc_iot, sc->sc_ioh, addr, data);       
}

/* controller register read helper function */
static inline uint16_t
qv_reg_rd(struct qv_softc *sc, uint16_t addr)
{
        return bus_space_read_2(sc->sc_iot, sc->sc_ioh, addr);       
}

/*
 * write a 6845 CRT controller register
 */
static  void
qv_crtc_wr(struct qv_softc *sc, uint16_t addr, uint16_t data)
{
        qv_reg_wr(sc, QV_CRTC_AR, addr);
        qv_reg_wr(sc, QV_CRTC_DR, data);        
}

/*
 * write a set of a set of video timing parameters to the CRTC
 */
static void
qv_setcrtc(struct qv_softc *sc, const uint16_t *pp)
{
        int i;

        for (i = 0; i < 14; i++)
                qv_crtc_wr(sc, i, pp[i]);
}

static void inline
qv_ic_write(struct uba_attach_args *ua, bus_size_t offs, uint16_t value)
{
        bus_space_write_2(ua->ua_iot, ua->ua_ioh, offs, value);
}

void
qv_ic_init(struct uba_attach_args *ua, bus_size_t offs)
{
        static int initted;
        int i;
        
        if (!initted) {
		/* init the interrupt controller */
	        qv_ic_write(ua, QV_IC_SR + offs, QV_IC_RESET);  
		/* reset irr			 */
	        qv_ic_write(ua, QV_IC_SR + offs, QV_IC_CLRIRR); 
		/* specify individual vectors	 */
	        qv_ic_write(ua, QV_IC_SR + offs, QV_IC_MODE);   
		/* preset autoclear data	 */
	        qv_ic_write(ua, QV_IC_SR + offs, QV_IC_ACREG);  
		/* all setup as autoclear	 */
	        qv_ic_write(ua, QV_IC_DR + offs, 0xff);         
        
		/* clear all vector addresses */
                for (i = 0; i < 8; i++)                         
                        qv_ic_setvec(ua, offs, i, 0);
        
                initted = 1;
        }
}

void
qv_ic_setvec(struct uba_attach_args *ua, bus_size_t offs, int ic_vec, int vecnum)
{
	/* preset vector address	*/
	qv_ic_write(ua, QV_IC_SR + offs, QV_IC_RMEM | RMEM_BC_1 | ic_vec);      
	/* give it the vector number	*/
	qv_ic_write(ua, QV_IC_DR + offs, vecnum);	                        
}

void
qv_ic_enable(struct uba_attach_args *ua, bus_size_t offs, int ic_vec, int enable)
{
        if (enable)
		/* enable the interrupt */        
	        qv_ic_write(ua, QV_IC_SR + offs, QV_IC_CIMR | ic_vec);          
        else
		/* disable the interrupt */        
	        qv_ic_write(ua, QV_IC_SR + offs, QV_IC_SIMR | ic_vec);          
}

void
qv_ic_arm(struct uba_attach_args *ua, bus_size_t offs, int arm)
{
        if (arm) 
		/* arm the interrupt ctrl	*/
                qv_ic_write(ua, QV_IC_SR + offs, QV_IC_ARM);	                
        else
		/* disarm the interrupt ctrl	*/
                qv_ic_write(ua, QV_IC_SR + offs, QV_IC_DISARM);	                
}

void
qv_ic_force(struct uba_attach_args *ua, bus_size_t offs, int ic_vec)
{
	/* force an interrupt	*/
	qv_ic_write(ua, QV_IC_SR + offs, QV_IC_SIRR | ic_vec);	                
}

/*
 * print attachment message
 */
int
qvauxprint(void *aux, const char *pnp)
{
        if (pnp) {
                aprint_normal("qvaux at %s", pnp);
                return (UNCONF);
        }
        return 0;
}

/*
 * autoconf attach function
 */
void
qv_attach(device_t parent, device_t self, void *aux)
{
        struct qv_softc *sc = device_private(self);
        struct uba_softc *uh = device_private(parent);
        struct uba_attach_args *ua = aux;
        struct uba_attach_args aa;
	int fcookie;
        struct wsemuldisplaydev_attach_args emulaa;
//        struct wsdisplaydev_attach_args dispaa;
        struct wsdisplay_font *console_font;
	int line;

        sc->sc_dev = self;
        sc->sc_iot = ua->ua_iot;
        sc->sc_ioh = ua->ua_ioh;
        sc->sc_fbt = sc->sc_iot;
        sc->sc_fbphys = QMEMBASE + ((qv_reg_rd(sc, QV_CSR) & QV_CSR_BANK) << 7);
	if (bus_space_map(sc->sc_fbt, sc->sc_fbphys, QVSIZE, 
			BUS_SPACE_MAP_LINEAR, &sc->sc_fbh)) {
		aprint_error_dev(self, "Couldn't alloc graphics memory.\n");
		return;
	}
	
	aprint_normal(": fb %8lo", sc->sc_fbphys & 0x3fffff);
	sc->sc_fb = bus_space_vaddr(sc->sc_fbt, sc->sc_fbh);
	sc->sc_scanmap = (uint16_t *)&sc->sc_fb[QV_SCANMAP];
#if 0
	if (extent_alloc_region(((struct uba_vsoftc*)uh)->uv_sgmap.aps_ex,
			sc->sc_fbphys & 0x3fffff, QVSIZE, EX_NOWAIT)) {
		aprint_error_dev(self, 
			"Couldn't alloc graphics memory in sgmap.\n");
		return;
	}
#endif
	//aprint_normal(": fb 0x%08lx", sc->sc_fbphys & 0x3fffff);

	bzero(sc->sc_fb, QVSIZE);
	
	for (line = 0; line < QV_YWIDTH; line++) {
	        sc->sc_scanmap[line] = line;
	}

        /* program crtc */
        qv_setcrtc(sc, qv_stdscreen[2].qv_crtc_param);
                
        /* enable video output */
        qv_reg_wr(sc, QV_CSR, qv_reg_rd(sc, QV_CSR) | (1 << 2) | (1 << 3));
#if 0
	if (sc->sc_curscr == NULL)
		callout_init(&qv_cursor_ch, 0);
	sc->sc_curscr = &qv_conscreen;

	callout_reset(&qv_cursor_ch, hz / 2, qv_crsr_blink, sc);
#endif
        /* interrupt handlers - XXX */

        uh->uh_lastiv -= 4;

        wsfont_init();
	if ((fcookie = wsfont_find(NULL, 8, 15, 0, WSDISPLAY_FONTORDER_R2L,
	    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP)) < 0) {
		aprint_error_dev(self, "could not find 8x15 font\n");
		return;
	}
	if (wsfont_lock(fcookie, &console_font) != 0) {
		aprint_error_dev(self, "could not lock 8x15 font\n");
		return;
	}
	sc->sc_font = console_font->data;

        aprint_normal("\n");

	aa.ua_iot = ua->ua_iot;
	aa.ua_ioh = ua->ua_ioh + 32; // offset
	aa.ua_cvec = ua->ua_cvec - 4;
	if (config_search_ia(NULL, self, "qv", &aa) != NULL) {
	        config_found_ia(self, "qv", &aa, qvauxprint);
                uh->uh_lastiv -= 4;
	}

	emulaa.console = 0; /* Not console */ 
	emulaa.scrdata = &qv_screenlist;
	emulaa.accessops = &qv_accessops;
	emulaa.accesscookie = self;
	if (config_search_ia(NULL, self, "wsemuldisplaydev", &emulaa) != NULL) {
	        config_found_ia(self, "wsemuldisplaydev", &emulaa,
	            wsemuldisplaydevprint);
	}

        //console_debugger();
	return;
}

/* QVSS frame buffer */

/*      uint_32 is stored little endian in frame buffer */
/*      bits are stored little endian in frame buffer   */

/*      uint_32 *fb;                                     */
/*      fb = (int *)phystova(0x303c0000);                */
/*      *fb = 0x00000001; */ /* sets bit in first column */

/* Frame Buffer Usage */

/* characters are 8 bits wide and QVHEIGHT high */
/* the scan map is allocated in terms of character height, */
/* so a pointer to the top line of a character can step to the */
/* next row without looking up the memory location in the scan map */

static	char *cursor;
static	int cur_on;

/*
 * return pointer to line in character glyph
 */
static inline char *
qv_font(struct qv_softc *sc, int c, int line)
{
        /* map char to font table offset */
        if (c < 32)
                c = 32;
        else if (c > 127)
                c -= 66;
        else
                c -= 32;
        
        /* return pointer line in font glyph */        
        return &sc->sc_font[c*QV_CHEIGHT + line];        
}

/*
 * return pointer to character line in frame buffer
 */
static inline char *
qv_fbp(struct qv_softc *sc, int row, int col, int line)
{
        return &sc->sc_fb[col + sc->sc_scanmap[row*QV_CHEIGHT + line]*QV_COLS];
}

/*
 * callout callback function to blink cursor
 */
#if 0
static void
qv_crsr_blink(void *arg)
{
        struct qv_softc *sc = arg;
        
	if (cur_on)
		*cursor ^= 255;
	callout_reset(&qv_cursor_ch, hz / 2, qv_crsr_blink, sc);
}
#endif
/*
 * emulop cursor
 */
void
qv_cursor(void *id, int on, int row, int col)
{
	struct qv_screen * const ss = id;

	if (ss == ss->ss_sc->sc_curscr) {
		*qv_fbp(ss->ss_sc, ss->ss_cury, ss->ss_curx, 14)
		    = *qv_font(ss->ss_sc, 
		        ss->ss_image[ss->ss_cury][ss->ss_curx], 14);
		cursor = qv_fbp(ss->ss_sc, row, col, 14);
		if ((cur_on = on))
			*cursor ^= 255;
	}
	ss->ss_curx = col;
	ss->ss_cury = row;
}

/*
 * emulop mapchar
 */
int
qv_mapchar(void *id, int uni, unsigned int *index)
{
	if (uni < 256) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

/*
 * emulop putchar
 */
static void
qv_putchar(void *id, int row, int col, u_int c, long attr)
{
	struct qv_screen * const ss = id;
	int i;
        char *gp;
        char *fp;
        char rvid;
        
	c &= 0xff;

	ss->ss_image[row][col] = c;
	ss->ss_attr[row][col] = attr;
	if (ss != ss->ss_sc->sc_curscr)
		return;

        gp = qv_font(ss->ss_sc, c, 0);
        fp = qv_fbp(ss->ss_sc, row, col, 0);
        rvid = (attr & WSATTR_REVERSE) ? 0xff : 0x00;
        for (i = 0; i < QV_CHEIGHT; i++) {
                *fp = *gp++ ^ rvid;
                fp += QV_COLS;
        }

	if (attr & WSATTR_UNDERLINE)
	        *qv_fbp(ss->ss_sc, row, col, 14) 
	            ^= *qv_fbp(ss->ss_sc, row, col, 14);
}

/*
 * emulop copy columns - copies columns inside a row
 */
static void
qv_copycols(void *id, int row, int srccol, int dstcol, int ncols)
{
	struct qv_screen * const ss = id;
	int i;

	memcpy(&ss->ss_image[row][dstcol], &ss->ss_image[row][srccol], ncols);
	memcpy(&ss->ss_attr[row][dstcol], &ss->ss_attr[row][srccol], ncols);
	if (ss != ss->ss_sc->sc_curscr)
		return;
	for (i = 0; i < QV_CHEIGHT; i++)
	        memcpy(qv_fbp(ss->ss_sc, row, dstcol, i),
	            qv_fbp(ss->ss_sc, row, srccol, i), ncols);
}

/*
 * emulop erase columns - erases a bunch of chars inside one row
 */
static void
qv_erasecols(void *id, int row, int startcol, int ncols, long fillattr)
{
	struct qv_screen * const ss = id;
	int i;

	memset(&ss->ss_image[row][startcol], 0, ncols);
	memset(&ss->ss_attr[row][startcol], 0, ncols);
	if (ss != ss->ss_sc->sc_curscr)
		return;
	for (i = 0; i < QV_CHEIGHT; i++)
	        memset(qv_fbp(ss->ss_sc, row, startcol, i), 0, ncols);
}

/*
 * overlap check
 * return 0 if no overlap
 *        -1 if overlap and dst is less than src (move up)
 * 	  +1 if overlap and src is less than dst (move down)
 */
static inline int
qv_rows_overlap(int srcrow, int dstrow, int nrows)
{
	if (dstrow < srcrow) {
		if (dstrow + nrows <= srcrow)
			return 0;
		else
			return -1;
	}
	else {
		if (srcrow + nrows <= dstrow)
			return 0;
		else
			return 1;
	}
}

/*
 * emulop copyrows - copy entire rows
 */
static void
qv_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	struct qv_screen * const ss = id;
	int ol;
	int n;
	int line;
	int tmp;
	uint16_t *sp;
	uint16_t *dp;

	memcpy(&ss->ss_image[dstrow][0], &ss->ss_image[srcrow][0],
	    nrows * QV_COLS);
	memcpy(&ss->ss_attr[dstrow][0], &ss->ss_attr[srcrow][0],
	    nrows * QV_COLS);
	if (ss != ss->ss_sc->sc_curscr)
		return;
	
	ol = qv_rows_overlap(srcrow, dstrow, nrows);
	if (ol == 0)
		for (n = 0; n < nrows; n++)
			bcopy(qv_fbp(ss->ss_sc, srcrow + n, 0, 0),
	    		    qv_fbp(ss->ss_sc, dstrow + n, 0, 0), QV_NEXTROW);
	else if (ol < 0) {
	 	for (n = 0; n < nrows; n++) {
			dp = &ss->ss_sc->sc_scanmap[(dstrow + n)*QV_CHEIGHT];  
			sp = &ss->ss_sc->sc_scanmap[(srcrow + n)*QV_CHEIGHT];
			for (line = 0; line < QV_CHEIGHT; line++) {
				tmp = *dp; 
				*dp = *sp;
				*sp = tmp;
				dp++;
				sp++;
			}
		}	
		qv_copyrows(id, dstrow + nrows - srcrow + dstrow, 
		    dstrow + nrows, srcrow - dstrow);
	}
	else {
	 	for (n = nrows - 1; n >= 0; n--) {
			dp = &ss->ss_sc->sc_scanmap[(dstrow + n)*QV_CHEIGHT];  
			sp = &ss->ss_sc->sc_scanmap[(srcrow + n)*QV_CHEIGHT];
			for (line = 0; line < QV_CHEIGHT; line++) {
				tmp = *dp; 
				*dp = *sp;
				*sp = tmp;
				dp++;
				sp++;
			}
		}	
		qv_copyrows(id, srcrow, dstrow, dstrow - srcrow);
	}
}

/*
 * emulop eraserows - erase a number of entire rows
 */
static void
qv_eraserows(void *id, int startrow, int nrows, long fillattr)
{
	struct qv_screen * const ss = id;
	int row;

	memset(&ss->ss_image[startrow][0], 0, nrows * QV_COLS);
	memset(&ss->ss_attr[startrow][0], 0, nrows * QV_COLS);
	if (ss != ss->ss_sc->sc_curscr)
		return;
        
	for (row = startrow; row < startrow + nrows; row++) {
	        memset(qv_fbp(ss->ss_sc, row, 0, 0), 0, QV_NEXTROW);
	}
}

/*
 * emulop allocattr
 */
static int
qv_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{
	*attrp = flags;
	return 0;
}

/*
 * emulop setcursor
 */
static void
qv_setcursor(struct qv_softc *sc, struct wsdisplay_cursor *v)
{
	uint16_t red, green, blue;
	uint32_t curfg[16], curmask[16];
	uint16_t *curp;
	int i;

	/* Enable cursor */
	if (v->which & WSDISPLAY_CURSOR_DOCUR) {
	        sc->sc_curon = (v->enable) ? CUR_ON : CUR_OFF;
	        qv_crtc_wr(sc, CUR_START, sc->sc_curon | (sc->sc_cury & 0x0f));
	}
	if (v->which & WSDISPLAY_CURSOR_DOHOT) {
		sc->sc_curhotX = v->hot.x;
		sc->sc_curhotY = v->hot.y;
	}
	if (v->which & WSDISPLAY_CURSOR_DOCMAP) {
		/* First background */
		red = fusword(v->cmap.red);
		green = fusword(v->cmap.green);
		blue = fusword(v->cmap.blue);
		bgmask = (((30L * red + 59L * green + 11L * blue) >> 8) >=
		    (((1<<8)-1)*50)) ? ~0 : 0;
		red = fusword(v->cmap.red+2);
		green = fusword(v->cmap.green+2);
		blue = fusword(v->cmap.blue+2);
		fgmask = (((30L * red + 59L * green + 11L * blue) >> 8) >=
		    (((1<<8)-1)*50)) ? ~0 : 0;
	}
	if (v->which & WSDISPLAY_CURSOR_DOSHAPE) {
		copyin(v->image, curfg, sizeof(curfg));
		copyin(v->mask, curmask, sizeof(curmask));      /* not used */
	        curp = (uint16_t *) &(sc->sc_fb)[QV_CURSRAM];
		for (i = 0; i < sizeof(curfg)/sizeof(curfg[0]); i++) {
		        curp[i] = (uint16_t)curfg[i];
		}
	}
}

/*
 * emulop ioctl
 */
int
qv_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wsdisplay_fbinfo *fb = (void *)data;
	//static uint16_t curc;
        struct qv_softc *sc = device_private(v);

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VAX_MONO;
		break;

	case WSDISPLAYIO_GINFO:
		fb->height = QV_YWIDTH;
		fb->width = QV_XWIDTH;
		fb->depth = 1;
		fb->cmsize = 2;
		break;

	case WSDISPLAYIO_SVIDEO:
		if (*(u_int *)data == WSDISPLAYIO_VIDEO_ON) {
                        /* enable video output */
  		        qv_reg_wr(sc, QV_CSR, 
  		            qv_reg_rd(sc, QV_CSR) | (1 << 2));
		} else {
                        /* disable video output */
                        qv_reg_wr(sc, QV_CSR, 
                            qv_reg_rd(sc, QV_CSR) & ~(1 << 2));
		}
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = (qv_reg_rd(sc, QV_CSR) & (1 << 2))
		    ? WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		break;

	case WSDISPLAYIO_SCURSOR:
		qv_setcursor(sc, (struct wsdisplay_cursor *)data);
		break;

	case WSDISPLAYIO_SCURPOS:
	        sc->sc_curx = ((struct wsdisplay_curpos *)data)->x;
		sc->sc_cury = ((struct wsdisplay_curpos *)data)->y;
                qv_crtc_wr(sc, CUR_START, CUR_OFF | (sc->sc_cury & 0x0f));
                qv_crtc_wr(sc, CUR_HI, sc->sc_cury >> 4);
                qv_reg_wr(sc, QV_CUR_X, sc->sc_curx);
                qv_crtc_wr(sc, CUR_START, 
                    sc->sc_curon | (sc->sc_cury & 0x0f));		
		break;

	case WSDISPLAYIO_GCURPOS:
		((struct wsdisplay_curpos *)data)->x = sc->sc_curx;
		((struct wsdisplay_curpos *)data)->y = sc->sc_cury;
		break;

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x = 16;
		((struct wsdisplay_curpos *)data)->y = 16;
		break;

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

/*
 * emulop mmap
 */
static paddr_t
qv_mmap(void *v, void *vs, off_t offset, int prot)
{
        struct qv_softc *sc = device_private(v);
        
	if (offset >= QVSIZE || offset < 0)
		return -1;
	return (sc->sc_fbphys) >> PGSHIFT;
}

/*
 * emulop allocate screen
 */
int
qv_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
        struct qv_softc *sc = device_private(v);
        struct qv_screen *ss;

	ss = malloc(sizeof(struct qv_screen), M_DEVBUF, M_WAITOK|M_ZERO);
	ss->ss_sc = sc;
	ss->ss_type = type;
	*cookiep = ss;
	*curxp = *curyp = *defattrp = 0;
        printf("qv_alloc_screen: \"%s\" %p\n", type->name, ss);
	return 0;
}

/*
 * emulop free screen
 */
void
qv_free_screen(void *v, void *cookie)
{
        printf("qv_free_screen: %p\n", cookie);
        free(cookie, M_DEVBUF);
}

/*
 * emulop show screen
 */
int
qv_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct qv_screen *ss = cookie;
	const struct _wsscreen_descr *descr;
	int row, col, line;
        printf("qv_show_screen: %p\n", cookie);

	if (ss == ss->ss_sc->sc_curscr)
		return (0);

        descr = (const struct _wsscreen_descr *)(ss->ss_type);
        qv_setcrtc(ss->ss_sc, descr->qv_crtc_param);
	for (row = 0; row < QV_ROWS; row++)
		for (line = 0; line < QV_CHEIGHT; line++) {
			for (col = 0; col < QV_COLS; col++) {
				u_char s, c = ss->ss_image[row][col];

				if (c < 32)
					c = 32;
				s = *qv_font(ss->ss_sc, c, line);
				if (ss->ss_attr[row][col] & WSATTR_REVERSE)
					s ^= 255;
				*qv_fbp(ss->ss_sc, row, col, line) = s;

				if (ss->ss_attr[row][col] & WSATTR_UNDERLINE)
					*qv_fbp(ss->ss_sc, row, col, line)
					    = 255;
			}
		}
        cursor = qv_fbp(ss->ss_sc, ss->ss_cury, ss->ss_curx, 14);
	ss->ss_sc->sc_curscr = ss;
	return (0);
}

#if 0
void
qv_reset_establish(void (*reset)(device_t), device_t dev)
{
        uba_reset_establish(reset, device_parent(dev));
}
#endif
cons_decl(qv);

void
qvcninit(struct consdev *cndev)
{
	int fcookie;
	struct wsdisplay_font *console_font;
	extern void lkccninit(struct consdev *);
	extern int lkccngetc(dev_t);
	extern int dz_vsbus_lk201_cnattach(int);

        //printf("qvcninit: \n");
	/* Clear screen */
	//memset(qv_addr, 0, 128*864);

	callout_init(&qv_cursor_ch, 0);
	//curscr = &qv_conscreen;
	wsdisplay_cnattach(&qv_stdscreen[0].qv_stdscreen, 
	    &qv_conscreen, 0, 0, 0);
	cn_tab->cn_pri = CN_INTERNAL;
	wsfont_init();
	if ((fcookie = wsfont_find(NULL, 8, 15, 0, WSDISPLAY_FONTORDER_R2L,
	    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP)) < 0)
	{
		printf("qv: could not find 8x15 font\n");
		return;
	}
	if (wsfont_lock(fcookie, &console_font) != 0) {
		printf("qv: could not lock 8x15 font\n");
		return;
	}
	//qf = console_font->data;

#if NQVKBD > 0 && 0
	qvkbd_cnattach(0); /* Connect keyboard and screen together */
#endif
}

/*
 * Called very early to setup the glass tty as console.
 * Because it's called before the VM system is inited, virtual memory
 * for the framebuffer can be stolen directly without disturbing anything.
 */
void
qvcnprobe(struct consdev *cndev)
{
        printf("qvcnprobe: \n");
#if 0
	extern vaddr_t virtual_avail;
	extern const struct cdevsw wsdisplay_cdevsw;

	switch (vax_boardtype) {
	case VAX_BTYP_410:
	case VAX_BTYP_420:
	case VAX_BTYP_43:
		if ((vax_confdata & KA420_CFG_L3CON) ||
		    (vax_confdata & KA420_CFG_MULTU))
			break; /* doesn't use graphics console */
		qv_addr = (void *)virtual_avail;
		virtual_avail += QVSIZE;
		ioaccess((vaddr_t)qv_addr, QVADDR, (QVSIZE/VAX_NBPG));
		cndev->cn_pri = CN_INTERNAL;
		cndev->cn_dev = makedev(cdevsw_lookup_major(&wsdisplay_cdevsw),
					0);
		break;
	default:
		break;
	}
#endif 
}
