/*	$NetBSD: vidcrender.c,v 1.11 2003/04/01 15:03:00 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe
 * Copyright (c) 1996 Robert Black
 * Copyright (c) 1994-1995 Melvyn Tang-Richardson
 * Copyright (c) 1994-1995 RiscBSD kernel team
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
 *	This product includes software developed by the RiscBSD kernel team
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RISCBSD TEAM ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * vidcrender.c
 *
 * Console assembly functions
 *
 * Created      : 17/09/94
 * Last updated : 07/02/96
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syslog.h>
#include <sys/resourcevar.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/param.h>
#include <arm/arm32/katelib.h>
#include <machine/bootconfig.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <arm/iomd/vidc.h>
#include <arm/iomd/console/console.h>
#include <machine/vconsole.h>

#include <arm/iomd/console/fonts/font_normal.h>
#include <arm/iomd/console/fonts/font_bold.h>
#include <arm/iomd/console/fonts/font_italic.h>

#ifndef DEBUGTERM
#define dprintf(x)	;
#endif

/* Options */
#define ACTIVITY_WARNING
#define SCREENBLANKER
#undef INVERSE_CONSOLE

/* Internal defines only */
#define DEVLOPING
#undef SELFTEST
#undef SILLIES

#ifdef SILLIES
#define PRETTYCURSOR
#endif

extern struct vconsole *vconsole_default;

extern videomemory_t videomemory;

extern font_struct font_terminal_14normal;
extern font_struct font_terminal_14bold;
extern font_struct font_terminal_14italic;

#define font_normal	font_terminal_14normal
#define font_bold	font_terminal_14bold
#define font_italic	font_terminal_14italic

#define VIDC_ENGINE_NAME "VIDC"
#define R_DATA ((struct vidc_info *)vc->r_data)
#define MODE (R_DATA->mode)

static int cold_init = 0;

extern struct vconsole *vconsole_master;
extern struct vconsole *vconsole_current;
static struct vidc_mode vidc_initialmode;
static struct vidc_mode *vidc_currentmode;

unsigned int dispstart;
unsigned int dispsize;
unsigned int dispbase;
unsigned int dispend;
unsigned int ptov;
unsigned int transfersize;
unsigned int vmem_base;
unsigned int phys_base;
int	flash;
int	cursor_flash;
char *cursor_normal;
char *cursor_transparent;
int p_cursor_normal;
int p_cursor_transparent;

/* Local function prototypes */
static int	vidcrender_coldinit(struct vconsole *);
static void	vidcrender_cls(struct vconsole *);
static int	vidc_cursor_init(struct vconsole *);
int		vidcrender_cursorintr(void *);
int		vidcrender_flashintr(void *);
static int	vidcrender_textpalette(struct vconsole *);
static void	vidcrender_render(struct vconsole *, char);
static void	vidcrender_mode(struct vconsole *, struct vidc_mode *);
int		vidcrender_init(struct vconsole *);
int		vidcrender_flash(struct vconsole *, int);
int		vidcrender_cursorflash(struct vconsole *, int);
int		vidcrender_flash_go(struct vconsole *);
int		vidcrender_blank(struct vconsole *, int);
void		vidcrender_reinit(void);
void 		vidcrender_putchar(dev_t, char, struct vconsole *);
int		vidcrender_spawn(struct vconsole *);
int		vidcrender_redraw(struct vconsole *, int, int, int, int);
int		vidcrender_swapin(struct vconsole *);
paddr_t		vidcrender_mmap(struct vconsole *, off_t, int);
void		vidcrender_scrollup(struct vconsole *, int, int);
void		vidcrender_scrolldown(struct vconsole *, int, int);
int		vidcrender_scrollback(struct vconsole *);
void		vidcrender_update(struct vconsole *);
int		vidcrender_scrollforward(struct vconsole *);
int		vidcrender_scrollbackend(struct vconsole *);
int		vidcrender_clreos(struct vconsole *, int);
int		vidcrender_debugprint(struct vconsole *);
static int	vidcrender_cursorupdate(struct vconsole *);
static int	vidcrender_cursorflashrate(struct vconsole *, int);
int		vidcrender_setfgcol(struct vconsole *, int);
int		vidcrender_setbgcol(struct vconsole *, int);
int		vidcrender_sgr(struct vconsole *, int);
int		vidcrender_scrollregion(struct vconsole *, int, int);
int		vidcrender_ioctl(struct vconsole *, dev_t, int, caddr_t, int,
				 struct proc *);
int		vidcrender_attach(struct vconsole *, struct device *,
				  struct device *, void *);

extern int	vidcrendermc_cls(unsigned char *, unsigned char *, int);
extern void	vidcrendermc_render(unsigned char *, unsigned char *, int, int);

void		physcon_display_base(u_int);

/*
 * This will be called while still in the mode that we were left
 * in after exiting from riscos
 */

static irqhandler_t cursor_ih;
irqhandler_t flash_ih;

#ifndef HARDCODEDMODES
/* The table of modes is separately compiled */
extern struct vidc_mode vidcmodes[];
#else /* HARDCODEDMODES */

/* We have a hard compiled table of modes and a list of definable modes */
static struct vidc_mode vidcmodes[] = {
	{32500,/**/52,  64, 30, 640,  30, 14,/**/3, 28, 0, 480, 0, 9,/**/0,/**/3},
	{65000,/**/128, 36, 60, 1024, 60 ,36,/**/6, 29, 0, 768, 0, 3,/**/0,/**/3},
};
#endif /* HARDCODEDMODES */


struct fsyn {
	int r, v, f;
};

static struct fsyn fsyn_pref[] = {
	{ 6,   2,  8000 },
	{ 4,   2, 12000 },
	{ 3,   2, 16000 },
	{ 2,   2, 24000 },
	{ 41, 43, 25171 },
	{ 50, 59, 28320 },
	{ 3,   4, 32000 },
	{ 2,   3, 36000 },
	{ 31, 58, 44903 },
	{ 12, 35, 70000 },
	{ 0,   0, 00000 }
};


/*#define mod(x)	(((x) > 0) ? (x) : (-x))*/

static __inline int
mod(int n)
{
	if (n < 0)
		return(-n);
	else
		return(n);
}

static int
vidcrender_coldinit(vc)
	struct vconsole *vc;
{
	int found;
	int loop;

	/* Blank out the cursor */

	vidc_write(VIDC_CP1, 0x0);
	vidc_write(VIDC_CP2, 0x0);
	vidc_write(VIDC_CP3, 0x0);

	/* the old vidc-console code can't handle all display depths */
	bootconfig.log2_bpp = 3;

	/* Try to determine the current mode */
	vidc_initialmode.hder = bootconfig.width+1;
	vidc_initialmode.vder = bootconfig.height+1;
	vidc_initialmode.log2_bpp = bootconfig.log2_bpp;

