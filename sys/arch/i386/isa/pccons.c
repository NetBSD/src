/*	$NetBSD: pccons.c,v 1.182.4.1 2007/03/12 05:48:38 rmind Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */

/*
 * code to work keyboard & display for PC-style console
 *
 * "NPCCONSKBD > 0" means that we access the keyboard through the MI keyboard
 * controller driver, ==0 that we access it directly.
 * XXX Only one of these attachments can be used in one kernel configuration.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pccons.c,v 1.182.4.1 2007/03/12 05:48:38 rmind Exp $");

#include "opt_ddb.h"
#include "opt_xserver.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <dev/cons.h>

#include "pc.h"
#include <machine/bus.h>
#if (NPCCONSKBD > 0)
#include <dev/pckbport/pckbportvar.h>
#else
/* consistency check: plain pccons can't coexist with pckbc */
#include "pckbc.h"
#if (NPCKBC > 0)
#error "(pc without pcconskbd) and pckbc can't coexist"
#endif
#endif /* NPCCONSKBD */

/* consistency check: pccons can't coexist with vga/ega or pcdisplay */
#include "vga.h"
#include "ega.h"
#include "pcdisplay.h"
#if (NVGA > 0) || (NEGA > 0) || (NPCDISPLAY > 0)
#error "pc and (vga or pcdisplay) can't coexist"
#endif

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>
#include <machine/pccons.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/pcdisplay.h>
#include <dev/ic/i8042reg.h>
#include <dev/pckbport/pckbdreg.h>

#define	XFREE86_BUG_COMPAT

#ifndef BEEP_FREQ
#define BEEP_FREQ 1500
#endif
#ifndef BEEP_TIME
#define BEEP_TIME (hz/5)
#endif

#define PCBURST 128

/*
 * Non-US keyboards definition
 */
#if defined(FRENCH_KBD) || defined(GERMAN_KBD) || defined(NORWEGIAN_KBD) || defined(FINNISH_KBD)
# define NONUS_KBD
# define DISPLAY_ISO8859
# define NUMERIC_SLASH_FIX
#endif

static u_short *Crtat;			/* pointer to backing store */
static u_short *crtat;			/* pointer to current char */
#if (NPCCONSKBD == 0)
static volatile u_char ack, nak;	/* Don't ask. */
static int poll_data = -1;
#endif
static u_char async, kernel, polling;	/* Really, you don't want to know. */
static u_char lock_state = 0x00;	/* all off */
#if (NPCCONSKBD == 0)
static u_char old_lock_state = 0xff,
	      typematic_rate = 0xff,	/* don't update until set by user */
	      old_typematic_rate = 0xff;
#endif
static u_short cursor_shape = 0xffff,	/* don't update until set by user */
	       old_cursor_shape = 0xffff;
#ifdef XSERVER
int pc_xmode = 0;
#endif
int pccons_is_console = 0;
#if (NPCCONSKBD > 0)
static pckbport_tag_t kbctag;
static pckbport_slot_t kbcslot;
static int kbc_attached;
#endif

#define	PCUNIT(x)	(minor(x))

static struct video_state {
	int 	cx, cy;		/* escape parameters */
	int 	row, col;	/* current cursor position */
	int 	nrow, ncol, nchr;	/* current screen geometry */
	u_char	state;		/* parser state */
#define	VSS_ESCAPE	1
#define	VSS_EBRACE	2
#define	VSS_EPARAM	3
	char	so;		/* in standout mode? */
	char	color;		/* color or mono display */
	char	at;		/* normal attributes */
	char	so_at;		/* standout attributes */
} vs;

struct pc_softc {
	struct	device sc_dev;
	void	*sc_ih;
	struct	tty *sc_tty;
};

static struct callout async_update_ch = CALLOUT_INITIALIZER;

int pcprobe(struct device *, struct cfdata *, void *);
void pcattach(struct device *, struct device *, void *);
int pcintr(void *);
void pcinit(void);

CFATTACH_DECL(pc, sizeof(struct pc_softc),
    pcprobe, pcattach, NULL, NULL);

extern struct cfdriver pc_cd;

#if (NPCCONSKBD > 0)
struct pcconskbd_softc {
	struct	device sc_dev;
};

int pcconskbdprobe(struct device *, struct cfdata *, void *);
void pcconskbdattach(struct device *, struct device *, void *);
void pcinput(void *, int);

CFATTACH_DECL(pcconskbd, sizeof(struct pcconskbd_softc), pcconskbdprobe,
    pcconskbdattach, NULL, NULL);

extern struct cfdriver pcconskbd_cd;
#endif

dev_type_open(pcopen);
dev_type_close(pcclose);
dev_type_read(pcread);
dev_type_write(pcwrite);
dev_type_ioctl(pcioctl);
dev_type_stop(pcstop);
dev_type_tty(pctty);
dev_type_poll(pcpoll);
dev_type_mmap(pcmmap);
 
const struct cdevsw pc_cdevsw = {
	pcopen, pcclose, pcread, pcwrite, pcioctl,
	pcstop, pctty, pcpoll, pcmmap, ttykqfilter, D_TTY
};

#define	COL		80
#define	ROW		25
#define	CHR		2

/*
 * DANGER WIL ROBINSON -- the values of SCROLL, NUM, CAPS, and ALT are
 * important.
 */
#define	SCROLL		0x0001	/* stop output */
#define	NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	SHIFT		0x0008	/* keyboard shift */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	ASCII		0x0020	/* ascii code for this key */
#define	ALT		0x0080	/* alternate shift -- alternate chars */
#define	FUNC		0x0100	/* function key */
#define	KP		0x0200	/* Keypad keys */
#define	NONE		0x0400	/* no function */
#ifdef NONUS_KBD
#define ALTGR           0x0040  /* Alt graphic */
#endif

static unsigned int addr_6845 = MONO_BASE;

#if (NPCCONSKBD == 0)
char *sget(void);
#endif
char *strans(u_char);
void sput(const u_char *, int);
#ifdef XSERVER
void pc_xmode_on(void);
void pc_xmode_off(void);
#endif

void	pcstart(struct tty *);
int	pcparam(struct tty *, struct termios *);

#if (NPCCONSKBD == 0)
int kbd_cmd(u_char, u_char);
#endif
void set_cursor_shape(void);
#ifdef XSERVER
#ifdef XFREE86_BUG_COMPAT
void get_cursor_shape(void);
#endif
#endif
void do_async_update(void *);
void async_update(void);
#if (NPCCONSKBD > 0)
void update_leds(void);
#else
#define update_leds async_update
#endif

#if (NPCCONSKBD == 0)
static inline int kbd_wait_output(void);
static inline int kbd_wait_input(void);
static inline void kbd_flush_input(void);
static u_char kbc_get8042cmd(void);
static int kbc_put8042cmd(u_char);
#endif

void pccnprobe(struct consdev *);
void pccninit(struct consdev *);
void pccnputc(dev_t, int);
int pccngetc(dev_t);
void pccnpollc(dev_t, int);

#if (NPCCONSKBD == 0)

#define	KBD_DELAY \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; }

static inline int
kbd_wait_output(void)
{
	u_int i;

	for (i = 100000; i; i--)
		if ((inb(IO_KBD + KBSTATP) & KBS_IBF) == 0) {
			KBD_DELAY;
			return (1);
		}
	return (0);
}

static inline int
kbd_wait_input(void)
{
	u_int i;

	for (i = 100000; i; i--)
		if ((inb(IO_KBD + KBSTATP) & KBS_DIB) != 0) {
			KBD_DELAY;
			return (1);
		}
	return (0);
}

static inline void
kbd_flush_input(void)
{
	u_int i;

	for (i = 10; i; i--) {
		if ((inb(IO_KBD + KBSTATP) & KBS_DIB) == 0)
			return;
		KBD_DELAY;
		(void) inb(IO_KBD + KBDATAP);
	}
}

#if 1
/*
 * Get the current command byte.
 */
static u_char
kbc_get8042cmd(void)
{

	if (!kbd_wait_output())
		return (-1);
	outb(IO_KBD + KBCMDP, K_RDCMDBYTE);
	if (!kbd_wait_input())
		return (-1);
	return (inb(IO_KBD + KBDATAP));
}
#endif

/*
 * Pass command byte to keyboard controller (8042).
 */
static int
kbc_put8042cmd(u_char val)
{

	if (!kbd_wait_output())
		return (0);
	outb(IO_KBD + KBCMDP, K_LDCMDBYTE);
	if (!kbd_wait_output())
		return (0);
	outb(IO_KBD + KBOUTP, val);
	return (1);
}

/*
 * Pass command to keyboard itself
 */
int
kbd_cmd(u_char val, u_char dopoll)
{
	u_int retries = 3;
	register u_int i;

	do {
		if (!kbd_wait_output())
			return (0);
		ack = nak = 0;
		outb(IO_KBD + KBOUTP, val);
		if (dopoll)
			for (i = 100000; i; i--) {
				if (inb(IO_KBD + KBSTATP) & KBS_DIB) {
					register u_char c;

					KBD_DELAY;
					c = inb(IO_KBD + KBDATAP);
					if (c == KBR_ACK || c == KBR_ECHO) {
						ack = 1;
						return (1);
					}
					if (c == KBR_RESEND) {
						nak = 1;
						break;
					}
#ifdef DIAGNOSTIC
					printf("kbd_cmd: input char %x lost\n", c);
#endif
				}
			}
		else
			for (i = 100000; i; i--) {
				(void) inb(IO_KBD + KBSTATP);
				if (ack)
					return (1);
				if (nak)
					break;
			}
		if (!nak)
			return (0);
	} while (--retries);
	return (0);
}

#endif /* NPCCONSKBD == 0 */

void
set_cursor_shape(void)
{
	register int iobase = addr_6845;

	outb(iobase, 10);
	outb(iobase+1, cursor_shape >> 8);
	outb(iobase, 11);
	outb(iobase+1, cursor_shape);
	old_cursor_shape = cursor_shape;
}

#ifdef XSERVER
#ifdef XFREE86_BUG_COMPAT
void
get_cursor_shape(void)
{
	register int iobase = addr_6845;

	outb(iobase, 10);
	cursor_shape = inb(iobase+1) << 8;
	outb(iobase, 11);
	cursor_shape |= inb(iobase+1);

	/*
	 * real 6845's, as found on, MDA, Hercules or CGA cards, do
	 * not support reading the cursor shape registers. the 6845
	 * tri-states it's data bus. This is _normally_ read by the
	 * CPU as either 0x00 or 0xff.. in which case we just use
	 * a line cursor.
	 */
	if (cursor_shape == 0x0000 || cursor_shape == 0xffff)
		cursor_shape = 0x0b10;
	else
		cursor_shape &= 0x1f1f;
}
#endif /* XFREE86_BUG_COMPAT */
#endif /* XSERVER */

void
do_async_update(void *v)
{
#if (NPCCONSKBD == 0)
	u_char poll = v ? 1 : 0;
#endif
	int pos;
	static int old_pos = -1;

	async = 0;

#if (NPCCONSKBD == 0)
	if (lock_state != old_lock_state) {
		old_lock_state = lock_state;
		if (!kbd_cmd(KBC_MODEIND, poll) ||
		    !kbd_cmd(lock_state, poll)) {
			printf("pc: timeout updating leds\n");
			(void) kbd_cmd(KBC_ENABLE, poll);
		}
	}
	if (typematic_rate != old_typematic_rate) {
		old_typematic_rate = typematic_rate;
		if (!kbd_cmd(KBC_TYPEMATIC, poll) ||
		    !kbd_cmd(typematic_rate, poll)) {
			printf("pc: timeout updating typematic rate\n");
			(void) kbd_cmd(KBC_ENABLE, poll);
		}
	}
#else
	/*
	 * If the mi pckbport driver is used, keyboard commands are handled
	 * there. The commands are issued synchronously (in update_leds()
	 * and pcioctl()).
	 */
#endif

#ifdef XSERVER
	if (pc_xmode > 0)
		return;
#endif

	pos = crtat - Crtat;
	if (pos != old_pos) {
		register int iobase = addr_6845;
		outb(iobase, 14);
		outb(iobase+1, pos >> 8);
		outb(iobase, 15);
		outb(iobase+1, pos);
		old_pos = pos;
	}
	if (cursor_shape != old_cursor_shape)
		set_cursor_shape();
}

void
async_update(void)
{

	if (kernel || polling) {
		if (async)
			callout_stop(&async_update_ch);
		do_async_update((void *)1);
	} else {
		if (async)
			return;
		async = 1;
		callout_reset(&async_update_ch, 1, do_async_update, NULL);
	}
}

#if (NPCCONSKBD > 0)
void update_leds()
{
	u_char cmd[2];

	cmd[0] = KBC_MODEIND;
	cmd[1] = lock_state & 7;

	pckbport_enqueue_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
}
#endif

/*
 * these are both bad jokes
 */
int
pcprobe(struct device *parent, struct cfdata *match,
	void *aux)
{
	struct isa_attach_args *ia = aux;
#if (NPCCONSKBD == 0)
	u_int i;
#else
	u_char cmd[2], resp[1];
	int res;
#endif

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	/*
	 * XXXJRT This is probably wrong, but then again, pccons is a
	 * XXXJRT total hack to begin with.
	 */
	if (ISA_DIRECT_CONFIG(ia))
		return (0);

#if (NPCCONSKBD == 0)
	/* Enable interrupts and keyboard, etc. */
	if (!kbc_put8042cmd(CMDBYTE)) {
		printf("pcprobe: command error\n");
		return (0);
	}
#else
	if (!kbc_attached) {
		printf("pcprobe: no keyboard\n");
		return (0);
	}
#endif

#if 1
	/* Flush any garbage. */
#if (NPCCONSKBD == 0)
	kbd_flush_input();
#else
	pckbport_flush(kbctag, kbcslot);
#endif
	/* Reset the keyboard. */
#if (NPCCONSKBD == 0)
	if (!kbd_cmd(KBC_RESET, 1)) {
		printf("pcprobe: reset error %d\n", 1);
		goto lose;
	}
	for (i = 600000; i; i--)
		if ((inb(IO_KBD + KBSTATP) & KBS_DIB) != 0) {
			KBD_DELAY;
			break;
		}
	if (i == 0 || inb(IO_KBD + KBDATAP) != KBR_RSTDONE) {
		printf("pcprobe: reset error %d\n", 2);
		goto lose;
	}
#else
	cmd[0] = KBC_RESET;
	res = pckbport_poll_cmd(kbctag, kbcslot, cmd, 1, 1, resp, 1);
	if (res) {
		printf("pcprobe: reset error %d\n", 1);
		/*
		 * XXX The keyboard is not present. Try to set the
		 * controller to "translating" anyway in case it is
		 * connected later. This should be done in attach().
		 */
		(void) pckbport_xt_translation(kbctag, kbcslot, 1);
		goto lose;
	}
	if (resp[0] != KBR_RSTDONE) {
		printf("pcprobe: reset error %d\n", 2);
		goto lose;
	}
#endif
	/*
	 * Some keyboards seem to leave a second ack byte after the reset.
	 * This is kind of stupid, but we account for them anyway by just
	 * flushing the buffer.
	 */
#if (NPCCONSKBD == 0)
	kbd_flush_input();
#else
	pckbport_flush(kbctag, kbcslot);
#endif
	/* Just to be sure. */
#if (NPCCONSKBD == 0)
	if (!kbd_cmd(KBC_ENABLE, 1)) {
		printf("pcprobe: reset error %d\n", 3);
		goto lose;
	}
#else
	cmd[0] = KBC_ENABLE;
	res = pckbport_poll_cmd(kbctag, kbcslot, cmd, 1, 0, 0, 0);
	if (res) {
		printf("pcprobe: reset error %d\n", 3);
		goto lose;
	}
#endif

	/*
	 * Some keyboard/8042 combinations do not seem to work if the keyboard
	 * is set to table 1; in fact, it would appear that some keyboards just
	 * ignore the command altogether.  So by default, we use the AT scan
	 * codes and have the 8042 translate them.  Unfortunately, this is
	 * known to not work on some PS/2 machines.  We try desparately to deal
	 * with this by checking the (lack of a) translate bit in the 8042 and
	 * attempting to set the keyboard to XT mode.  If this all fails, well,
	 * tough luck.
	 *
	 * XXX It would perhaps be a better choice to just use AT scan codes
	 * and not bother with this.
	 */
#if (NPCCONSKBD == 0)
	if (kbc_get8042cmd() & KC8_TRANS) {
		/* The 8042 is translating for us; use AT codes. */
		if (!kbd_cmd(KBC_SETTABLE, 1) || !kbd_cmd(2, 1)) {
			printf("pcprobe: reset error %d\n", 4);
			goto lose;
		}
	} else {
		/* Stupid 8042; set keyboard to XT codes. */
		if (!kbd_cmd(KBC_SETTABLE, 1) || !kbd_cmd(1, 1)) {
			printf("pcprobe: reset error %d\n", 5);
			goto lose;
		}
	}
#else
	if (pckbport_xt_translation(kbctag, kbcslot, 1)) {
		/* The 8042 is translating for us; use AT codes. */
		cmd[0] = KBC_SETTABLE;
		cmd[1] = 2;
		res = pckbport_poll_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
		if (res) {
			printf("pcprobe: reset error %d\n", 4);
			goto lose;
		}
	} else {
		/* Stupid 8042; set keyboard to XT codes. */
		cmd[0] = KBC_SETTABLE;
		cmd[1] = 1;
		res = pckbport_poll_cmd(kbctag, kbcslot, cmd, 2, 0, 0, 0);
		if (res) {
			printf("pcprobe: reset error %d\n", 5);
			goto lose;
		}
	}
#endif

lose:
	/*
	 * Technically, we should probably fail the probe.  But we'll be nice
	 * and allow keyboard-less machines to boot with the console.
	 */
#endif /* 1 */

#if (NPCCONSKBD > 0)
	ia->ia_nio = 0;
	ia->ia_nirq = 0;
#else
	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = 16;
	ia->ia_nirq = 1;
#endif

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;

	return (1);
}

void
pcattach(struct device *parent, struct device *self, void *aux)
{
	struct pc_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;

	if (crtat == 0)
		pcinit();

	printf(": %s\n", vs.color ? "color" : "mono");
	do_async_update((void *)1);

#if (NPCCONSKBD > 0)
	pckbport_set_inputhandler(kbctag, kbcslot, pcinput, sc, sc->sc_dev.dv_xname);
#else
	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
	    IST_EDGE, IPL_TTY, pcintr, sc);
#endif

	/*
	 * Reserve CRTC I/O ports to keep other devices such as PCMCIA
	 * from using them.
	 */
	if (bus_space_map(ia->ia_iot, addr_6845, 0x2, 0, &ioh))
		printf("pc: mapping of CRTC registers failed\n");

	if (vs.color)
		if (bus_space_map(ia->ia_iot, 0x3C0, 0x10, 0, &ioh))
			printf("pc: mapping of VGA registers failed\n");

	if (pccons_is_console) {
		int maj;

		/* Locate the major number. */
		maj = cdevsw_lookup_major(&pc_cdevsw);

		/* There can be only one, but it can have any unit number. */
		cn_tab->cn_dev = makedev(maj, device_unit(&sc->sc_dev));

		printf("%s: console\n", sc->sc_dev.dv_xname);
	}
}

#if (NPCCONSKBD > 0)
int
pcconskbdprobe(struct device *parent, struct cfdata *match, void *aux)
{
	struct pckbport_attach_args *pka = aux;

	if (pka->pa_slot != PCKBPORT_KBD_SLOT)
		return (0);
	return (1);
}

void
pcconskbdattach(struct device *parent, struct device *self, void *aux)
{
	struct pckbport_attach_args *pka = aux;

	printf("\n");

	kbctag = pka->pa_tag;
	kbcslot = pka->pa_slot;
	kbc_attached = 1;
}

int
pcconskbd_cnattach(pckbport_tag_t tag, pckbport_slot_t slot)
{
	kbctag = tag;
	kbcslot = slot;
	kbc_attached = 1;
	return (0);
}
#endif

int
pcopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pc_softc *sc;
	int unit = PCUNIT(dev);
	struct tty *tp;

	if (unit >= pc_cd.cd_ndevs)
		return (ENXIO);
	sc = pc_cd.cd_devs[unit];
	if (sc == 0)
		return (ENXIO);

	if (!sc->sc_tty) {
		tp = sc->sc_tty = ttymalloc();
		tty_attach(tp);
	} else
		tp = sc->sc_tty;

	tp->t_oproc = pcstart;
	tp->t_param = pcparam;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		pcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;

	return ((*tp->t_linesw->l_open)(dev, tp));
}

int
pcclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	if (tp == NULL)
		return (0);
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
#ifdef notyet /* XXX */
	ttyfree(tp);
#endif
	return (0);
}

int
pcread(dev_t dev, struct uio *uio, int flag)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
pcwrite(dev_t dev, struct uio *uio, int flag)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
pcpoll(dev_t dev, int events, struct lwp *l)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
pctty(dev_t dev)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}

/*
 * Got a console receive interrupt -
 * the console processor wants to give us a character.
 * Catch the character, and see who it goes to.
 */
#if (NPCCONSKBD > 0)
void
pcinput(void *arg, int data)
{
	struct pc_softc *sc = arg;
	register struct tty *tp = sc->sc_tty;
	u_char *cp;

	if (!tp || (tp->t_state & TS_ISOPEN) == 0)
		return;

	cp = strans(data);
	if (cp)
		do
			(*tp->t_linesw->l_rint)(*cp++, tp);
		while (*cp);
}
#else
int
pcintr(void *arg)
{
	struct pc_softc *sc = arg;
	register struct tty *tp = sc->sc_tty;
	u_char *cp;

	if ((inb(IO_KBD + KBSTATP) & KBS_DIB) == 0)
		return (0);
	do {
		cp = sget();

		if (polling) {
			poll_data = *cp;
			return (1);
		}
		if (!tp || (tp->t_state & TS_ISOPEN) == 0)
			return (1);
		if (cp)
			do
				(*tp->t_linesw->l_rint)(*cp++, tp);
			while (*cp);
	} while (inb(IO_KBD + KBSTATP) & KBS_DIB);
	return (1);
}
#endif

int
pcioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {
#ifdef XSERVER
	case CONSOLE_X_MODE_ON:
		pc_xmode_on();
		ttyflush(tp, FREAD);
		return (0);
	case CONSOLE_X_MODE_OFF:
		pc_xmode_off();
		ttyflush(tp, FREAD);
		return (0);
	case CONSOLE_X_BELL:
		/*
		 * If set, data is a pointer to a length 2 array of
		 * integers.  data[0] is the pitch in Hz and data[1]
		 * is the duration in msec.
		 */
		if (data)
			sysbeep(((int*)data)[0],
				(((int*)data)[1] * hz) / 1000);
		else
			sysbeep(BEEP_FREQ, BEEP_TIME);
		return (0);
#endif /* XSERVER */
	case CONSOLE_SET_TYPEMATIC_RATE: {
 		u_char	rate;

 		if (!data)
			return (EINVAL);
		rate = *((u_char *)data);
		/*
		 * Check that it isn't too big (which would cause it to be
		 * confused with a command).
		 */
		if (rate & 0x80)
			return (EINVAL);
#if (NPCCONSKBD > 0)
		{
			u_char cmd[2];

			cmd[0] = KBC_TYPEMATIC;
			cmd[1] = rate;

			return (pckbport_enqueue_cmd(kbctag, kbcslot, cmd, 2, 0,
						  1, 0));
		}
#else
		typematic_rate = rate;
		async_update();
		return (0);
#endif
 	}
	default:
		return (EPASSTHROUGH);
	}

#ifdef DIAGNOSTIC
	panic("pcioctl: impossible");
#endif
}

void
pcstart(struct tty *tp)
{
	struct clist *cl;
	int s, len;
	u_char buf[PCBURST];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	splx(s);

	lock_state &= ~SCROLL;

	/*
	 * We need to do this outside spl since it could be fairly
	 * expensive and we don't want our serial ports to overflow.
	 */
	cl = &tp->t_outq;
	len = q_to_b(cl, buf, PCBURST);
	sput(buf, len);

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (cl->c_cc) {
		tp->t_state |= TS_TIMEOUT;
		callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, tp);
	}
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(cl);
		}
		selwakeup(&tp->t_wsel);
	}
out:
	splx(s);
}

void
pcstop(struct tty *tp, int flag)
{

	lock_state |= SCROLL;
	async_update();
}

int
pccnattach(void)
{
	static struct consdev pccons = {
		NULL, NULL, pccngetc, pccnputc, pccnpollc, NULL, NULL,
		    NULL, NODEV, CN_NORMAL
	};

	cn_tab = &pccons;

	pccons_is_console = 1;

	return (0);
}

/* ARGSUSED */
void
pccnputc(dev_t dev, int c)
{
	u_char oldkernel = kernel;
	char help = c;

	kernel = 1;
	if (help == '\n')
		sput("\r\n", 2);
	else
		sput(&help, 1);
	kernel = oldkernel;
}

/*
 * Note: the spl games here are to deal with some strange PC kbd controllers
 * in some system configurations.
 * This is not canonical way to handle polling input.
 */