	dispbase = vmem_base = dispstart = videomemory.vidm_vbase;
	phys_base = videomemory.vidm_pbase;

/* Nut - should be using videomemory.vidm_size - mark */

	if (videomemory.vidm_type == VIDEOMEM_TYPE_DRAM) {
		dispsize = videomemory.vidm_size;
		transfersize = 16;
	} else {
		dispsize = bootconfig.vram[0].pages * PAGE_SIZE;
		transfersize = dispsize >> 10;
	}
    
	ptov = dispbase - phys_base;

	dispend = dispstart+dispsize;
 
	vidc_currentmode = &vidcmodes[0];
	loop = 0;
	found = 0;
  
	while (vidcmodes[loop].pixel_rate != 0) {
  		if (vidcmodes[loop].hder == (bootconfig.width + 1)
  		    && vidcmodes[loop].vder == (bootconfig.height + 1)
		    && vidcmodes[loop].frame_rate == bootconfig.framerate) {
			vidc_currentmode = &vidcmodes[loop];
			found = 1;
		}
		++loop;
	}

	if (!found) {
		vidc_currentmode = &vidcmodes[0];
		loop = 0;
		found = 0;

		while (vidcmodes[loop].pixel_rate != 0) {
			if (vidcmodes[loop].hder == (bootconfig.width + 1)
 			    && vidcmodes[loop].vder == (bootconfig.height + 1)) {
 				vidc_currentmode = &vidcmodes[loop];
 				found = 1;
 			}
 			++loop;
 		}
	}

	/* vidc_currentmode = &vidcmodes[0]; */
	vidc_currentmode->log2_bpp = bootconfig.log2_bpp;

	if (vc)
		R_DATA->flash = R_DATA->cursor_flash = 0;

	dispstart = dispbase;
	dispend = dispstart+dispsize;
    
	IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart-ptov);
	IOMD_WRITE_WORD(IOMD_VIDSTART, dispstart-ptov);
	IOMD_WRITE_WORD(IOMD_VIDEND, (dispend-transfersize)-ptov);
	return 0;
}

struct vidc_mode newmode;

static const int bpp_mask_table[] = {
	0,  /* 1bpp */
	1,  /* 2bpp */
	2,  /* 4bpp */
	3,  /* 8bpp */
	4,  /* 16bpp */
	6   /* 32bpp */
};

void
vidcrender_mode(vc, mode)
	struct vconsole *vc;
	struct vidc_mode *mode;
{    
	register int acc;
	int bpp_mask;
        int ereg;

	int best_r, best_v, best_match;

/*
 * Find out what bit mask we need to or with the vidc20 control register
 * in order to generate the desired number of bits per pixel.
 * log2_bpp is log base 2 of the number of bits per pixel.
 */

	bpp_mask = bpp_mask_table[mode->log2_bpp];

/*
	printf ( "res = (%d, %d) rate = %d\n", mode->hder, mode->vder, mode->pixel_rate );
*/

	newmode = *mode;
	vidc_currentmode = &newmode;

    /* Program the VCO Look up a preferred value before choosing one */
	{
		int least_error = mod(fsyn_pref[0].f - vidc_currentmode->pixel_rate);
		int counter;
		best_r = fsyn_pref[0].r;
		best_match = fsyn_pref[0].f;
		best_v = fsyn_pref[0].v;
        
	/* Look up */
        
		counter=0;
        
		while (fsyn_pref[counter].r != 0) {
			if (least_error > mod(fsyn_pref[counter].f - vidc_currentmode->pixel_rate)) {
				best_match = fsyn_pref[counter].f;
				least_error = mod(fsyn_pref[counter].f - vidc_currentmode->pixel_rate);
				best_r = fsyn_pref[counter].r;
				best_v = fsyn_pref[counter].v;
			}
			counter++;
		}

		if (least_error > 0) { /* Accuracy of 1000Hz */
			int r, v, f;
			for (v = 63; v > 0; v--)
				for (r = 63; r > 0; r--) {
					f = ((v * vidc_fref)/1000) / r;
					if (least_error >= mod(f - vidc_currentmode->pixel_rate)) {
						best_match = f;
						least_error = mod(f - vidc_currentmode->pixel_rate);
						best_r = r;
						best_v = v;
					}
				}
		}
    
		if (best_r > 63) best_r=63;
		if (best_v > 63) best_v=63;
		if (best_r < 1)  best_r= 1;
		if (best_v < 1)  best_v= 1;
    
	}
/*
	printf ( "best_v = %d  best_r = %d best_f = %d\n", best_v, best_r, best_match );
*/

	if (vc==vconsole_current) {
		vidc_write(VIDC_FSYNREG, (best_v-1)<<8 | (best_r-1)<<0);

		acc=0;
		acc+=vidc_currentmode->hswr;	vidc_write(VIDC_HSWR, (acc - 8 ) & (~1));
		acc+=vidc_currentmode->hbsr;	vidc_write(VIDC_HBSR, (acc - 12) & (~1));
		acc+=vidc_currentmode->hdsr;	vidc_write(VIDC_HDSR, (acc - 18) & (~1));
		acc+=vidc_currentmode->hder;	vidc_write(VIDC_HDER, (acc - 18) & (~1));
		acc+=vidc_currentmode->hber;	vidc_write(VIDC_HBER, (acc - 12) & (~1));
		acc+=vidc_currentmode->hcr;	vidc_write(VIDC_HCR,  (acc - 8 ) & (~3));

		acc=0;
		acc+=vidc_currentmode->vswr;	vidc_write(VIDC_VSWR, (acc - 1));
		acc+=vidc_currentmode->vbsr;	vidc_write(VIDC_VBSR, (acc - 1));
		acc+=vidc_currentmode->vdsr;	vidc_write(VIDC_VDSR, (acc - 1));
		acc+=vidc_currentmode->vder;	vidc_write(VIDC_VDER, (acc - 1));
		acc+=vidc_currentmode->vber;	vidc_write(VIDC_VBER, (acc - 1));
		acc+=vidc_currentmode->vcr;	vidc_write(VIDC_VCR,  (acc - 1));

		IOMD_WRITE_WORD(IOMD_FSIZE, vidc_currentmode->vcr
		    + vidc_currentmode->vswr
		    + vidc_currentmode->vber
		    + vidc_currentmode->vbsr - 1);

		if (dispsize <= 1024*1024)
			vidc_write(VIDC_DCTL, vidc_currentmode->hder>>2 | 1<<16 | 1<<12);
		else
			vidc_write(VIDC_DCTL, vidc_currentmode->hder>>2 | 3<<16 | 1<<12);

		ereg = 1<<12;
		if (vidc_currentmode->sync_pol & 0x01)
			ereg |= 1<<16;
		if (vidc_currentmode->sync_pol & 0x02)
			ereg |= 1<<18;
		vidc_write(VIDC_EREG, ereg);
		if (dispsize > 1024*1024) {
			if (vidc_currentmode->hder >= 800)
 				vidc_write(VIDC_CONREG, 7<<8 | bpp_mask<<5);
			else
				vidc_write(VIDC_CONREG, 6<<8 | bpp_mask<<5);
		} else {
			vidc_write(VIDC_CONREG, 7<<8 | bpp_mask<<5);
		}
	}

	R_DATA->mode = *vidc_currentmode;
	R_DATA->screensize = R_DATA->XRES * R_DATA->YRES * (1 << R_DATA->mode.log2_bpp)/8;
	R_DATA->pixelsperbyte = 8 / (1 << R_DATA->mode.log2_bpp);
	R_DATA->frontporch = MODE.hswr + MODE.hbsr + MODE.hdsr;
	R_DATA->topporch = MODE.vswr + MODE.vbsr + MODE.vdsr;
	R_DATA->bytes_per_line = R_DATA->XRES *
	    R_DATA->font->y_spacing/R_DATA->pixelsperbyte;
	R_DATA->bytes_per_scroll = (vc->ycur % R_DATA->font->y_spacing)
	    * vc->xcur / R_DATA->pixelsperbyte;
	R_DATA->text_width = R_DATA->XRES / R_DATA->font->x_spacing;
	R_DATA->text_height = R_DATA->YRES / R_DATA->font->y_spacing;
	vc->xchars = R_DATA->text_width;
	vc->ychars = R_DATA->text_height;
}

void
physcon_display_base(base)
	u_int base;
{
	dispstart = dispstart-dispbase + base;
	dispbase = vmem_base = base;
	dispend = base + dispsize;
	ptov = dispbase - phys_base;
}

static struct vidc_info masterinfo;
static int cursor_init = 0;

int
vidcrender_init(vc)
	struct vconsole *vc;
{
	struct vidc_info *new;
	int loop;

	if (cold_init == 0) {
		vidcrender_coldinit(vc);
	} else {
		if (cursor_init == 0)
			vidcrender_flash_go(vc);
	}

	/*
	 * If vc->r_data is initialised then this means that the previous
	 * render engine on this vconsole was not freed properly.  I should
	 * not try to clear it up, since I could panic the kernel.  Instead
	 * I forget about its memory, which could cause a memory leak, but
	 * this would be easily detectable and fixable
	 */
     
#ifdef SELFTEST
	if (vc->r_data != 0) {
		printf( "*********************************************************\n" );
		printf( "You have configured SELFTEST mode in the console driver\n" );
		printf( "vc->rdata non zero.  This could mean a new console\n"      );
		printf( "render engine has not freed up its data structure when\n"  );
		printf( "exiting.\n" 						     );
		printf( "DO NOT COMPILE NON DEVELOPMENT KERNELS WITH SELFTEST\n"    );
		printf( "*********************************************************" );
    	}
#endif

	if (vc == vconsole_master) {
		vc->r_data = (char *)&masterinfo;
	} else {
		MALLOC((vc->r_data), char *, sizeof(struct vidc_info),
		    M_DEVBUF, M_NOWAIT);
	}
    
	if (vc->r_data==0)
		panic("render engine initialisation failed. CLEAN THIS UP!");

	R_DATA->normalfont = &font_normal;
	R_DATA->italicfont = &font_italic;
	R_DATA->boldfont   = &font_bold;
	R_DATA->font = R_DATA->normalfont;

	vidcrender_mode(vc, vidc_currentmode);
	R_DATA->scrollback_end = dispstart;
    
	new = (struct vidc_info *)vc->r_data;

	R_DATA->text_colours = 1 << (1 << R_DATA->mode.log2_bpp);	/* real number of colours */
	if (R_DATA->text_colours > 8) R_DATA->text_colours = 8;
    
#ifdef INVERSE_CONSOLE
	R_DATA->n_backcolour = R_DATA->text_colours - 1;
	R_DATA->n_forecolour = 0;
#else
	R_DATA->n_backcolour = 0;
	R_DATA->n_forecolour = R_DATA->text_colours - 1;
#endif

	R_DATA->backcolour = R_DATA->n_backcolour;
	R_DATA->forecolour = R_DATA->n_forecolour;

	R_DATA->forefillcolour = 0;
	R_DATA->backfillcolour = 0;

	R_DATA->bold = 0;
	R_DATA->reverse = 0;

	for (loop = 0; loop < R_DATA->pixelsperbyte; ++loop) {
		R_DATA->forefillcolour |= (R_DATA->forecolour <<
		    loop * (1 << R_DATA->mode.log2_bpp));
		R_DATA->backfillcolour |= (R_DATA->backcolour <<
		    loop * (1 << R_DATA->mode.log2_bpp));
	}

	R_DATA->fast_render = R_DATA->forecolour | (R_DATA->backcolour<<8) | (R_DATA->font->pixel_height<<16);
	R_DATA->blanked=0;
	vc->BLANK(vc, BLANK_NONE);
    
	if (vc == vconsole_current)
		vidcrender_textpalette(vc);


	if (cold_init == 0) {
		vidc_write(VIDC_CP1, 0x0);
		vidc_write(VIDC_CP2, 0x0);
		vidc_write(VIDC_CP3, 0x0);
	} else
		vidc_cursor_init(vc) ;

	cold_init=1;
	return 0;
}

void
vidcrender_reinit(void)
{
	vidcrender_coldinit(vconsole_current);
	vidcrender_mode(vconsole_current, vidc_currentmode);
}

void
vidcrender_putchar(dev, c, vc)
	dev_t dev;
	char c;
	struct vconsole *vc;
{
	vc->PUTSTRING(&c, 1, vc);
}

int
vidcrender_spawn(vc)
	struct vconsole *vc;
{
	vc->xchars = R_DATA->text_width;
	vc->ychars = R_DATA->text_height;
	vc->xcur = 0;
	vc->ycur = 0;
	vc->flags = 0;
	return 0;
}