/* ARGSUSED */
int
pccngetc(dev_t dev)
{
	register char *cp;

#ifdef XSERVER
	if (pc_xmode > 0)
		return (0);
#endif

	do {
		/* wait for byte */
#if (NPCCONSKBD == 0)
		int s = splhigh();

		if (poll_data != -1) {
			int data = poll_data;
			poll_data = -1;
			splx(s);
			return (data);
		}

		while ((inb(IO_KBD + KBSTATP) & KBS_DIB) == 0);

		/* see if it's worthwhile */
		cp = sget();
		splx(s);
#else
		int data;
		do {
			data = pckbport_poll_data(kbctag, kbcslot);
		} while (data == -1);
		cp = strans(data);
#endif
	} while (!cp);
	if (*cp == '\r')
		return ('\n');
	return (*cp);
}

void
pccnpollc(dev_t dev, int on)
{

	polling = on;
#if (NPCCONSKBD > 0)
	pckbport_set_poll(kbctag, kbcslot, on);
#else
	if (on)
		poll_data = -1;
	else {
		int unit;
		struct pc_softc *sc;
		int s;

		/*
		 * If disabling polling on a device that's been configured,
		 * make sure there are no bytes left in the FIFO, holding up
		 * the interrupt line.  Otherwise we won't get any further
		 * interrupts.
		 */
		unit = PCUNIT(dev);
		if (pc_cd.cd_ndevs > unit) {
			sc = pc_cd.cd_devs[unit];
			if (sc != 0) {
				s = spltty();
				pcintr(sc);
				splx(s);
			}
		}
	}
#endif
}	

/*
 * Set line parameters.
 */
int
pcparam(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return (0);
}

#ifndef	PCCONS_DEFAULT_FG
#define	PCCONS_DEFAULT_FG	FG_LIGHTGREY
#endif
#ifndef	PCCONS_DEFAULT_SO_FG
#define	PCCONS_DEFAULT_SO_FG	FG_YELLOW
#endif

#ifndef	PCCONS_DEFAULT_BG
#define	PCCONS_DEFAULT_BG	BG_BLACK
#endif
#ifndef	PCCONS_DEFAULT_SO_BG
#define	PCCONS_DEFAULT_SO_BG	BG_BLACK
#endif

void
pcinit(void)
{
	u_short volatile *cptest;
	u_short *cp;
	u_short was;
	unsigned cursorat;

	cptest = cp = ISA_HOLE_VADDR(CGA_BUF);
	was = *cptest;
	*cptest = (u_short) 0xA55A;
	if (*cptest != 0xA55A) {
		cptest = cp = ISA_HOLE_VADDR(MONO_BUF);
		addr_6845 = MONO_BASE;
		vs.color = 0;
	} else {
		*cptest = was;
		addr_6845 = CGA_BASE;
		vs.color = 1;
	}

	/* Extract cursor location */
	outb(addr_6845, 14);
	cursorat = inb(addr_6845+1) << 8;
	outb(addr_6845, 15);
	cursorat |= inb(addr_6845+1);

	if (cursorat > COL * ROW)
		cursorat = 0;

#ifdef FAT_CURSOR
	cursor_shape = 0x0012;
#endif

	Crtat = (u_short *)cp;
	crtat = (u_short *)(cp + cursorat);

	vs.ncol = COL;
	vs.nrow = ROW;
	vs.nchr = COL * ROW;

	vs.at = PCCONS_DEFAULT_FG | PCCONS_DEFAULT_BG;
	if (vs.color == 0)
		vs.so_at = FG_BLACK | BG_LIGHTGREY;
	else
		vs.so_at = PCCONS_DEFAULT_SO_FG | PCCONS_DEFAULT_SO_BG;

	fillw((vs.at << 8) | ' ', crtat, vs.nchr - cursorat);
}

#define	wrtchar(c, at) \
    do { \
	    char *_cp = (char *)crtat; \
	    *_cp++ = (c); \
	    *_cp = (at); \
	    crtat++; \
	    vs.col++; \
    } while (/*CONSTCOND*/0)

/* translate ANSI color codes to standard pc ones */
static const char fgansitopc[] = {
	FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
	FG_MAGENTA, FG_CYAN, FG_LIGHTGREY
};

static const char bgansitopc[] = {
	BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
	BG_MAGENTA, BG_CYAN, BG_LIGHTGREY
};

#ifdef DISPLAY_ISO8859
static const u_char iso2ibm437[] =
{
           0,     0,     0,     0,     0,     0,     0,     0,
           0,     0,     0,     0,     0,     0,     0,     0,
           0,     0,     0,     0,     0,     0,     0,     0,
           0,     0,     0,     0,     0,     0,     0,     0,
        0xff,  0xad,  0x9b,  0x9c,     0,  0x9d,     0,  0x40,
        0x6f,  0x63,  0x61,  0xae,     0,     0,     0,     0,
        0xf8,  0xf1,  0xfd,  0x33,     0,  0xe6,     0,  0xfa,
           0,  0x31,  0x6f,  0xaf,  0xac,  0xab,     0,  0xa8,
        0x41,  0x41,  0x41,  0x41,  0x8e,  0x8f,  0x92,  0x80,
        0x45,  0x90,  0x45,  0x45,  0x49,  0x49,  0x49,  0x49,
        0x81,  0xa5,  0x4f,  0x4f,  0x4f,  0x4f,  0x99,  0x4f,
        0x4f,  0x55,  0x55,  0x55,  0x9a,  0x59,     0,  0xe1,
        0x85,  0xa0,  0x83,  0x61,  0x84,  0x86,  0x91,  0x87,
        0x8a,  0x82,  0x88,  0x89,  0x8d,  0xa1,  0x8c,  0x8b,
           0,  0xa4,  0x95,  0xa2,  0x93,  0x6f,  0x94,  0x6f,
        0x6f,  0x97,  0xa3,  0x96,  0x81,  0x98,     0,     0
};
#endif

/*
 * `pc3' termcap emulation.
 */