int
vidcrender_redraw(vc, x, y, a, b)
	struct vconsole *vc;
	int x, y;
	int a, b;
{
	int xs, ys;
	struct vidc_state vidc;
	font_struct *p_font = R_DATA->font;
	int p_forecol = R_DATA->forecolour;
	int p_backcol = R_DATA->backcolour;
	if (x<0) x=0;
	if (y<0) y=0;
	if (x>(vc->xchars-1)) x=vc->xchars-1;
	if (y>(vc->ychars-1)) x=vc->ychars-1;

	if (a>(vc->xchars-1)) a=vc->xchars-1;
	if (b>(vc->ychars-1)) b=vc->ychars-1;

	if (a<x) a=x;
	if (b<y) b=y;


	vidc = *vidc_current;
	xs=vc->xcur;
	ys=vc->ycur;

	vc->xcur = 0;
	vc->ycur = 0;
 	if ((vc->flags&LOSSY) == 0)
	{
                register int c;
                /* This has *GOT* to be turboed */
                for ( vc->ycur=y; vc->ycur<=b; vc->ycur++ )
                {
                        for ( vc->xcur=x; vc->xcur<=a; vc->xcur++ )
                        {
                                c = (vc->charmap)[vc->xcur+vc->ycur*vc->xchars];
            			if ((c&BOLD)!=0)
					R_DATA->font = R_DATA->boldfont;
				else	
					R_DATA->font = R_DATA->normalfont;
				R_DATA->fast_render = ((c>>8)&0x7)|(((c>>11)&0x7)<<8)| (R_DATA->font->pixel_height<<16);
				if ( c & BLINKING )
					 c+=1<<8 | 1;
				if ((c&BLINKING)!=0) {
				    R_DATA->forecolour+=16;
				    R_DATA->backcolour+=16;
				}
                                vidcrender_render( vc, c&0xff );
                        }
                }
	}
	vc->xcur = xs;
	vc->ycur = ys;
	R_DATA->forecolour = p_forecol;
	R_DATA->backcolour = p_backcol;
	R_DATA->font = p_font;
	return 0;
}


int
vidcrender_swapin(vc)
	struct vconsole *vc;
{
	register int counter;
	int xs, ys;
	struct vidc_state vidc;
	font_struct *p_font = R_DATA->font;
	int p_forecol = R_DATA->forecolour;
	int p_backcol = R_DATA->backcolour;

#ifdef ACTIVITY_WARNING
	vconsole_pending = 0;
#endif
	vidc_write ( VIDC_CP1, 0x0 );

	vidc = *vidc_current;
	vidc_write ( VIDC_PALREG, 0x00000000 );
	for ( counter=0; counter<255; counter++ )
		vidc_write ( VIDC_PALETTE, 0x00000000 );
	xs=vc->xcur;
	ys=vc->ycur;
/*TODO This needs to be vidc_restore (something) */
        vidcrender_mode ( vc, &MODE );

	vc->xcur = 0;
	vc->ycur = 0;
 	if ( (vc->flags&LOSSY) == 0 )
	{
                register int c;
                /* This has *GOT* to be turboed */
                for ( vc->ycur=0; vc->ycur<vc->ychars; vc->ycur++ )
                {
                        for ( vc->xcur=0; vc->xcur<vc->xchars; vc->xcur++ )
                        {
                                c = (vc->charmap)[vc->xcur+vc->ycur*vc->xchars];
            			if ((c&BOLD)!=0)
					R_DATA->font = R_DATA->boldfont;
				else	
					R_DATA->font = R_DATA->normalfont;
/*
    				R_DATA->forecolour = ((c>>8)&0x7);
 				R_DATA->backcolour = ((c>>11)&0x7);
*/
				R_DATA->fast_render = ((c>>8)&0x7)|(((c>>11)&0x7)<<8)| (R_DATA->font->pixel_height<<16);
				if ( c & BLINKING )
					c+=1<<8 | 1;
				if ((c&BLINKING)!=0)
				{
				    R_DATA->forecolour+=16;
				    R_DATA->backcolour+=16;
				}
                                vidcrender_render( vc, c&0xff );
                        }
                }
	}
	else
	{
	    vc->CLS ( vc );
	}

	if ( vc->vtty==1 )
	{
	    vc->xcur = xs;
	    vc->ycur = ys;
	    vidcrender_textpalette ( vc );
	    vidc_write ( VIDC_CP1, 0xffffff );
 	    R_DATA->forecolour = p_forecol;
	    R_DATA->backcolour = p_backcol;
	    R_DATA->font = p_font;
	}
/* Make the cursor blank */
        IOMD_WRITE_WORD(IOMD_CURSINIT,p_cursor_transparent);
	return 0;

}

paddr_t
vidcrender_mmap(vc, offset, nprot)
	struct vconsole *vc;
	off_t offset;
	int nprot;
{
	if ((u_int)offset >= videomemory.vidm_size)
		return (-1);
	return(arm_btop(((videomemory.vidm_pbase) + (offset))));
}

void
vidcrender_render(vc, c)
	struct vconsole *vc;
	char c;
{
	register unsigned char *fontaddr;
	register unsigned char *addr;

	/* Calculate the font's address */

	fontaddr = R_DATA->font->data
            + ((c-(0x20)) * R_DATA->font->height
            * R_DATA->font->width);

	addr = (unsigned char *)dispstart
            + (vc->xcur * R_DATA->font->x_spacing)
            + (vc->ycur * R_DATA->bytes_per_line);

	vidcrendermc_render ( addr, fontaddr, R_DATA->fast_render,
    	    R_DATA->XRES );
}

/*
 * Uugh.  vidc graphics dont support scrolling regions so we have to emulate
 * it here.  This would normally require much software scrolling which is
 * horriblly slow, so I'm going to try and do a composite scroll, which
 * causes problems for scrollback but it's less speed critical
 */

void
vidcrender_scrollup(vc, low, high)
	struct vconsole *vc;
	int low;
	int high;
{
    unsigned char *start, *end;

    if ( ( low==0 ) && ( high==vc->ychars-1 ))
    {
	/* All hardware scroll */
        dispstart+=R_DATA->bytes_per_line;
        if ( dispstart >= dispend )
    	    dispstart -= dispsize;

	high=high+1;	/* Big hack */

        IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart - ptov );
    }
    else
    {
        char *oldstart=(char *)dispstart;

	/* Composite scroll */

	if ( (high-low) > (vc->ychars>>1) )
	{
	    /* Scroll region greater than half the screen */

            dispstart+=R_DATA->bytes_per_line;
            if ( dispstart >= dispend ) dispstart -= dispsize;

            IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart - ptov );

	    if ( low!=0 )
	    {
    	        start = (unsigned char *)oldstart;
    	        end=(unsigned char*)oldstart+((low+1) * R_DATA->bytes_per_line);
    	        memcpy(start+R_DATA->bytes_per_line, start,
		    end-start-R_DATA->bytes_per_line);
	    }

	    if ( high!=(vc->ychars-1) )
	    {
    	        start =(unsigned char *)dispstart+(high)*R_DATA->bytes_per_line;
    	        end=(unsigned char*)dispstart+((vc->ychars)*R_DATA->bytes_per_line);
    	        memcpy(start+R_DATA->bytes_per_line, start,
		    end-start-R_DATA->bytes_per_line);
	    }
	    high++;
	}
	else
	{
	    /* Scroll region less than half the screen */

	    /* NO COMPOSITE SCROLL YET */

	    high++;
	    if (low<0) low=0;
	    if (high>(vc->ychars)) high=vc->ychars;
	    if (low>high) return;	/* yuck */
	    start = (unsigned char *)dispstart + ((low)*R_DATA->bytes_per_line);
	    end = (unsigned char *)dispstart +  ((high)*R_DATA->bytes_per_line);
	    memcpy(start, start+R_DATA->bytes_per_line,
		(end-start)-R_DATA->bytes_per_line );
	    R_DATA->scrollback_end = dispstart;
	}
    }
    memset ( (char *) dispstart + ((high-1)*R_DATA->bytes_per_line) ,
	     R_DATA->backfillcolour,
	     R_DATA->bytes_per_line   );
}

void
vidcrender_scrolldown(vc, low, high)
	struct vconsole *vc;
	int low;
	int high;
{
    unsigned char *start;
    unsigned char *end;

    if ( low<0 ) low = 0;
    if ( high>(vc->ychars-1) ) high=vc->ychars-1;
    
    if ( ( low==0 ) && ( high==vc->ychars-1 ))
    {
        dispstart-=R_DATA->bytes_per_line;

        if ( dispstart < dispbase )
    	    dispstart += dispsize;

        IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart - ptov );
    }
    else
    {
	if ( ((high-low) > (vc->ychars>>1)) )
	{
high--;
   	    if (high!=(vc->ychars-1))
	    {
	        start =(unsigned char*)dispstart+((high+1)*R_DATA->bytes_per_line);
	        end=(unsigned char*)dispstart+((vc->ychars)*R_DATA->bytes_per_line);
  	        memcpy(start, start+R_DATA->bytes_per_line,
	 	    (end-start)-R_DATA->bytes_per_line );
	    }

            dispstart-=R_DATA->bytes_per_line;
            if ( dispstart < dispbase )
    	        dispstart += dispsize;
            IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart - ptov);
    	    start = (unsigned char *)dispstart + (low * R_DATA->bytes_per_line);

   	    if (low!=0)
	    {
	        end = (unsigned char *)dispstart + ((low+1)*R_DATA->bytes_per_line);
  	        memcpy((char *)dispstart,
  	            (char *)(dispstart+R_DATA->bytes_per_line),
		    (int)((end-dispstart)-R_DATA->bytes_per_line ));
	    }
	}
        else
	{
    	    start = (unsigned char *)dispstart + (low * R_DATA->bytes_per_line);
    	    end = (unsigned char *)dispstart  + ((high+1) * R_DATA->bytes_per_line);
    	    memcpy(start+R_DATA->bytes_per_line, start,
    	        end-start-R_DATA->bytes_per_line);
	}

    }
    memset ((char*) dispstart + (low*R_DATA->bytes_per_line) ,
		R_DATA->backfillcolour, R_DATA->bytes_per_line );
}

void
vidcrender_cls(vc)
	struct vconsole *vc;
{
	vidcrendermc_cls ( (char *)dispstart, (char *)dispstart+R_DATA->screensize, R_DATA->backfillcolour );
    /*
	memset((char *)dispstart,
		R_DATA->backfillcolour, R_DATA->screensize);
    */  
	vc->xcur = vc->ycur = 0;
}

void
vidcrender_update(vc)
	struct vconsole *vc;
{
}

static char vidcrender_name[] = VIDC_ENGINE_NAME;

static int scrollback_ptr = 0;

int
vidcrender_scrollback(vc)
	struct vconsole *vc;
{
	int temp;

	if (scrollback_ptr==0)
	    scrollback_ptr=dispstart;

	temp = scrollback_ptr;

	scrollback_ptr-=R_DATA->bytes_per_line * (vc->ychars-2);
	
        if ( scrollback_ptr < dispbase )
    	    scrollback_ptr += dispsize;

	if (	(scrollback_ptr>dispstart)&&
		(scrollback_ptr<(dispstart+R_DATA->screensize) ) )
	{
	    scrollback_ptr=temp;
	    return 0;
	}

	vc->r_scrolledback = 1;

        IOMD_WRITE_WORD(IOMD_VIDINIT, scrollback_ptr - ptov);
	return 0;
}

int
vidcrender_scrollforward(vc)
	struct vconsole *vc;
{
	register int temp;

	if (scrollback_ptr==0)
	    return 0;
	
	temp = scrollback_ptr;

	scrollback_ptr+=R_DATA->bytes_per_line * (vc->ychars - 2);
	
        if ( scrollback_ptr >= dispend )
    	    scrollback_ptr -= dispsize;

	if ( scrollback_ptr == dispstart )
	{
            IOMD_WRITE_WORD(IOMD_VIDINIT, scrollback_ptr - ptov);
	    scrollback_ptr=0;
	    vc->r_scrolledback = 0;
	    return 0;
	}

        IOMD_WRITE_WORD(IOMD_VIDINIT, scrollback_ptr - ptov);
	return 0;
}

int
vidcrender_scrollbackend(vc)
	struct vconsole *vc;
{
	scrollback_ptr = 0;
        IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart - ptov);
	vc->r_scrolledback = 0;
	return 0;
}

int
vidcrender_clreos(vc, code)
	struct vconsole *vc;
	int code;
{
    char *addr;
    char *endofscreen;

    addr = (unsigned char *)dispstart
         + (vc->xcur * R_DATA->font->x_spacing)
         + (vc->ycur * R_DATA->bytes_per_line);

    endofscreen = (unsigned char *)dispstart
         + (vc->xchars * R_DATA->font->x_spacing)
         + (vc->ychars * R_DATA->bytes_per_line);


	switch (code)
	{
		case 0:
    			vidcrendermc_cls ( addr, 
				(unsigned char *)dispend, 
				R_DATA->backfillcolour );
			if ((unsigned char *)endofscreen > (unsigned char *)dispend) {
				char string[80];
				sprintf(string, "(addr=%08x eos=%08x dispend=%08x dispstart=%08x base=%08x)",
				   (u_int)addr, (u_int)endofscreen, dispend, dispstart, dispbase);
				dprintf(string);
				vidcrendermc_cls((unsigned char *)dispbase, (unsigned char *)(dispbase + (endofscreen - dispend)), R_DATA->backfillcolour);
			}
			break;

		case 1:
    			vidcrendermc_cls ( (unsigned char *)dispstart+R_DATA->screensize, 
				addr, 
				R_DATA->backfillcolour );
			break;
			
		case 2:
		default:
			vidcrender_cls ( vc );
			break;
	}
	return 0;
}

#define VIDC R_DATA->vidc

int
vidcrender_debugprint(vc)
	struct vconsole *vc;
{
#ifdef	DEVLOPING
    printf ( "VIDCCONSOLE DEBUG INFORMATION\n\n" );
    printf ( "res (%d, %d) charsize (%d, %d) cursor (%d, %d)\n"
							      , R_DATA->XRES, R_DATA->YRES
							      , vc->xchars, vc->ychars, vc->xcur, vc->ycur );
    printf ( "bytes_per_line %d\n"			      , R_DATA->bytes_per_line 		     );
    printf ( "pixelsperbyte  %d\n"			      , R_DATA->pixelsperbyte		     );
    printf ( "dispstart      %08x\n"			      , dispstart 		     );
    printf ( "dispend        %08x\n"			      , dispend 		     );
    printf ( "screensize     %08x\n"			      , R_DATA->screensize 		     );