void
sput(const u_char *cp, int n)
{
	u_char c, scroll = 0;

#ifdef XSERVER
	if (pc_xmode > 0)
		return;
#endif

	if (crtat == 0)
		pcinit();

	while (n--) {
		if (!(c = *cp++))
			continue;

		switch (c) {
		case 0x1B:
			if (vs.state >= VSS_ESCAPE) {
				wrtchar(c, vs.so_at); 
				vs.state = 0;
				goto maybe_scroll;
			} else
				vs.state = VSS_ESCAPE;
			break;

		case '\t': {
			int inccol = 8 - (vs.col & 7);
			crtat += inccol;
			vs.col += inccol;
		}
		maybe_scroll:
			if (vs.col >= COL) {
				vs.col -= COL;
				scroll = 1;
			}
			break;

		case '\010':
			if (crtat <= Crtat)
				break;
			--crtat;
			if (--vs.col < 0)
				vs.col += COL;	/* non-destructive backspace */
			break;

		case '\r':
			crtat -= vs.col;
			vs.col = 0;
			break;

		case '\n':
			crtat += vs.ncol;
			scroll = 1;
			break;

		default:
			switch (vs.state) {
			case 0:
				if (c == '\a')
					sysbeep(BEEP_FREQ, BEEP_TIME);
				else {
					/*
					 * If we're outputting multiple printed
					 * characters, just blast them to the
					 * screen until we reach the end of the
					 * buffer or a control character.  This
					 * saves time by short-circuiting the
					 * switch.
					 * If we reach the end of the line, we
					 * break to do a scroll check.
					 */
					for (;;) {
#ifdef DISPLAY_ISO8859
				               if (c & 0x80) 
						        c = iso2ibm437[c&0x7f];
#endif
						if (vs.so)
							wrtchar(c, vs.so_at);
						else
							wrtchar(c, vs.at);
						if (vs.col >= vs.ncol) {
							vs.col = 0;
							scroll = 1;
							break;
						}
						if (!n || (c = *cp) < ' ')
							break;
						n--, cp++;
					}
				}
				break;
			case VSS_ESCAPE:
				if (c == '[') {	/* Start ESC [ sequence */
					vs.cx = vs.cy = 0;
					vs.state = VSS_EBRACE;
				} else if (c == 'c') { /* Clear screen & home */
					fillw((vs.at << 8) | ' ',
					    Crtat, vs.nchr);
					crtat = Crtat;
					vs.col = 0;
					vs.state = 0;
				} else { /* Invalid, clear state */
					wrtchar(c, vs.so_at); 
					vs.state = 0;
					goto maybe_scroll;
				}
				break;
			default: /* VSS_EBRACE or VSS_EPARAM */
				switch (c) {
					int pos;
				case 'm':
					if (!vs.cx)
						vs.so = 0;
					else
						vs.so = 1;
					vs.state = 0;
					break;
				case 'A': { /* back cx rows */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.nrow;
					pos = crtat - Crtat;
					pos -= vs.ncol * cx;
					if (pos < 0)
						pos += vs.nchr;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'B': { /* down cx rows */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.nrow;
					pos = crtat - Crtat;
					pos += vs.ncol * cx;
					if (pos >= vs.nchr) 
						pos -= vs.nchr;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'C': { /* right cursor */
					int cx = vs.cx,
					    col = vs.col;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.ncol;
					pos = crtat - Crtat;
					pos += cx;
					col += cx;
					if (col >= vs.ncol) {
						pos -= vs.ncol;
						col -= vs.ncol;
					}
					vs.col = col;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'D': { /* left cursor */
					int cx = vs.cx,
					    col = vs.col;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.ncol;
					pos = crtat - Crtat;
					pos -= cx;
					col -= cx;
					if (col < 0) {
						pos += vs.ncol;
						col += vs.ncol;
					}
					vs.col = col;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'J': /* Clear ... */
					switch (vs.cx) {
					case 0:
						/* ... to end of display */
						fillw((vs.at << 8) | ' ', 
						    crtat,
						    Crtat + vs.nchr - crtat);
						break;
					case 1:
						/* ... to next location */
						fillw((vs.at << 8) | ' ',
						    Crtat, crtat - Crtat + 1);
						break;
					case 2:
						/* ... whole display */
						fillw((vs.at << 8) | ' ',
						    Crtat, vs.nchr);
						break;
					}
					vs.state = 0;
					break;
				case 'K': /* Clear line ... */
					switch (vs.cx) {
					case 0:
						/* ... current to EOL */
						fillw((vs.at << 8) | ' ',
						    crtat, vs.ncol - vs.col);
						break;
					case 1:
						/* ... beginning to next */
						fillw((vs.at << 8) | ' ',
						    crtat - vs.col, vs.col + 1);
						break;
					case 2:
						/* ... entire line */
						fillw((vs.at << 8) | ' ',
						    crtat - vs.col, vs.ncol);
						break;
					}
					vs.state = 0;
					break;
				case 'f': /* in system V consoles */
				case 'H': { /* Cursor move */
					int cx = vs.cx,
					    cy = vs.cy;
					if (!cx || !cy) {
						crtat = Crtat;
						vs.col = 0;
					} else {
						if (cx > vs.nrow)
							cx = vs.nrow;
						if (cy > vs.ncol)
							cy = vs.ncol;
						crtat = Crtat +
						    (cx - 1) * vs.ncol + cy - 1;
						vs.col = cy - 1;
					}
					vs.state = 0;
					break;
				}
				case 'M': { /* delete cx rows */
					u_short *crtAt = crtat - vs.col;
					int cx = vs.cx,
					    row = (crtAt - Crtat) / vs.ncol,
					    nrow = vs.nrow - row;
					if (cx <= 0)
						cx = 1;
					else if (cx > nrow)
						cx = nrow;
					if (cx < nrow)
						memmove(crtAt,
						    crtAt + vs.ncol * cx,
						    vs.ncol *
						    (nrow - cx) * CHR);
					fillw((vs.at << 8) | ' ',
					    crtAt + vs.ncol * (nrow - cx),
					    vs.ncol * cx);
					vs.state = 0;
					break;
				}
				case 'S': { /* scroll up cx lines */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else if (cx > vs.nrow)
						cx = vs.nrow;
					if (cx < vs.nrow)
						memmove(Crtat,
						    Crtat + vs.ncol * cx,
						    vs.ncol *
						    (vs.nrow - cx) * CHR);
					fillw((vs.at << 8) | ' ',
					    Crtat + vs.ncol * (vs.nrow - cx),
					    vs.ncol * cx);
#if 0
					crtat -= vs.ncol * cx; /* XXX */
#endif
					vs.state = 0;
					break;
				}
				case 'L': { /* insert cx rows */
					u_short *crtAt = crtat - vs.col;
					int cx = vs.cx,
					    row = (crtAt - Crtat) / vs.ncol,
					    nrow = vs.nrow - row;
					if (cx <= 0)
						cx = 1;
					else if (cx > nrow)
						cx = nrow;
					if (cx < nrow)
						memmove(crtAt + vs.ncol * cx,
						    crtAt,
						    vs.ncol * (nrow - cx) *
						    CHR);
					fillw((vs.at << 8) | ' ', 
					    crtAt, vs.ncol * cx);
					vs.state = 0;
					break;
				}
				case 'T': { /* scroll down cx lines */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else if (cx > vs.nrow)
						cx = vs.nrow;
					if (cx < vs.nrow)
						memmove(Crtat + vs.ncol * cx,
						    Crtat,
						    vs.ncol * (vs.nrow - cx) *
						    CHR);
					fillw((vs.at << 8) | ' ', 
					    Crtat, vs.ncol * cx);
#if 0
					crtat += vs.ncol * cx; /* XXX */
#endif
					vs.state = 0;
					break;
				}
				case ';': /* Switch params in cursor def */
					vs.state = VSS_EPARAM;
					break;
				case 'r':
					vs.so_at = (vs.cx & FG_MASK) |
					    ((vs.cy << 4) & BG_MASK);
					vs.state = 0;
					break;
				case 'x': /* set attributes */
					switch (vs.cx) {
					case 0:
						vs.at = PCCONS_DEFAULT_FG |
						    PCCONS_DEFAULT_BG;
						break;
					case 1:
						/* ansi background */
						if (!vs.color)
							break;
						vs.at &= FG_MASK;
						vs.at |= bgansitopc[vs.cy & 7];
						break;
					case 2:
						/* ansi foreground */
						if (!vs.color)
							break;
						vs.at &= BG_MASK;
						vs.at |= fgansitopc[vs.cy & 7];
						break;
					case 3:
						/* pc text attribute */
						if (vs.state >= VSS_EPARAM)
							vs.at = vs.cy;
						break;
					}
					vs.state = 0;
					break;
					
				default: /* Only numbers valid here */
					if ((c >= '0') && (c <= '9')) {
						if (vs.state >= VSS_EPARAM) {
							vs.cy *= 10;
							vs.cy += c - '0';
						} else {
							vs.cx *= 10;
							vs.cx += c - '0';
						}
					} else
						vs.state = 0;
					break;
				}
				break;
			}
		}
		if (scroll) {
			scroll = 0;
			/* scroll check */
			if (crtat >= Crtat + vs.nchr) {
				memmove(Crtat, Crtat + vs.ncol,
				    (vs.nchr - vs.ncol) * CHR);
				fillw((vs.at << 8) | ' ',
				    Crtat + vs.nchr - vs.ncol,
				    vs.ncol);
				crtat -= vs.ncol;
			}
		}
	}
	async_update();
}

#define	CODE_SIZE	4		/* Use a max of 4 for now... */
#ifndef NONUS_KBD
typedef struct {
	u_short	type;
	char unshift[CODE_SIZE];
	char shift[CODE_SIZE];
	char ctl[CODE_SIZE];
} Scan_def;

static Scan_def	scan_codes[] = {
	{ NONE,	"",		"",		"" },		/* 0 unused */
	{ ASCII,"\033",		"\033",		"\033" },	/* 1 ESCape */
	{ ASCII,"1",		"!",		"!" },		/* 2 1 */
	{ ASCII,"2",		"@",		"\000" },	/* 3 2 */
	{ ASCII,"3",		"#",		"#" },		/* 4 3 */
	{ ASCII,"4",		"$",		"$" },		/* 5 4 */
	{ ASCII,"5",		"%",		"%" },		/* 6 5 */
	{ ASCII,"6",		"^",		"\036" },	/* 7 6 */
	{ ASCII,"7",		"&",		"&" },		/* 8 7 */
	{ ASCII,"8",		"*",		"\010" },	/* 9 8 */
	{ ASCII,"9",		"(",		"(" },		/* 10 9 */
	{ ASCII,"0",		")",		")" },		/* 11 0 */
	{ ASCII,"-",		"_",		"\037" },	/* 12 - */
	{ ASCII,"=",		"+",		"+" },		/* 13 = */
#ifndef PCCONS_REAL_BS
	{ ASCII,"\177",		"\177",		"\010" },	/* 14 backspace */
#else
	{ ASCII,"\010",		"\010",		"\177" },	/* 14 backspace */
#endif
	{ ASCII,"\t",		"\177\t",	"\t" },		/* 15 tab */
	{ ASCII,"q",		"Q",		"\021" },	/* 16 q */
	{ ASCII,"w",		"W",		"\027" },	/* 17 w */
	{ ASCII,"e",		"E",		"\005" },	/* 18 e */
	{ ASCII,"r",		"R",		"\022" },	/* 19 r */
	{ ASCII,"t",		"T",		"\024" },	/* 20 t */
	{ ASCII,"y",		"Y",		"\031" },	/* 21 y */
	{ ASCII,"u",		"U",		"\025" },	/* 22 u */
	{ ASCII,"i",		"I",		"\011" },	/* 23 i */
	{ ASCII,"o",		"O",		"\017" },	/* 24 o */
	{ ASCII,"p",		"P",		"\020" },	/* 25 p */
	{ ASCII,"[",		"{",		"\033" },	/* 26 [ */
	{ ASCII,"]",		"}",		"\035" },	/* 27 ] */
	{ ASCII,"\r",		"\r",		"\n" },		/* 28 return */
#ifdef CAPS_IS_CONTROL
	{ CAPS,	"",		"",		"" },		/* 29 caps */
#else
	{ CTL,	"",		"",		"" },		/* 29 control */
#endif
	{ ASCII,"a",		"A",		"\001" },	/* 30 a */
	{ ASCII,"s",		"S",		"\023" },	/* 31 s */
	{ ASCII,"d",		"D",		"\004" },	/* 32 d */
	{ ASCII,"f",		"F",		"\006" },	/* 33 f */
	{ ASCII,"g",		"G",		"\007" },	/* 34 g */
	{ ASCII,"h",		"H",		"\010" },	/* 35 h */
	{ ASCII,"j",		"J",		"\n" },		/* 36 j */
	{ ASCII,"k",		"K",		"\013" },	/* 37 k */
	{ ASCII,"l",		"L",		"\014" },	/* 38 l */
	{ ASCII,";",		":",		";" },		/* 39 ; */
	{ ASCII,"'",		"\"",		"'" },		/* 40 ' */
	{ ASCII,"`",		"~",		"`" },		/* 41 ` */
	{ SHIFT,"",		"",		"" },		/* 42 shift */
	{ ASCII,"\\",		"|",		"\034" },	/* 43 \ */
	{ ASCII,"z",		"Z",		"\032" },	/* 44 z */
	{ ASCII,"x",		"X",		"\030" },	/* 45 x */
	{ ASCII,"c",		"C",		"\003" },	/* 46 c */
	{ ASCII,"v",		"V",		"\026" },	/* 47 v */
	{ ASCII,"b",		"B",		"\002" },	/* 48 b */
	{ ASCII,"n",		"N",		"\016" },	/* 49 n */
	{ ASCII,"m",		"M",		"\r" },		/* 50 m */
	{ ASCII,",",		"<",		"<" },		/* 51 , */
	{ ASCII,".",		">",		">" },		/* 52 . */
	{ ASCII,"/",		"?",		"\037" },	/* 53 / */
	{ SHIFT,"",		"",		"" },		/* 54 shift */
	{ KP,	"*",		"*",		"*" },		/* 55 kp * */
	{ ALT,	"",		"",		"" },		/* 56 alt */
	{ ASCII," ",		" ",		"\000" },	/* 57 space */
#ifdef CAPS_IS_CONTROL
	{ CTL,	"",		"",		"" },		/* 58 control */
#else
	{ CAPS,	"",		"",		"" },		/* 58 caps */
#endif
	{ FUNC,	"\033[M",	"\033[Y",	"\033[k" },	/* 59 f1 */
	{ FUNC,	"\033[N",	"\033[Z",	"\033[l" },	/* 60 f2 */
	{ FUNC,	"\033[O",	"\033[a",	"\033[m" },	/* 61 f3 */
	{ FUNC,	"\033[P",	"\033[b",	"\033[n" },	/* 62 f4 */
	{ FUNC,	"\033[Q",	"\033[c",	"\033[o" },	/* 63 f5 */
	{ FUNC,	"\033[R",	"\033[d",	"\033[p" },	/* 64 f6 */
	{ FUNC,	"\033[S",	"\033[e",	"\033[q" },	/* 65 f7 */
	{ FUNC,	"\033[T",	"\033[f",	"\033[r" },	/* 66 f8 */
	{ FUNC,	"\033[U",	"\033[g",	"\033[s" },	/* 67 f9 */
	{ FUNC,	"\033[V",	"\033[h",	"\033[t" },	/* 68 f10 */
	{ NUM,	"",		"",		"" },		/* 69 num lock */
	{ SCROLL,"",		"",		"" },		/* 70 scroll lock */
	{ KP,	"7",		"\033[H",	"7" },		/* 71 kp 7 */
	{ KP,	"8",		"\033[A",	"8" },		/* 72 kp 8 */
	{ KP,	"9",		"\033[I",	"9" },		/* 73 kp 9 */
	{ KP,	"-",		"-",		"-" },		/* 74 kp - */
	{ KP,	"4",		"\033[D",	"4" },		/* 75 kp 4 */
	{ KP,	"5",		"\033[E",	"5" },		/* 76 kp 5 */
	{ KP,	"6",		"\033[C",	"6" },		/* 77 kp 6 */
	{ KP,	"+",		"+",		"+" },		/* 78 kp + */
	{ KP,	"1",		"\033[F",	"1" },		/* 79 kp 1 */
	{ KP,	"2",		"\033[B",	"2" },		/* 80 kp 2 */
	{ KP,	"3",		"\033[G",	"3" },		/* 81 kp 3 */
	{ KP,	"0",		"\033[L",	"0" },		/* 82 kp 0 */
	{ KP,	".",		"\177",		"." },		/* 83 kp . */
	{ NONE,	"",		"",		"" },		/* 84 0 */
	{ NONE,	"100",		"",		"" },		/* 85 0 */
	{ NONE,	"101",		"",		"" },		/* 86 0 */
	{ FUNC,	"\033[W",	"\033[i",	"\033[u" },	/* 87 f11 */
	{ FUNC,	"\033[X",	"\033[j",	"\033[v" },	/* 88 f12 */
	{ NONE,	"102",		"",		"" },		/* 89 0 */
	{ NONE,	"103",		"",		"" },		/* 90 0 */
	{ NONE,	"",		"",		"" },		/* 91 0 */
	{ NONE,	"",		"",		"" },		/* 92 0 */
	{ NONE,	"",		"",		"" },		/* 93 0 */
	{ NONE,	"",		"",		"" },		/* 94 0 */
	{ NONE,	"",		"",		"" },		/* 95 0 */
	{ NONE,	"",		"",		"" },		/* 96 0 */
	{ NONE,	"",		"",		"" },		/* 97 0 */
	{ NONE,	"",		"",		"" },		/* 98 0 */
	{ NONE,	"",		"",		"" },		/* 99 0 */
	{ NONE,	"",		"",		"" },		/* 100 */
	{ NONE,	"",		"",		"" },		/* 101 */
	{ NONE,	"",		"",		"" },		/* 102 */
	{ NONE,	"",		"",		"" },		/* 103 */
	{ NONE,	"",		"",		"" },		/* 104 */
	{ NONE,	"",		"",		"" },		/* 105 */
	{ NONE,	"",		"",		"" },		/* 106 */
	{ NONE,	"",		"",		"" },		/* 107 */
	{ NONE,	"",		"",		"" },		/* 108 */
	{ NONE,	"",		"",		"" },		/* 109 */
	{ NONE,	"",		"",		"" },		/* 110 */
	{ NONE,	"",		"",		"" },		/* 111 */
	{ NONE,	"",		"",		"" },		/* 112 */
	{ NONE,	"",		"",		"" },		/* 113 */
	{ NONE,	"",		"",		"" },		/* 114 */
	{ NONE,	"",		"",		"" },		/* 115 */
	{ NONE,	"",		"",		"" },		/* 116 */
	{ NONE,	"",		"",		"" },		/* 117 */
	{ NONE,	"",		"",		"" },		/* 118 */
	{ NONE,	"",		"",		"" },		/* 119 */
	{ NONE,	"",		"",		"" },		/* 120 */
	{ NONE,	"",		"",		"" },		/* 121 */
	{ NONE,	"",		"",		"" },		/* 122 */
	{ NONE,	"",		"",		"" },		/* 123 */
	{ NONE,	"",		"",		"" },		/* 124 */
	{ NONE,	"",		"",		"" },		/* 125 */
	{ NONE,	"",		"",		"" },		/* 126 */
	{ NONE,	"",		"",		"" },		/* 127 */
};

#else /* NONUS_KBD */

typedef struct {
	u_short type;
	char unshift[CODE_SIZE];
	char shift[CODE_SIZE];
	char ctl[CODE_SIZE];
	char altgr[CODE_SIZE];
} Scan_def;

#ifdef FRENCH_KBD

static Scan_def	scan_codes[] = {
	{ NONE,		"",	"",	"",	"" },	/* 0 unused */
	{ ASCII,	"\033",	"\033",	"\033", "\033" }, /* 1 ESCape */
	{ ASCII,	"&",	"1",	"&",	"" },	/* 2 1 */
	{ ASCII,	"\351",	"2",	"\211",	"~" },	/* 3 2 */
	{ ASCII,	"\"",	"3",	"\"",	"#" },	/* 4 3 */
	{ ASCII,	"'",	"4",	"'",	"{" },	/* 5 4 */
	{ ASCII,	"(",	"5",	"(",	"[" },	/* 6 5 */
	{ ASCII,	"-",	"6",	"-",	"|" },	/* 7 6 */
	{ ASCII,	"\350",	"7",	"\210", "`" },	/* 8 7 */
	{ ASCII,	"_",	"8",	"\037",	"\\" },	/* 9 8 */
	{ ASCII,	"\347",	"9",	"\207",	"^" },	/* 10 9 */
	{ ASCII,	"\340",	"0",	"\340",	"@" },	/* 11 0 */
	{ ASCII,	")",	"\260",	")",	"]" },	/* 12 - */
	{ ASCII,	"=",	"+",	"+",	"}" },	/* 13 = */
	{ ASCII,	"\177",	"\177",	"\010",	"\177" }, /* 14 backspace */
	{ ASCII,	"\t",	"\177\t", "\t",	"\t" },	/* 15 tab */
	{ ASCII,	"a",	"A",	"\001",	"a" },	/* 16 q */
	{ ASCII,	"z",	"Z",	"\032", "z" },	/* 17 w */
	{ ASCII,	"e",	"E",	"\005", "e" },	/* 18 e */
	{ ASCII,	"r",	"R",	"\022", "r" },	/* 19 r */
	{ ASCII,	"t",	"T",	"\024",	"t" },	/* 20 t */
	{ ASCII,	"y",	"Y",	"\031",	"y" },	/* 21 y */
	{ ASCII,	"u",	"U",	"\025", "u" },	/* 22 u */
	{ ASCII,	"i",	"I",	"\011",	"i" },	/* 23 i */
	{ ASCII,	"o",	"O",	"\017", "o" },	/* 24 o */
	{ ASCII,	"p",	"P",	"\020",	"p" },	/* 25 p */
	{ NONE,		"",	"",	"",	"" },	/* 26 [ */
	{ ASCII,	"$",	"\243",	"$",	"$" },	/* 27 ] */
	{ ASCII,	"\r",	"\r",	"\n",	"\r" },	/* 28 return */
	{ CTL,		"",	"",	"",	"" },	/* 29 control */
	{ ASCII,	"q",	"Q",	"\021",	"q" },	/* 30 a */
	{ ASCII,	"s",	"S",	"\023",	"s" },	/* 31 s */
	{ ASCII,	"d",	"D",	"\004",	"d" },	/* 32 d */
	{ ASCII,	"f",	"F",	"\006",	"f" },	/* 33 f */
	{ ASCII,	"g",	"G",	"\007",	"g" },	/* 34 g */
	{ ASCII,	"h",	"H",	"\010",	"h" },	/* 35 h */
	{ ASCII,	"j",	"J",	"\n",	"j" },	/* 36 j */
	{ ASCII,	"k",	"K",	"\013",	"k" },	/* 37 k */
	{ ASCII,	"l",	"L",	"\014",	"l" },	/* 38 l */
	{ ASCII,	"m",	"M",	"\r",	"m" },	/* 39 ; */
	{ ASCII,	"\371",	"%",	"\231",	"\371" }, /* 40 ' */
	{ ASCII,	"\262",	"",	"\262",	"\262" }, /* 41 ` */
	{ SHIFT,	"",	"",	"",	"" },	/* 42 shift */
	{ ASCII,	"*",	"\265",	"*",	"*" },	/* 43 \ */
	{ ASCII,	"w",	"W",	"\027",	"w" },	/* 44 z */
	{ ASCII,	"x",	"X",	"\030",	"x" },	/* 45 x */
	{ ASCII,	"c",	"C",	"\003",	"c" },	/* 46 c */
	{ ASCII,	"v",	"V",	"\026",	"v" },	/* 47 v */
	{ ASCII,	"b",	"B",	"\002",	"b" },	/* 48 b */
	{ ASCII,	"n",	"N",	"\016",	"n" },	/* 49 n */
	{ ASCII,	",",	"?",	",",	"," },	/* 50 m */
	{ ASCII,	";",	".",	";",	";" },	/* 51 , */
	{ ASCII,	":",	"/",	"\037",	":" },	/* 52 . */
	{ ASCII,	"!",	"\266",	"!",	"!" },	/* 53 / */
	{ SHIFT,	"",	"",	"",	"" },	/* 54 shift */
	{ KP,		"*",	"*",	"*",	"*" },	/* 55 kp * */
	{ ALT,		"",	"",	"",	"" },	/* 56 alt */
	{ ASCII,	" ",	" ",	"\000",	" " },	/* 57 space */
	{ CAPS,	"",	"",	"",	"" },	/* 58 caps */
	{ FUNC,	"\033[M",	"\033[Y",	"\033[k",	"" }, /* 59 f1 */
	{ FUNC,	"\033[N",	"\033[Z",	"\033[l",	"" }, /* 60 f2 */
	{ FUNC,	"\033[O",	"\033[a",	"\033[m",	"" }, /* 61 f3 */
	{ FUNC,	"\033[P",	"\033[b",	"\033[n",	"" }, /* 62 f4 */
	{ FUNC,	"\033[Q",	"\033[c",	"\033[o",	"" }, /* 63 f5 */
	{ FUNC,	"\033[R",	"\033[d",	"\033[p",	"" }, /* 64 f6 */
	{ FUNC,	"\033[S",	"\033[e",	"\033[q",	"" }, /* 65 f7 */
	{ FUNC,	"\033[T",	"\033[f",	"\033[r",	"" }, /* 66 f8 */
	{ FUNC,	"\033[U",	"\033[g",	"\033[s",	"" }, /* 67 f9 */
	{ FUNC,	"\033[V",	"\033[h",	"\033[t",	"" }, /* 68 f10 */
	{ NUM,	"",		"",		"",		"" }, /* 69 num lock */
	{ SCROLL,	"",		"",		"",		"" }, /* 70 scroll lock */
	{ KP,	"7",		"\033[H",	"7",	"" },	/* 71 kp 7 */
	{ KP,	"8",		"\033[A",	"8",	"" },	/* 72 kp 8 */
	{ KP,	"9",		"\033[I",	"9",	"" },	/* 73 kp 9 */
	{ KP,	"-",		"-",		"-",	"" },	/* 74 kp - */
	{ KP,	"4",		"\033[D",	"4",	"" },	/* 75 kp 4 */
	{ KP,	"5",		"\033[E",	"5",	"" },	/* 76 kp 5 */
	{ KP,	"6",		"\033[C",	"6",	"" },	/* 77 kp 6 */
	{ KP,	"+",		"+",		"+",	"" },	/* 78 kp + */
	{ KP,	"1",		"\033[F",	"1",	"" },	/* 79 kp 1 */
	{ KP,	"2",		"\033[B",	"2",	"" },	/* 80 kp 2 */
	{ KP,	"3",		"\033[G",	"3",	"" },	/* 81 kp 3 */
	{ KP,	"0",		"\033[L",	"0",	"" },	/* 82 kp 0 */
	{ KP,	".",		"\177",		".",	"" },	/* 83 kp . */
	{ NONE,	"",		"",		"",	"" },	/* 84 0 */
	{ NONE,	"100",		"",		"",	"" },	/* 85 0 */
	{ ASCII,	"<",		">",		"<",	"<" },	/* 86 < > */
	{ FUNC,	"\033[W",	"\033[i",	"\033[u","" },	/* 87 f11 */
	{ FUNC,	"\033[X",	"\033[j",	"\033[v","" },	/* 88 f12 */
	{ NONE,	"102",		"",		"",	"" },	/* 89 0 */
	{ NONE,	"103",		"",		"",	"" },	/* 90 0 */
	{ NONE,	"",		"",		"",	"" },	/* 91 0 */
	{ NONE,	"",		"",		"",	"" },	/* 92 0 */
	{ NONE,	"",		"",		"",	"" },	/* 93 0 */
	{ NONE,	"",		"",		"",	"" },	/* 94 0 */
	{ NONE,	"",		"",		"",	"" },	/* 95 0 */
	{ NONE,	"",		"",		"",	"" },	/* 96 0 */
	{ NONE,	"",		"",		"",	"" },	/* 97 0 */
	{ NONE,	"",		"",		"",	"" },	/* 98 0 */
	{ NONE,	"",		"",		"",	"" },	/* 99 0 */
	{ NONE,	"",		"",		"",	"" },	/* 100 */
	{ NONE,	"",		"",		"",	"" },	/* 101 */
	{ NONE,	"",		"",		"",	"" },	/* 102 */
	{ NONE,	"",		"",		"",	"" },	/* 103 */
	{ NONE,	"",		"",		"",	"" },	/* 104 */
	{ NONE,	"",		"",		"",	"" },	/* 105 */
	{ NONE,	"",		"",		"",	"" },	/* 106 */
	{ NONE,	"",		"",		"",	"" },	/* 107 */
	{ NONE,	"",		"",		"",	"" },	/* 108 */
	{ NONE,	"",		"",		"",	"" },	/* 109 */
	{ NONE,	"",		"",		"",	"" },	/* 110 */
	{ NONE,	"",		"",		"",	"" },	/* 111 */
	{ NONE,	"",		"",		"",	"" },	/* 112 */
	{ NONE,	"",		"",		"",	"" },	/* 113 */
	{ NONE,	"",		"",		"",	"" },	/* 114 */
	{ NONE,	"",		"",		"",	"" },	/* 115 */
	{ NONE,	"",		"",		"",	"" },	/* 116 */
	{ NONE,	"",		"",		"",	"" },	/* 117 */
	{ NONE,	"",		"",		"",	"" },	/* 118 */
	{ NONE,	"",		"",		"",	"" },	/* 119 */
	{ NONE,	"",		"",		"",	"" },	/* 120 */
	{ NONE,	"",		"",		"",	"" },	/* 121 */
	{ NONE,	"",		"",		"",	"" },	/* 122 */
	{ NONE,	"",		"",		"",	"" },	/* 123 */
	{ NONE,	"",		"",		"",	"" },	/* 124 */
	{ NONE,	"",		"",		"",	"" },	/* 125 */
	{ NONE,	"",		"",		"",	"" },	/* 126 */
	{ NONE,	"",		"",		"",	"" }	/* 127 */
};

#endif /* FRENCH_KBD */

#ifdef GERMAN_KBD

static Scan_def	scan_codes[] = {
	{ NONE,	"",	"",	"",	"" },	/* 0 unused */
	{ ASCII,	"\033",	"\033",	"\033", "\033"}, /* 1 ESCape */
	{ ASCII,	"1",	"!",	"!",	"" },	/* 2 1 */
	{ ASCII,	"2",	"\"",	"\"",	"\xb2" }, /* 3 2 */
	{ ASCII,	"3",	"\xa7",	"\xa7",	"\xb3" }, /* 4 3 */
	{ ASCII,	"4",	"$",	"$",	"" },	/* 5 4 */
	{ ASCII,	"5",	"%",	"%",	"" },	/* 6 5 */
	{ ASCII,	"6",	"&",	"&",	"" },	/* 7 6 */
	{ ASCII,	"7",	"/",	"/",	"{" },	/* 8 7 */
	{ ASCII,	"8",	"(",	"(",	"[" },	/* 9 8 */
	{ ASCII,	"9",	")",	")",	"]" },	/* 10 9 */
	{ ASCII,	"0",	"=",	"=",	"}" },	/* 11 0 */
	{ ASCII,	"\xdf","?",	"?",	"\\" },	/* 12 - */
	{ ASCII,	"'",	"`",	"`",	"" },	/* 13 = */
	{ ASCII,	"\177",	"\177",	"\010",	"\177" }, /* 14 backspace */
	{ ASCII,	"\t",	"\177\t", "\t",	"\t" },	/* 15 tab */
	{ ASCII,	"q",	"Q",	"\021",	"@" },	/* 16 q */
	{ ASCII,	"w",	"W",	"\027", "w" },	/* 17 w */
	{ ASCII,	"e",	"E",	"\005", "e" },	/* 18 e */
	{ ASCII,	"r",	"R",	"\022", "r" },	/* 19 r */
	{ ASCII,	"t",	"T",	"\024",	"t" },	/* 20 t */
	{ ASCII,	"z",	"Z",	"\032",	"z" },	/* 21 y */
	{ ASCII,	"u",	"U",	"\025", "u" },	/* 22 u */
	{ ASCII,	"i",	"I",	"\011",	"i" },	/* 23 i */
	{ ASCII,	"o",	"O",	"\017", "o" },	/* 24 o */
	{ ASCII,	"p",	"P",	"\020",	"p" },	/* 25 p */
	{ ASCII,	"\xfc",	"\xdc",	"\xfc",	"\xdc" }, /* 26 [ */
	{ ASCII,	"+",	"*",	"+",	"~" },	/* 27 ] */
	{ ASCII,	"\r",	"\r",	"\n",	"\r" },	/* 28 return */
	{ CTL,	"",	"",	"",	"" },	/* 29 control */
	{ ASCII,	"a",	"A",	"\001",	"a" },	/* 30 a */
	{ ASCII,	"s",	"S",	"\023",	"s" },	/* 31 s */
	{ ASCII,	"d",	"D",	"\004",	"d" },	/* 32 d */
	{ ASCII,	"f",	"F",	"\006",	"f" },	/* 33 f */
	{ ASCII,	"g",	"G",	"\007",	"g" },	/* 34 g */
	{ ASCII,	"h",	"H",	"\010",	"h" },	/* 35 h */
	{ ASCII,	"j",	"J",	"\n",	"j" },	/* 36 j */
	{ ASCII,	"k",	"K",	"\013",	"k" },	/* 37 k */
	{ ASCII,	"l",	"L",	"\014",	"l" },	/* 38 l */
	{ ASCII,	"\xf6",	"\xd6",	"\xf6",	"\xd6" }, /* 39 ; */
	{ ASCII,	"\xe4",	"\xc4",	"\xe4",	"\xc4" }, /* 40 ' */
	{ ASCII,	"\136",	"\370",	"\136",	"\370" }, /* 41 ` */
	{ SHIFT,	"",	"",	"",	"" },	/* 42 shift */
	{ ASCII,	"#",	"'",	"#",	"'" },	/* 43 \ */
	{ ASCII,	"y",	"Y",	"\x19",	"y" },	/* 44 z */
	{ ASCII,	"x",	"X",	"\030",	"x" },	/* 45 x */
	{ ASCII,	"c",	"C",	"\003",	"c" },	/* 46 c */
	{ ASCII,	"v",	"V",	"\026",	"v" },	/* 47 v */
	{ ASCII,	"b",	"B",	"\002",	"b" },	/* 48 b */
	{ ASCII,	"n",	"N",	"\016",	"n" },	/* 49 n */
	{ ASCII,	"m",	"M",	"\r",	"m" },	/* 50 m */
	{ ASCII,	",",	";",	",",	";" },	/* 51 , */
	{ ASCII,	".",	":",	".",	":" },	/* 52 . */
	{ ASCII,	"-",	"_",	"-",	"_" },	/* 53 / */
	{ SHIFT,	"",	"",	"",	"" },	/* 54 shift */
	{ KP,	"*",	"*",	"*",	"*" },	/* 55 kp * */
	{ ALT,	"",	"",	"",	"" },	/* 56 alt */
	{ ASCII,	" ",	" ",	"\000",	" " },	/* 57 space */
	{ CAPS,	"",	"",	"",	"" },	/* 58 caps */
	{ FUNC,	"\033[M",	"\033[Y",	"\033[k",	"" }, /* 59 f1 */
	{ FUNC,	"\033[N",	"\033[Z",	"\033[l",	"" }, /* 60 f2 */
	{ FUNC,	"\033[O",	"\033[a",	"\033[m",	"" }, /* 61 f3 */
	{ FUNC,	"\033[P",	"\033[b",	"\033[n",	"" }, /* 62 f4 */
	{ FUNC,	"\033[Q",	"\033[c",	"\033[o",	"" }, /* 63 f5 */
	{ FUNC,	"\033[R",	"\033[d",	"\033[p",	"" }, /* 64 f6 */
	{ FUNC,	"\033[S",	"\033[e",	"\033[q",	"" }, /* 65 f7 */
	{ FUNC,	"\033[T",	"\033[f",	"\033[r",	"" }, /* 66 f8 */
	{ FUNC,	"\033[U",	"\033[g",	"\033[s",	"" }, /* 67 f9 */
	{ FUNC,	"\033[V",	"\033[h",	"\033[t",	"" }, /* 68 f10 */
	{ NUM,	"",		"",		"",		"" }, /* 69 num lock */
	{ SCROLL,	"",		"",		"",		"" }, /* 70 scroll lock */
	{ KP,	"7",		"\033[H",	"7",	"" },	/* 71 kp 7 */
	{ KP,	"8",		"\033[A",	"8",	"" },	/* 72 kp 8 */
	{ KP,	"9",		"\033[I",	"9",	"" },	/* 73 kp 9 */
	{ KP,	"-",		"-",		"-",	"" },	/* 74 kp - */
	{ KP,	"4",		"\033[D",	"4",	"" },	/* 75 kp 4 */
	{ KP,	"5",		"\033[E",	"5",	"" },	/* 76 kp 5 */
	{ KP,	"6",		"\033[C",	"6",	"" },	/* 77 kp 6 */
	{ KP,	"+",		"+",		"+",	"" },	/* 78 kp + */
	{ KP,	"1",		"\033[F",	"1",	"" },	/* 79 kp 1 */
	{ KP,	"2",		"\033[B",	"2",	"" },	/* 80 kp 2 */
	{ KP,	"3",		"\033[G",	"3",	"" },	/* 81 kp 3 */
	{ KP,	"0",		"\033[L",	"0",	"" },	/* 82 kp 0 */
	{ KP,	",",		"\177",		",",	"" },	/* 83 kp . */
	{ NONE,	"",		"",		"",	"" },	/* 84 0 */
	{ NONE,	"100",		"",		"",	"" },	/* 85 0 */
	{ ASCII,	"<",		">",		"<",	"|" },	/* 86 < > */
	{ FUNC,	"\033[W",	"\033[i",	"\033[u","" },	/* 87 f11 */
	{ FUNC,	"\033[X",	"\033[j",	"\033[v","" },	/* 88 f12 */
	{ NONE,	"102",		"",		"",	"" },	/* 89 0 */
	{ NONE,	"103",		"",		"",	"" },	/* 90 0 */
	{ NONE,	"",		"",		"",	"" },	/* 91 0 */
	{ NONE,	"",		"",		"",	"" },	/* 92 0 */
	{ NONE,	"",		"",		"",	"" },	/* 93 0 */
	{ NONE,	"",		"",		"",	"" },	/* 94 0 */
	{ NONE,	"",		"",		"",	"" },	/* 95 0 */
	{ NONE,	"",		"",		"",	"" },	/* 96 0 */
	{ NONE,	"",		"",		"",	"" },	/* 97 0 */
	{ NONE,	"",		"",		"",	"" },	/* 98 0 */
	{ NONE,	"",		"",		"",	"" },	/* 99 0 */
	{ NONE,	"",		"",		"",	"" },	/* 100 */
	{ NONE,	"",		"",		"",	"" },	/* 101 */
	{ NONE,	"",		"",		"",	"" },	/* 102 */
	{ NONE,	"",		"",		"",	"" },	/* 103 */
	{ NONE,	"",		"",		"",	"" },	/* 104 */
	{ NONE,	"",		"",		"",	"" },	/* 105 */
	{ NONE,	"",		"",		"",	"" },	/* 106 */
	{ NONE,	"",		"",		"",	"" },	/* 107 */
	{ NONE,	"",		"",		"",	"" },	/* 108 */
	{ NONE,	"",		"",		"",	"" },	/* 109 */
	{ NONE,	"",		"",		"",	"" },	/* 110 */
	{ NONE,	"",		"",		"",	"" },	/* 111 */
	{ NONE,	"",		"",		"",	"" },	/* 112 */
	{ NONE,	"",		"",		"",	"" },	/* 113 */
	{ NONE,	"",		"",		"",	"" },	/* 114 */
	{ NONE,	"",		"",		"",	"" },	/* 115 */
	{ NONE,	"",		"",		"",	"" },	/* 116 */
	{ NONE,	"",		"",		"",	"" },	/* 117 */
	{ NONE,	"",		"",		"",	"" },	/* 118 */
	{ NONE,	"",		"",		"",	"" },	/* 119 */
	{ NONE,	"",		"",		"",	"" },	/* 120 */
	{ NONE,	"",		"",		"",	"" },	/* 121 */
	{ NONE,	"",		"",		"",	"" },	/* 122 */
	{ NONE,	"",		"",		"",	"" },	/* 123 */
	{ NONE,	"",		"",		"",	"" },	/* 124 */
	{ NONE,	"",		"",		"",	"" },	/* 125 */
	{ NONE,	"",		"",		"",	"" },	/* 126 */
	{ NONE,	"",		"",		"",	"" }	/* 127 */
};

#endif /* GERMAN_KBD */

#ifdef NORWEGIAN_KBD
static Scan_def	scan_codes[] = {
	{ NONE,	"",	"",	"",	"" },	/* 0 unused */
	{ ASCII,	"\033",	"\033",	"\033", "\033" }, /* 1 ESCape */
	{ ASCII,	"1",	"!",	"",	"\241" },	/* 2 1 */
	{ ASCII,	"2",	"\"",	"\000",	"@" },	/* 3 2 */
	{ ASCII,	"3",	"#",	"",	"\243" },	/* 4 3 */
	{ ASCII,	"4",	"$",	"",	"$" },	/* 5 4 */
	{ ASCII,	"5",	"%",	"\034",	"\\" },	/* 6 5 */
	{ ASCII,	"6",	"&",	"\034",	"|" },	/* 7 6 */
	{ ASCII,	"7",	"/",	"\033", "{" },	/* 8 7 */
	{ ASCII,	"8",	"(",	"\033",	"[" },	/* 9 8 */
	{ ASCII,	"9",	")",	"\035",	"]" },	/* 10 9 */
	{ ASCII,	"0",	"=",	"\035",	"}" },	/* 11 0 */
	{ ASCII,	"+",	"?",	"\037",	"\277" },	/* 12 - */
	{ ASCII,	"\\",	"`",	"\034",	"'" },	/* 13 = */
	{ ASCII,	"\177",	"\177",	"\010",	"\177" },	/* 14 backspace */
	{ ASCII,	"\t",	"\177\t", "\t", "\t" },	/* 15 tab */
	{ ASCII,	"q",	"Q",	"\021",	"q" },	/* 16 q */
	{ ASCII,	"w",	"W",	"\027", "w" },	/* 17 w */
	{ ASCII,	"e",	"E",	"\005", "\353" },	/* 18 e */
	{ ASCII,	"r",	"R",	"\022", "r" },	/* 19 r */
	{ ASCII,	"t",	"T",	"\024",	"t" },	/* 20 t */
	{ ASCII,	"y",	"Y",	"\031",	"y" },	/* 21 y */
	{ ASCII,	"u",	"U",	"\025", "\374" },	/* 22 u */
	{ ASCII,	"i",	"I",	"\011",	"i" },	/* 23 i */
	{ ASCII,	"o",	"O",	"\017", "\366" },	/* 24 o */
	{ ASCII,	"p",	"P",	"\020",	"p" },	/* 25 p */
	{ ASCII,	"\345",	"\305",	"\334",	"\374" },	/* 26 [ */
	{ ASCII,	"~",	"^",	"\036",	"" },	/* 27 ] */
	{ ASCII,	"\r",	"\r",	"\n",	"\r" },	/* 28 return */
	{ CTL,	"",	"",	"",	"" },	/* 29 control */
	{ ASCII,	"a",	"A",	"\001",	"\344" },	/* 30 a */
	{ ASCII,	"s",	"S",	"\023",	"\337" },	/* 31 s */
	{ ASCII,	"d",	"D",	"\004",	"d" },	/* 32 d */
	{ ASCII,	"f",	"F",	"\006",	"f" },	/* 33 f */
	{ ASCII,	"g",	"G",	"\007",	"g" },	/* 34 g */
	{ ASCII,	"h",	"H",	"\010",	"h" },	/* 35 h */
	{ ASCII,	"j",	"J",	"\n",	"j" },	/* 36 j */
	{ ASCII,	"k",	"K",	"\013",	"k" },	/* 37 k */
	{ ASCII,	"l",	"L",	"\014",	"l" },	/* 38 l */
	{ ASCII,	"\370",	"\330",	"\326",	"\366" },	/* 39 ; */
	{ ASCII,	"\346",	"\306",	"\304",	"\344" },	/* 40 ' */
	{ ASCII,	"|",	"@",	"\034",	"\247" },	/* 41 ` */
	{ SHIFT,	"",	"",	"",	"" },	/* 42 shift */
	{ ASCII,	"'",	"*",	"'",	"'" },	/* 43 \ */
	{ ASCII,	"z",	"Z",	"\032",	"z" },	/* 44 z */
	{ ASCII,	"x",	"X",	"\030",	"x" },	/* 45 x */
	{ ASCII,	"c",	"C",	"\003",	"c" },	/* 46 c */
	{ ASCII,	"v",	"V",	"\026",	"v" },	/* 47 v */
	{ ASCII,	"b",	"B",	"\002",	"b" },	/* 48 b */
	{ ASCII,	"n",	"N",	"\016",	"n" },	/* 49 n */
	{ ASCII,	"m",	"M",	"\015",	"m" },	/* 50 m */
	{ ASCII,	",",	";",	",",	"," },	/* 51 , */
	{ ASCII,	".",	":",	".",	"." },	/* 52 . */
	{ ASCII,	"-",	"_",	"\037",	"-" },	/* 53 / */
	{ SHIFT,	"",	"",	"",	"" },	/* 54 shift */
	{ KP,	"*",	"*",	"*",	"*" },	/* 55 kp * */
	{ ALT,	"",	"",	"",	"" },	/* 56 alt */
	{ ASCII,	" ",	" ",	"\000",	" " },	/* 57 space */
	{ CAPS,	"",	"",	"",	"" },	/* 58 caps */
	{ FUNC,	"\033[M",	"\033[Y",	"\033[k",	"" }, /* 59 f1 */
	{ FUNC,	"\033[N",	"\033[Z",	"\033[l",	"" }, /* 60 f2 */
	{ FUNC,	"\033[O",	"\033[a",	"\033[m",	"" }, /* 61 f3 */
	{ FUNC,	"\033[P",	"\033[b",	"\033[n",	"" }, /* 62 f4 */
	{ FUNC,	"\033[Q",	"\033[c",	"\033[o",	"" }, /* 63 f5 */
	{ FUNC,	"\033[R",	"\033[d",	"\033[p",	"" }, /* 64 f6 */
	{ FUNC,	"\033[S",	"\033[e",	"\033[q",	"" }, /* 65 f7 */
	{ FUNC,	"\033[T",	"\033[f",	"\033[r",	"" }, /* 66 f8 */
	{ FUNC,	"\033[U",	"\033[g",	"\033[s",	"" }, /* 67 f9 */
	{ FUNC,	"\033[V",	"\033[h",	"\033[t",	"" }, /* 68 f10 */
	{ NUM,	"",		"",		"",		"" }, /* 69 num lock */
	{ SCROLL,	"",		"",		"",		"" }, /* 70 scroll lock */
	{ KP,	"7",		"\033[H",	"7",	"" },	/* 71 kp 7 */
	{ KP,	"8",		"\033[A",	"8",	"" },	/* 72 kp 8 */
	{ KP,	"9",		"\033[I",	"9",	"" },	/* 73 kp 9 */
	{ KP,	"-",		"-",		"-",	"" },	/* 74 kp - */
	{ KP,	"4",		"\033[D",	"4",	"" },	/* 75 kp 4 */
	{ KP,	"5",		"\033[E",	"5",	"" },	/* 76 kp 5 */
	{ KP,	"6",		"\033[C",	"6",	"" },	/* 77 kp 6 */
	{ KP,	"+",		"+",		"+",	"" },	/* 78 kp + */
	{ KP,	"1",		"\033[F",	"1",	"" },	/* 79 kp 1 */
	{ KP,	"2",		"\033[B",	"2",	"" },	/* 80 kp 2 */
	{ KP,	"3",		"\033[G",	"3",	"" },	/* 81 kp 3 */
	{ KP,	"0",		"\033[L",	"0",	"" },	/* 82 kp 0 */
	{ KP,	".",		"\177",		".",	"" },	/* 83 kp . */
	{ NONE,	"",		"",		"",	"" },	/* 84 0 */
	{ NONE,	"100",		"",		"",	"" },	/* 85 0 */
	{ ASCII,	"<",		">",		"\273",	"\253" },	/* 86 < > */
	{ FUNC,	"\033[W",	"\033[i",	"\033[u","" },	/* 87 f11 */
	{ FUNC,	"\033[X",	"\033[j",	"\033[v","" },	/* 88 f12 */
	{ NONE,	"102",		"",		"",	"" },	/* 89 0 */
	{ NONE,	"103",		"",		"",	"" },	/* 90 0 */
	{ NONE,	"",		"",		"",	"" },	/* 91 0 */
	{ NONE,	"",		"",		"",	"" },	/* 92 0 */
	{ NONE,	"",		"",		"",	"" },	/* 93 0 */
	{ NONE,	"",		"",		"",	"" },	/* 94 0 */
	{ NONE,	"",		"",		"",	"" },	/* 95 0 */
	{ NONE,	"",		"",		"",	"" },	/* 96 0 */
	{ NONE,	"",		"",		"",	"" },	/* 97 0 */
	{ NONE,	"",		"",		"",	"" },	/* 98 0 */
	{ NONE,	"",		"",		"",	"" },	/* 99 0 */
	{ NONE,	"",		"",		"",	"" },	/* 100 */
	{ NONE,	"",		"",		"",	"" },	/* 101 */
	{ NONE,	"",		"",		"",	"" },	/* 102 */
	{ NONE,	"",		"",		"",	"" },	/* 103 */
	{ NONE,	"",		"",		"",	"" },	/* 104 */
	{ NONE,	"",		"",		"",	"" },	/* 105 */
	{ NONE,	"",		"",		"",	"" },	/* 106 */
	{ NONE,	"",		"",		"",	"" },	/* 107 */
	{ NONE,	"",		"",		"",	"" },	/* 108 */
	{ NONE,	"",		"",		"",	"" },	/* 109 */
	{ NONE,	"",		"",		"",	"" },	/* 110 */
	{ NONE,	"",		"",		"",	"" },	/* 111 */
	{ NONE,	"",		"",		"",	"" },	/* 112 */
	{ NONE,	"",		"",		"",	"" },	/* 113 */
	{ NONE,	"",		"",		"",	"" },	/* 114 */
	{ NONE,	"",		"",		"",	"" },	/* 115 */
	{ NONE,	"",		"",		"",	"" },	/* 116 */
	{ NONE,	"",		"",		"",	"" },	/* 117 */
	{ NONE,	"",		"",		"",	"" },	/* 118 */
	{ NONE,	"",		"",		"",	"" },	/* 119 */
	{ NONE,	"",		"",		"",	"" },	/* 120 */
	{ NONE,	"",		"",		"",	"" },	/* 121 */
	{ NONE,	"",		"",		"",	"" },	/* 122 */
	{ NONE,	"",		"",		"",	"" },	/* 123 */
	{ NONE,	"",		"",		"",	"" },	/* 124 */
	{ NONE,	"",		"",		"",	"" },	/* 125 */
	{ NONE,	"",		"",		"",	"" },	/* 126 */
	{ NONE,	"",		"",		"",	"" }	/* 127 */
};
#endif

#ifdef FINNISH_KBD
static Scan_def	scan_codes[] = {
	{ NONE,	"",	"",	"",	"" },	/* 0 unused */
	{ ASCII,	"\033",	"\033",	"\033", "\033" },	/* 1 ESCape */
	{ ASCII,	"1",	"!",	"",	"\241" },	/* 2 1 */
	{ ASCII,	"2",	"\"",	"\000",	"@" },	/* 3 2 */
	{ ASCII,	"3",	"#",	"",	"\243" },	/* 4 3 */
	{ ASCII,	"4",	"$",	"",	"$" },	/* 5 4 */
	{ ASCII,	"5",	"%",	"\034",	"%" },	/* 6 5 */
	{ ASCII,	"6",	"&",	"\034",	"&" },	/* 7 6 */
	{ ASCII,	"7",	"/",	"\033", "{" },	/* 8 7 */
	{ ASCII,	"8",	"(",	"\033",	"[" },	/* 9 8 */
	{ ASCII,	"9",	")",	"\035",	"]" },	/* 10 9 */
	{ ASCII,	"0",	"=",	"\035",	"}" },	/* 11 0 */
	{ ASCII,	"+",	"?",	"\037",	"\\" },	/* 12 - */
	{ ASCII,	"'",	"`",	"\034",	"'" },	/* 13 = */
	{ ASCII,	"\177",	"\177",	"\010",	"\177" },	/* 14 backspace */
	{ ASCII,	"\t",	"\177\t", "\t", "\t" },	/* 15 tab */
	{ ASCII,	"q",	"Q",	"\021",	"q" },	/* 16 q */
	{ ASCII,	"w",	"W",	"\027", "w" },	/* 17 w */
	{ ASCII,	"e",	"E",	"\005", "\353" },	/* 18 e */
	{ ASCII,	"r",	"R",	"\022", "r" },	/* 19 r */
	{ ASCII,	"t",	"T",	"\024",	"t" },	/* 20 t */
	{ ASCII,	"y",	"Y",	"\031",	"y" },	/* 21 y */
	{ ASCII,	"u",	"U",	"\025", "\374" },	/* 22 u */
	{ ASCII,	"i",	"I",	"\011",	"i" },	/* 23 i */
	{ ASCII,	"o",	"O",	"\017", "\366" },	/* 24 o */
	{ ASCII,	"p",	"P",	"\020",	"p" },	/* 25 p */
	{ ASCII,	"\345",	"\305",	"\035",	"}" },	/* 26 [ */
	{ ASCII,	"~",	"^",	"\036",	"~" },	/* 27 ] */
	{ ASCII,	"\r",	"\r",	"\n",	"\r" },	/* 28 return */
	{ CTL,	"",	"",	"",	"" },	/* 29 control */
	{ ASCII,	"a",	"A",	"\001",	"\344" },	/* 30 a */
	{ ASCII,	"s",	"S",	"\023",	"\337" },	/* 31 s */
	{ ASCII,	"d",	"D",	"\004",	"d" },	/* 32 d */
	{ ASCII,	"f",	"F",	"\006",	"f" },	/* 33 f */
	{ ASCII,	"g",	"G",	"\007",	"g" },	/* 34 g */
	{ ASCII,	"h",	"H",	"\010",	"h" },	/* 35 h */
	{ ASCII,	"j",	"J",	"\n",	"j" },	/* 36 j */
	{ ASCII,	"k",	"K",	"\013",	"k" },	/* 37 k */
	{ ASCII,	"l",	"L",	"\014",	"l" },	/* 38 l */
	{ ASCII,	"\366",	"\326",	"\034",	"|" },	/* 39 ; */
	{ ASCII,	"\344",	"\304",	"\033",	"{" },	/* 40 ' */
	{ ASCII,	"\247",	"\275",	"\000",	"@" },	/* 41 ` */
	{ SHIFT,	"",	"",	"",	"" },	/* 42 shift */
	{ ASCII,	"'",	"*",	"'",	"'" },	/* 43 \ */
	{ ASCII,	"z",	"Z",	"\032",	"z" },	/* 44 z */
	{ ASCII,	"x",	"X",	"\030",	"x" },	/* 45 x */
	{ ASCII,	"c",	"C",	"\003",	"c" },	/* 46 c */
	{ ASCII,	"v",	"V",	"\026",	"v" },	/* 47 v */
	{ ASCII,	"b",	"B",	"\002",	"b" },	/* 48 b */
	{ ASCII,	"n",	"N",	"\016",	"n" },	/* 49 n */
	{ ASCII,	"m",	"M",	"\015",	"m" },	/* 50 m */
	{ ASCII,	",",	";",	",",	"," },	/* 51 , */
	{ ASCII,	".",	":",	".",	"." },	/* 52 . */
	{ ASCII,	"-",	"_",	"\037",	"-" },	/* 53 / */
	{ SHIFT,	"",	"",	"",	"" },	/* 54 shift */
	{ KP,	"*",	"*",	"*",	"*" },	/* 55 kp * */
	{ ALT,	"",	"",	"",	"" },	/* 56 alt */
	{ ASCII,	" ",	" ",	"\000",	" " },	/* 57 space */
	{ CAPS,	"",	"",	"",	"" },	/* 58 caps */
	{ FUNC,	"\033[M",	"\033[Y",	"\033[k",	"" },	/* 59 f1 */
	{ FUNC,	"\033[N",	"\033[Z",	"\033[l",	"" },	/* 60 f2 */
	{ FUNC,	"\033[O",	"\033[a",	"\033[m",	"" },	/* 61 f3 */
	{ FUNC,	"\033[P",	"\033[b",	"\033[n",	"" },	/* 62 f4 */
	{ FUNC,	"\033[Q",	"\033[c",	"\033[o",	"" },	/* 63 f5 */
	{ FUNC,	"\033[R",	"\033[d",	"\033[p",	"" },	/* 64 f6 */
	{ FUNC,	"\033[S",	"\033[e",	"\033[q",	"" },	/* 65 f7 */
	{ FUNC,	"\033[T",	"\033[f",	"\033[r",	"" },	/* 66 f8 */
	{ FUNC,	"\033[U",	"\033[g",	"\033[s",	"" },	/* 67 f9 */
	{ FUNC,	"\033[V",	"\033[h",	"\033[t",	"" },	/* 68 f10 */
	{ NUM,	"",		"",		"",		"" },	/* 69 num lock */
	{ SCROLL,	"",		"",		"",		"" },  	/* 70 scroll lock */
	{ KP,	"7",		"\033[H",	"7",	"" },	/* 71 kp 7 */
	{ KP,	"8",		"\033[A",	"8",	"" },	/* 72 kp 8 */
	{ KP,	"9",		"\033[I",	"9",	"" },	/* 73 kp 9 */
	{ KP,	"-",		"-",		"-",	"" },	/* 74 kp - */
	{ KP,	"4",		"\033[D",	"4",	"" },	/* 75 kp 4 */
	{ KP,	"5",		"\033[E",	"5",	"" },	/* 76 kp 5 */
	{ KP,	"6",		"\033[C",	"6",	"" },	/* 77 kp 6 */
	{ KP,	"+",		"+",		"+",	"" },	/* 78 kp + */
	{ KP,	"1",		"\033[F",	"1",	"" },	/* 79 kp 1 */
	{ KP,	"2",		"\033[B",	"2",	"" },	/* 80 kp 2 */
	{ KP,	"3",		"\033[G",	"3",	"" },	/* 81 kp 3 */
	{ KP,	"0",		"\033[L",	"0",	"" },	/* 82 kp 0 */
	{ KP,	".",		"\177",		".",	"" },	/* 83 kp . */
	{ NONE,	"",		"",		"",	"" },	/* 84 0 */
	{ NONE,	"100",		"",		"",	"" },	/* 85 0 */
	{ ASCII,	"<",		">",		"<",	"|" },	/* 86 < > */
	{ FUNC,	"\033[W",	"\033[i",	"\033[u","" },	/* 87 f11 */
	{ FUNC,	"\033[X",	"\033[j",	"\033[v","" },	/* 88 f12 */
	{ NONE,	"102",		"",		"",	"" },	/* 89 0 */
	{ NONE,	"103",		"",		"",	"" },	/* 90 0 */
	{ NONE,	"",		"",		"",	"" },	/* 91 0 */
	{ NONE,	"",		"",		"",	"" },	/* 92 0 */
	{ NONE,	"",		"",		"",	"" },	/* 93 0 */
	{ NONE,	"",		"",		"",	"" },	/* 94 0 */
	{ NONE,	"",		"",		"",	"" },	/* 95 0 */
	{ NONE,	"",		"",		"",	"" },	/* 96 0 */
	{ NONE,	"",		"",		"",	"" },	/* 97 0 */
	{ NONE,	"",		"",		"",	"" },	/* 98 0 */
	{ NONE,	"",		"",		"",	"" },	/* 99 0 */
	{ NONE,	"",		"",		"",	"" },	/* 100 */
	{ NONE,	"",		"",		"",	"" },	/* 101 */
	{ NONE,	"",		"",		"",	"" },	/* 102 */
	{ NONE,	"",		"",		"",	"" },	/* 103 */
	{ NONE,	"",		"",		"",	"" },	/* 104 */
	{ NONE,	"",		"",		"",	"" },	/* 105 */
	{ NONE,	"",		"",		"",	"" },	/* 106 */
	{ NONE,	"",		"",		"",	"" },	/* 107 */
	{ NONE,	"",		"",		"",	"" },	/* 108 */
	{ NONE,	"",		"",		"",	"" },	/* 109 */
	{ NONE,	"",		"",		"",	"" },	/* 110 */
	{ NONE,	"",		"",		"",	"" },	/* 111 */
	{ NONE,	"",		"",		"",	"" },	/* 112 */
	{ NONE,	"",		"",		"",	"" },	/* 113 */
	{ NONE,	"",		"",		"",	"" },	/* 114 */
	{ NONE,	"",		"",		"",	"" },	/* 115 */
	{ NONE,	"",		"",		"",	"" },	/* 116 */
	{ NONE,	"",		"",		"",	"" },	/* 117 */
	{ NONE,	"",		"",		"",	"" },	/* 118 */
	{ NONE,	"",		"",		"",	"" },	/* 119 */
	{ NONE,	"",		"",		"",	"" },	/* 120 */
	{ NONE,	"",		"",		"",	"" },	/* 121 */
	{ NONE,	"",		"",		"",	"" },	/* 122 */
	{ NONE,	"",		"",		"",	"" },	/* 123 */
	{ NONE,	"",		"",		"",	"" },	/* 124 */
	{ NONE,	"",		"",		"",	"" },	/* 125 */
	{ NONE,	"",		"",		"",	"" },	/* 126 */
	{ NONE,	"",		"",		"",	"" },	/* 127 */
};
#endif 

/*
 * XXXX Add tables for other keyboards here
 */

#endif

#if (NPCCONSKBD == 0)
char *
sget(void)
{
	u_char dt, *capchar;

top:
	KBD_DELAY;
	dt = inb(IO_KBD + KBDATAP);

	switch (dt) {
	case KBR_ACK:
		ack = 1;
		goto loop;
	case KBR_RESEND:
		nak = 1;
		goto loop;
	}

	capchar = strans(dt);
	if (capchar)
		return (capchar);

loop:
	if ((inb(IO_KBD + KBSTATP) & KBS_DIB) == 0)
		return (0);
	goto top;
}
#endif

/*
 * Get characters from the keyboard.  If none are present, return NULL.
 */
char *
strans(u_char dt)
{
	static u_char extended = 0, shift_state = 0;
	static u_char capchar[2];

#ifdef XSERVER
	if (pc_xmode > 0) {
#if defined(DDB) && defined(XSERVER_DDB)
		/* F12 enters the debugger while in X mode */
		if ((dt == 88) && !pccons_is_console)
			Debugger();
#endif
		capchar[0] = dt;
		capchar[1] = 0;
		/*
		 * Check for locking keys.
		 *
		 * XXX Setting the LEDs this way is a bit bogus.  What if the
		 * keyboard has been remapped in X?
		 */
		switch (scan_codes[dt & 0x7f].type) {
		case NUM:
			if (dt & 0x80) {
				shift_state &= ~NUM;
				break;
			}
			if (shift_state & NUM)
				break;
			shift_state |= NUM;
			lock_state ^= NUM;
			update_leds();
			break;
		case CAPS:
			if (dt & 0x80) {
				shift_state &= ~CAPS;
				break;
			}
			if (shift_state & CAPS)
				break;
			shift_state |= CAPS;
			lock_state ^= CAPS;
			update_leds();
			break;
		case SCROLL:
			if (dt & 0x80) {
				shift_state &= ~SCROLL;
				break;
			}
			if (shift_state & SCROLL)
				break;
			shift_state |= SCROLL;
			lock_state ^= SCROLL;
			update_leds();
			break;
		}
		return (capchar);
	}
#endif /* XSERVER */

	switch (dt) {
	case KBR_EXTENDED0:
		extended = 1;
		return (0);
	}

#ifdef DDB
	/*
	 * Check for cntl-alt-esc.
	 */
	if ((dt == 1) && ((shift_state & (CTL | ALT)) == (CTL | ALT))
	    && pccons_is_console) {
		Debugger();
		dt |= 0x80;	/* discard esc (ddb discarded ctl-alt) */
	}
#endif

	/*
	 * Check for make/break.
	 */
	if (dt & 0x80) {
		/*
		 * break
		 */
		dt &= 0x7f;
		switch (scan_codes[dt].type) {
		case NUM:
			shift_state &= ~NUM;
			break;
		case CAPS:
			shift_state &= ~CAPS;
			break;
		case SCROLL:
			shift_state &= ~SCROLL;
			break;
		case SHIFT:
			shift_state &= ~SHIFT;
			break;
		case ALT:
#ifdef NONUS_KBD
			if (extended) 
			        shift_state &= ~ALTGR;
			else
#endif		     
			shift_state &= ~ALT;
			break;
		case CTL:
			shift_state &= ~CTL;
			break;
		}
	} else {
		/*
		 * make
		 */
#ifdef NUMERIC_SLASH_FIX
		/* fix numeric / on non US keyboard */
		if (extended && dt == 53) {
			capchar[0] = '/';
			extended = 0;
			return (capchar);
		}
#endif
		switch (scan_codes[dt].type) {
		/*
		 * locking keys
		 */
		case NUM:
			if (shift_state & NUM)
				break;
			shift_state |= NUM;
			lock_state ^= NUM;
			update_leds();
			break;
		case CAPS:
			if (shift_state & CAPS)
				break;
			shift_state |= CAPS;
			lock_state ^= CAPS;
			update_leds();
			break;
		case SCROLL:
			if (shift_state & SCROLL)
				break;
			shift_state |= SCROLL;
			if ((lock_state & SCROLL) == 0)
				capchar[0] = 'S' - '@';
			else
				capchar[0] = 'Q' - '@';
			extended = 0;
			return (capchar);
		/*
		 * non-locking keys
		 */
		case SHIFT:
			shift_state |= SHIFT;
			break;
		case ALT:
#ifdef NONUS_KBD
			if (extended)  
			        shift_state |= ALTGR;
			else
#endif
			shift_state |= ALT;
			break;
		case CTL:
			shift_state |= CTL;
			break;
		case ASCII:
#ifdef NONUS_KBD
			if (shift_state & ALTGR) {
			        capchar[0] = scan_codes[dt].altgr[0];
				if (shift_state & CTL) 
				        capchar[0] &= 0x1f;
			} else
#endif
			/* control has highest priority */
			if (shift_state & CTL)
				capchar[0] = scan_codes[dt].ctl[0];
			else if (shift_state & SHIFT)
				capchar[0] = scan_codes[dt].shift[0];
			else
				capchar[0] = scan_codes[dt].unshift[0];
			if ((lock_state & CAPS) && capchar[0] >= 'a' &&
			    capchar[0] <= 'z') {
				capchar[0] -= ('a' - 'A');
			}
			capchar[0] |= (shift_state & ALT);
			extended = 0;
			return (capchar);
		case NONE:
			break;
		case FUNC: {
			char *more_chars;
			if (shift_state & SHIFT)
				more_chars = scan_codes[dt].shift;
			else if (shift_state & CTL)
				more_chars = scan_codes[dt].ctl;
			else
				more_chars = scan_codes[dt].unshift;
			extended = 0;
			return (more_chars);
		}
		case KP: {
			char *more_chars;
			if (shift_state & (SHIFT | CTL) ||
			    (lock_state & NUM) == 0 || extended)
				more_chars = scan_codes[dt].shift;
			else
				more_chars = scan_codes[dt].unshift;
			extended = 0;
			return (more_chars);
		}
		}
	}

	extended = 0;
	return (0);
}

paddr_t
pcmmap(dev_t dev, off_t offset, int nprot)
{

	if (offset > 0x20000)
		return (-1);
	return (x86_btop(0xa0000 + offset));
}

#ifdef XSERVER
void
pc_xmode_on(void)
{
#ifdef COMPAT_10
	struct trapframe *fp;
#endif

	if (pc_xmode)
		return;
	pc_xmode = 1;

#ifdef XFREE86_BUG_COMPAT
	/* If still unchanged, get current shape. */
	if (cursor_shape == 0xffff)
		get_cursor_shape();
#endif

#ifdef COMPAT_10
	/* This is done by i386_iopl(3) now. */
	fp = curlwp->l_md.md_regs;
	fp->tf_eflags |= PSL_IOPL;
#endif
}

void
pc_xmode_off(void)
{
	struct trapframe *fp;

	if (pc_xmode == 0)
		return;
	pc_xmode = 0;

#ifdef XFREE86_BUG_COMPAT
	/* XXX It would be hard to justify why the X server doesn't do this. */
	set_cursor_shape();
#endif
	async_update();

	fp = curlwp->l_md.md_regs;
	fp->tf_eflags &= ~PSL_IOPL;
}
#endif /* XSERVER */