    printf ( "fontwidth      %08x\n"			      , R_DATA->font->pixel_width 	     );
    printf ( "fontheight     %08x\n"			      , R_DATA->font->pixel_height           );
    printf ( "\n" 										     );
    printf ( "palreg = %08x  bcol    = %08x\n"		      , VIDC.palreg, VIDC.bcol		     );
    printf ( "cp1    = %08x  cp2     = %08x  cp3    = %08x\n" , VIDC.cp1, VIDC.cp2, VIDC.cp3         );
    printf ( "hcr    = %08x  hswr    = %08x  hbsr   = %08x\n" , VIDC.hcr, VIDC.hswr, VIDC.hbsr       );
    printf ( "hder   = %08x  hber    = %08x  hcsr   = %08x\n" , VIDC.hder, VIDC.hber, VIDC.hcsr      );
    printf ( "hir    = %08x\n"				      , VIDC.hir 			     );
    printf ( "vcr    = %08x  vswr    = %08x  vbsr   = %08x\n" , VIDC.vcr, VIDC.vswr, VIDC.vbsr       );
    printf ( "vder   = %08x  vber    = %08x  vcsr   = %08x\n" , VIDC.vder, VIDC.vber, VIDC.vcsr      );
    printf ( "vcer   = %08x\n"				      , VIDC.vcer 			     );
    printf ( "ereg   = %08x  fsynreg = %08x  conreg = %08x\n" , VIDC.ereg, VIDC.fsynreg, VIDC.conreg );
    printf ( "\n" );
    printf ( "flash %08x, cursor_flash %08x", R_DATA->flash, R_DATA->cursor_flash );
#else
    printf ( "VIDCCONSOLE - NO DEBUG INFO\n" );
#endif
    return 0;
}

#ifdef NICE_UPDATE
static int need_update = 0;

void
vidcrender_updatecursor(arg)
	void *arg;
{
	struct vconsole *vc = vconsole_current;

	vidc_write(VIDC_HCSR, R_DATA->frontporch-17+ (vc->xcur)*R_DATA->font->pixel_width );
	vidc_write(VIDC_VCSR, R_DATA->topporch-2+ (vc->ycur+1)*R_DATA->font->pixel_height-2 + 3
	    - R_DATA->font->pixel_height);
	vidc_write(VIDC_VCER, R_DATA->topporch-2+ (vc->ycur+3)*R_DATA->font->pixel_height+2 + 3 );
	return;
}

int
vidcrender_cursorupdate(vc)
	struct vconsole *vc;
{
    timeout ( vidcrender_updatecursor, NULL, 20 );
    return 0;
}

#else

static int
vidcrender_cursorupdate(vc)
	struct vconsole *vc;
{
	vidc_write(VIDC_HCSR, R_DATA->frontporch-17+ (vc->xcur)*R_DATA->font->pixel_width );
	vidc_write(VIDC_VCSR, R_DATA->topporch-2+ (vc->ycur+1)*R_DATA->font->pixel_height-2 + 3
	    - R_DATA->font->pixel_height);
	vidc_write(VIDC_VCER, R_DATA->topporch-2+ (vc->ycur+3)*R_DATA->font->pixel_height+2 + 3 );
	return (0);
}

#endif

#define DEFAULT_CURSORSPEED (25)

static int CURSORSPEED = DEFAULT_CURSORSPEED;

static int
vidcrender_cursorflashrate(vc, rate)
	struct vconsole *vc;
	int rate;
{
	CURSORSPEED = 60/rate;
	return(0);
}

static int cursorcounter=DEFAULT_CURSORSPEED;
static int flashcounter=DEFAULT_CURSORSPEED;
#ifdef PRETTYCURSOR
static int pretty=0xff;
#endif

static int cursor_col = 0x0;

int
vidcrender_cursorintr(arg)
	void *arg;
{
	struct vconsole *vc = arg;
	if ( cursor_flash==0 )
		return 0;

	/*
	 * We don't need this.
	 */
	if (vconsole_blankcounter >= 0)
		vconsole_blankcounter--;

	if (vconsole_blankcounter == 0) {
		vconsole_blankcounter=vconsole_blankinit;
		vidcrender_blank ( vc, BLANK_OFF );
	}

	cursorcounter--;
	if (cursorcounter<=0) {
		cursorcounter=CURSORSPEED;
		cursor_col = cursor_col ^ 0xffffff;
#ifdef ACTIVITY_WARNING
		if (vconsole_pending)  {
			if ( cursor_col==0 )
				IOMD_WRITE_WORD(IOMD_CURSINIT,p_cursor_transparent);
		        else
				IOMD_WRITE_WORD(IOMD_CURSINIT,p_cursor_normal);
			vidc_write ( VIDC_CP1, cursor_col&0xff );
		} else
#endif
		{
			if ( cursor_col==0 )
				IOMD_WRITE_WORD(IOMD_CURSINIT,p_cursor_transparent);
			else
				IOMD_WRITE_WORD(IOMD_CURSINIT,p_cursor_normal);
			vidc_write ( VIDC_CP1, 0xffffff );
		}
	}
	return(0);	/* Pass interrupt on down the chain */
}

int
vidcrender_flashintr(arg)
	void *arg;
{
/*	struct vconsole *vc = arg;*/
	if ( flash==0 )
		return 0;

	flashcounter--;
	if (flashcounter<=0) {
		flashcounter=CURSORSPEED;
		if ( cursor_col == 0 ) {

			vidc_write(VIDC_PALREG, 0x00000010);
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(255,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0, 255,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(255, 255,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0, 255));
			vidc_write(VIDC_PALETTE, VIDC_COL(255,   0, 255));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0, 255, 255));
			vidc_write(VIDC_PALETTE, VIDC_COL(255, 255, 255));
			vidc_write(VIDC_PALETTE, VIDC_COL(128, 128, 128));
			vidc_write(VIDC_PALETTE, VIDC_COL(255, 128, 128));
			vidc_write(VIDC_PALETTE, VIDC_COL(128, 255, 128));
			vidc_write(VIDC_PALETTE, VIDC_COL(255, 255, 128));
			vidc_write(VIDC_PALETTE, VIDC_COL(128, 128, 255));
			vidc_write(VIDC_PALETTE, VIDC_COL(255, 128, 255));
			vidc_write(VIDC_PALETTE, VIDC_COL(255, 255, 255));
		} else {
			vidc_write(VIDC_PALREG, 0x00000010);
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
			vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
		}
	}
	return(0);	/* Pass interrupt on down the chain */
}

static int
vidc_cursor_init(vc)
	struct vconsole *vc;
{
	static char *cursor_data = NULL;
	int counter;
	int line;
	paddr_t pa;

	if (!cursor_data) {
		/* Allocate cursor memory first time round */
		cursor_data = (char *)uvm_km_zalloc(kernel_map, PAGE_SIZE);
		if (!cursor_data)
			panic("Cannot allocate memory for hardware cursor");
		(void) pmap_extract(pmap_kernel(), (vaddr_t)cursor_data, &pa);
		IOMD_WRITE_WORD(IOMD_CURSINIT, pa);
	}

	/* Blank the cursor while initialising it's sprite */

	vidc_write ( VIDC_CP1, 0x0 );
	vidc_write ( VIDC_CP2, 0x0 );
	vidc_write ( VIDC_CP3, 0x0 );

 	cursor_normal       = cursor_data;
	cursor_transparent  = cursor_data + (R_DATA->font->pixel_height *
					R_DATA->font->pixel_width);

 	cursor_transparent += 32;
	cursor_transparent = (char *)((int)cursor_transparent & (~31) );

	for ( line = 0; line<R_DATA->font->pixel_height; ++ line )
	{
	    for ( counter=0; counter<R_DATA->font->pixel_width/4;counter++ )
		cursor_normal[line*R_DATA->font->pixel_width + counter]=0x55;
	    for ( ; counter<8; counter++ )
		cursor_normal[line*R_DATA->font->pixel_width + counter]=0;
	}

	for ( line = 0; line<R_DATA->font->pixel_height; ++ line )
	{
	    for ( counter=0; counter<R_DATA->font->pixel_width/4;counter++ )
		cursor_transparent[line*R_DATA->font->pixel_width + counter]=0x00;
	    for ( ; counter<8; counter++ )
		cursor_transparent[line*R_DATA->font->pixel_width + counter]=0;
	}


	(void) pmap_extract(pmap_kernel(), (vaddr_t)cursor_normal,
	    (paddr_t *)&p_cursor_normal);
	(void) pmap_extract(pmap_kernel(), (vaddr_t)cursor_transparent,
	    (paddr_t *)&p_cursor_transparent);

/*
	memset ( cursor_normal, 0x55,
		R_DATA->font->pixel_width*R_DATA->font->pixel_height );

	memset ( cursor_transparent, 0x55,
		R_DATA->font->pixel_width*R_DATA->font->pixel_height );
*/

	/* Ok, now see the cursor */

	vidc_write ( VIDC_CP1, 0xffffff );
        return 0;
}

int
vidcrender_setfgcol(vc, col)
	struct vconsole *vc;
	int col;
{
	register int loop;

	if ( R_DATA->forecolour >= 16 )
		R_DATA->forecolour=16;
	else
		R_DATA->forecolour=0;
    
	R_DATA->forefillcolour = 0;
    
	R_DATA->forecolour += col;
    
/*TODO
	if ( R_DATA->forecolour >> 1<<R_DATA->NUMCOLOURS )
		R_DATA->forecolour>>1;
*/

	for (loop = 0; loop < R_DATA->pixelsperbyte; ++loop) {
		R_DATA->forefillcolour |= (R_DATA->forecolour <<
		    loop * (1 << R_DATA->mode.log2_bpp));
	}
	R_DATA->fast_render = R_DATA->forecolour | (R_DATA->backcolour<<8) | (R_DATA->font->pixel_height<<16);
	return 0;
}

int
vidcrender_setbgcol(vc, col)
	struct vconsole *vc;
	int col;
{
	register int loop;

	if ( R_DATA->backcolour >= 16 )
		R_DATA->backcolour=16;
	else
		R_DATA->backcolour=0;

	R_DATA->backfillcolour = 0;
	R_DATA->backcolour += col;
 /*TODO   
	if ( R_DATA->backcolour >> 1<<R_DATA->NUMCOLOURS )
		R_DATA->backcolour>>1;
*/

	for (loop = 0; loop < R_DATA->pixelsperbyte; ++loop) {
		R_DATA->backfillcolour |= (R_DATA->backcolour <<
		    loop * (1 << R_DATA->mode.log2_bpp));
	}
	return 0;
}

int
vidcrender_textpalette(vc)
	struct vconsole *vc;
{
        R_DATA->forecolour = COLOUR_WHITE_8;
        R_DATA->backcolour = COLOUR_BLACK_8;

        vidc_write(VIDC_PALREG, 0x00000000);
        vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0,   0));
        vidc_write(VIDC_PALETTE, VIDC_COL(255,   0,   0));
        vidc_write(VIDC_PALETTE, VIDC_COL(  0, 255,   0));
        vidc_write(VIDC_PALETTE, VIDC_COL(255, 255,   0));
        vidc_write(VIDC_PALETTE, VIDC_COL(  0,   0, 255));
        vidc_write(VIDC_PALETTE, VIDC_COL(255,   0, 255));
        vidc_write(VIDC_PALETTE, VIDC_COL(  0, 255, 255));
        vidc_write(VIDC_PALETTE, VIDC_COL(255, 255, 255));
        vidc_write(VIDC_PALETTE, VIDC_COL(128, 128, 128));
        vidc_write(VIDC_PALETTE, VIDC_COL(255, 128, 128));
        vidc_write(VIDC_PALETTE, VIDC_COL(128, 255, 128));
        vidc_write(VIDC_PALETTE, VIDC_COL(255, 255, 128));
        vidc_write(VIDC_PALETTE, VIDC_COL(128, 128, 255));
        vidc_write(VIDC_PALETTE, VIDC_COL(255, 128, 255));
        vidc_write(VIDC_PALETTE, VIDC_COL(255, 255, 255));

R_DATA->fast_render = R_DATA->forecolour | (R_DATA->backcolour<<8) | (R_DATA->font->pixel_height<<16);
    return 0;
}

int
vidcrender_sgr(vc, type)
	struct vconsole *vc;
	int type;
{
    switch(type)
    {
        case 0: /* Normal */
	    if ((1 << R_DATA->mode.log2_bpp) == 8)
      	    {
        	R_DATA->n_forecolour = COLOUR_WHITE_8;
        	R_DATA->n_backcolour = COLOUR_BLACK_8;
      	    }

	    R_DATA->forecolour = R_DATA->n_forecolour;
            R_DATA->backcolour = R_DATA->n_backcolour;
            R_DATA->font = R_DATA->normalfont;
            break;

	case 1: /* bold */
            R_DATA->font = R_DATA->boldfont;
	    break;	

	case 22: /* not bold */
            R_DATA->font = R_DATA->normalfont;
	    break;	

        case 5:	/* blinking */
	    if ( R_DATA->forecolour < 16 )
	    {
		R_DATA->forecolour+=16;
		R_DATA->backcolour+=16;
		R_DATA->n_forecolour+=16;
		R_DATA->n_backcolour+=16;
	    }
	    break;

        case 25: /* not blinking */
	    if ( R_DATA->forecolour >= 16 )
	    {
		R_DATA->forecolour-=16;
		R_DATA->backcolour-=16;
		R_DATA->n_forecolour-=16;
		R_DATA->n_backcolour-=16;
	    }
	    break;

        case 7: /* reverse */
	    R_DATA->forecolour = R_DATA->n_backcolour;
            R_DATA->backcolour = R_DATA->n_forecolour;
    	    break;
            
        case 27: /* not reverse */
	    R_DATA->forecolour = R_DATA->n_forecolour;
            R_DATA->backcolour = R_DATA->n_backcolour;
    	    break;
    }
R_DATA->fast_render = R_DATA->forecolour | (R_DATA->backcolour<<8) | (R_DATA->font->pixel_height<<16);
    return 0;
}

int
vidcrender_scrollregion(vc, low, high)
	struct vconsole *vc;
	int low;
	int high;
{
	return 0;
}

int
vidcrender_blank(vc, type)
	struct vconsole *vc;
	int type;
{
        int ereg;

	vc->blanked=type;

	ereg = 1<<12;
	if (vidc_currentmode->sync_pol & 0x01)
		ereg |= 1<<16;
	if (vidc_currentmode->sync_pol & 0x02)
		ereg |= 1<<18;

	switch (type) {
	case 0:
    		vidc_write(VIDC_EREG, ereg);
		break;
		
	case 1: /* not implemented yet */
	case 2:
	case 3:
		vidc_write(VIDC_EREG, 0);
		break;
	}
	return 0;
}

int vidcrender_ioctl ( struct vconsole *vc, dev_t dev, int cmd, caddr_t data,
			int flag, struct proc *p )
{
	int error;
	int bpp, log2_bpp;
	struct tty *tp;
	struct winsize ws;
	struct vidc_mode *vmode;

	switch (cmd) {
	case CONSOLE_MODE:
    		tp = find_tp(dev);
		vmode = (struct vidc_mode *) data;

		/* the interface specifies bpp instead of log2_bpp ... fix me! */
		bpp = vmode->log2_bpp;

		/* translate 24 to 32 ... since this is the correct log2 value */
		if (bpp == 24) bpp=32;

		/* translate the bpp to log2_bpp */
		if (bpp < 1 || bpp > 32)
			bpp = 8; /* Set 8 bpp if we get asked for something silly */
		for (log2_bpp = 0; bpp != 1; bpp >>= 1)
			log2_bpp++;
                vmode->log2_bpp = log2_bpp;

		/* set new mode and update structures */
		vidcrender_mode ( vc, vmode );
    		vc->MODECHANGE ( vc );
		ws.ws_row=vc->ychars;
		ws.ws_col=vc->xchars;
		error = (*tp->t_linesw->l_ioctl)(tp, TIOCSWINSZ, (char *)&ws, flag, p);
		if (error != EPASSTHROUGH)
			return (error);
		return ttioctl(tp, TIOCSWINSZ, (char *)&ws, flag, p);
		break;

	case CONSOLE_RESETSCREEN:
	{
		extern unsigned int dispbase;
		extern unsigned int dispsize;
		extern unsigned int ptov;
		extern unsigned int transfersize;

		IOMD_WRITE_WORD(IOMD_VIDINIT, dispbase-ptov);
		IOMD_WRITE_WORD(IOMD_VIDSTART, dispbase-ptov);
		IOMD_WRITE_WORD(IOMD_VIDEND, (dispbase+dispsize-transfersize)-ptov);
		return 0;
        }
	case CONSOLE_RESTORESCREEN:
	{
		extern unsigned int dispstart;
		extern unsigned int dispsize;
		extern unsigned int ptov;
		extern unsigned int transfersize;

		IOMD_WRITE_WORD(IOMD_VIDINIT, dispstart-ptov);
		IOMD_WRITE_WORD(IOMD_VIDSTART, dispstart-ptov);
		IOMD_WRITE_WORD(IOMD_VIDEND, (dispstart+dispsize-transfersize)-ptov);
		vidc_stdpalette();
		return 0;
        }
	case CONSOLE_GETINFO:
	{
		extern videomemory_t videomemory;
		register struct console_info *inf = (void *)data;


		inf->videomemory = videomemory;
		inf->width  = R_DATA->mode.hder;
		inf->height = R_DATA->mode.vder;

		inf->bpp    = 1 << R_DATA->mode.log2_bpp;

		return 0;
	}
	case CONSOLE_PALETTE:
	{
		register struct console_palette *pal = (void *)data;
		vidc_write(VIDC_PALREG, pal->entry);
		vidc_write(VIDC_PALETTE, VIDC_COL(pal->red, pal->green, pal->blue));
 		return 0;
	}
	}
	return (EPASSTHROUGH);
}

int
vidcrender_attach(vc, parent, self, aux)
	struct vconsole *vc;
	struct device *parent;
	struct device *self;
	void *aux;
{
	vidc_cursor_init ( vc );
	vidcrender_flash_go ( vc );
	return 0;
}

int
vidcrender_flash_go(vc)
	struct vconsole *vc;
{
	static int irqclaimed = 0;
	static int lock=0;
	if (lock==1)
		return -1;
	lock=0;

	if (!irqclaimed) {
		cursor_ih.ih_func = vidcrender_cursorintr;
		cursor_ih.ih_arg = vc;
		cursor_ih.ih_level = IPL_TTY;
		cursor_ih.ih_name = "vsync";
		irq_claim ( IRQ_FLYBACK, &cursor_ih );
		irqclaimed = 1;
	}

	cursor_init = 0;
	return 0;
}

/* What does this function do ? */
int 
vidcrender_flash(vc, flash)
	struct vconsole *vc;
	int flash;
{
	flash = flash;
	return(0);
}

int
vidcrender_cursorflash(vc, flash)
	struct vconsole *vc;
	int flash;
{
	cursor_flash = flash;
	return(0);
}

struct render_engine vidcrender = {
	vidcrender_name,
	vidcrender_init,
	
	vidcrender_putchar,

	vidcrender_spawn,
	vidcrender_swapin,
	vidcrender_mmap,
	vidcrender_render,
	vidcrender_scrollup,
	vidcrender_scrolldown,
	vidcrender_cls,
	vidcrender_update,
	vidcrender_scrollback,
	vidcrender_scrollforward,
	vidcrender_scrollbackend,
	vidcrender_clreos,
	vidcrender_debugprint,
	vidcrender_cursorupdate,
	vidcrender_cursorflashrate,
	vidcrender_setfgcol,
	vidcrender_setbgcol,
	vidcrender_textpalette,
	vidcrender_sgr,
	vidcrender_blank,
	vidcrender_ioctl,
	vidcrender_redraw,
	vidcrender_attach,
	vidcrender_flash,
	vidcrender_cursorflash
};
