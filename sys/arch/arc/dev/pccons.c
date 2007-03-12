/*	$NetBSD: pccons.c,v 1.48.4.1 2007/03/12 05:46:48 rmind Exp $	*/
/*	$OpenBSD: pccons.c,v 1.22 1999/01/30 22:39:37 imp Exp $	*/
/*	NetBSD: pccons.c,v 1.89 1995/05/04 19:35:20 cgd Exp	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
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

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pccons.c,v 1.48.4.1 2007/03/12 05:46:48 rmind Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/callout.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/kcore.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <machine/bus.h>

#include <dev/ic/pcdisplay.h>
#include <machine/pccons.h>
#include <machine/kbdreg.h>

#include <dev/cons.h>
#include <dev/isa/isavar.h>

#include <arc/arc/arcbios.h>
#include <arc/dev/pcconsvar.h>

#include "ioconf.h"

#define	XFREE86_BUG_COMPAT

#ifndef BEEP_FREQ
#define BEEP_FREQ 1600
#endif
#ifndef BEEP_TIME
#define BEEP_TIME (hz/5)
#endif

#define PCBURST 128

static u_short *Crtat;			/* pointer to backing store */
static u_short *crtat;			/* pointer to current char */
static u_char async, kernel, polling;	/* Really, you don't want to know. */
static u_char lock_state = 0x00,	/* all off */
	      old_lock_state = 0xff,
	      typematic_rate = 0xff,	/* don't update until set by user */
	      old_typematic_rate = 0xff;
static u_short cursor_shape = 0xffff,	/* don't update until set by user */
	       old_cursor_shape = 0xffff;
static pccons_keymap_t scan_codes[KB_NUM_KEYS];/* keyboard translation table */
int pc_xmode = 0;

/*
 *  Keyboard output queue.
 */
int	kb_oq_put = 0;
int	kb_oq_get = 0;
u_char	kb_oq[8];

#define	PCUNIT(x)	(minor(x))

static struct video_state {
	int 	cx, cy;		/* escape parameters */
	int 	row, col;	/* current cursor position */
	int 	nrow, ncol, nchr;	/* current screen geometry */
	int	offset;		/* Saved cursor pos */
	u_char	state;		/* parser state */
#define	VSS_ESCAPE	1
#define	VSS_EBRACE	2
#define	VSS_EPARAM	3
	char	so;		/* in standout mode? */
	char	color;		/* color or mono display */
	char	at;		/* normal attributes */
	char	so_at;		/* standout attributes */
} vs;

static struct callout async_update_ch = CALLOUT_INITIALIZER;

void pc_xmode_on(void);
void pc_xmode_off(void);
static u_char kbc_get8042cmd(void);
int kbd_cmd(u_char, u_char);
static inline int kbd_wait_output(void);
static inline int kbd_wait_input(void);
void kbd_flush_input(void);
void set_cursor_shape(void);
void get_cursor_shape(void);
void async_update(void);
void do_async_update(u_char);

void pccnputc(dev_t, int c);
int pccngetc(dev_t);
void pccnpollc(dev_t, int);

dev_type_open(pcopen);
dev_type_close(pcclose);
dev_type_read(pcread);
dev_type_write(pcwrite);
dev_type_ioctl(pcioctl);
dev_type_tty(pctty);
dev_type_poll(pcpoll);
dev_type_mmap(pcmmap);

const struct cdevsw pc_cdevsw = {
	pcopen, pcclose, pcread, pcwrite, pcioctl,
	nostop, pctty, pcpoll, pcmmap, ttykqfilter, D_TTY
};

#define	CHR		2

char *sget(void);
void sput(const u_char *, int);

void	pcstart(struct tty *);
int	pcparam(struct tty *, struct termios *);
static inline void wcopy(void *, void *, u_int);
void	pc_context_init(bus_space_tag_t, bus_space_tag_t, bus_space_tag_t,
	    struct pccons_config *);

extern void fillw(int, uint16_t *, int);

#define	KBD_DELAY \
		DELAY(10);

#define crtc_read_1(reg) \
	bus_space_read_1(pccons_console_context.pc_crt_iot, \
	    pccons_console_context.pc_6845_ioh, reg)
#define crtc_write_1(reg, data) \
	bus_space_write_1(pccons_console_context.pc_crt_iot, \
	    pccons_console_context.pc_6845_ioh, reg, data)

struct pccons_context pccons_console_context;

void
kbd_context_init(bus_space_tag_t kbd_iot, struct pccons_config *config)
{
	struct pccons_kbd_context *pkc = &pccons_console_context.pc_pkc;

	if (pkc->pkc_initialized)
		return;
	pkc->pkc_initialized = 1;

	pkc->pkc_iot = kbd_iot;

	bus_space_map(kbd_iot, config->pc_kbd_cmdp, 1, 0,
	    &pkc->pkc_cmd_ioh);
	bus_space_map(kbd_iot, config->pc_kbd_datap, 1, 0,
	    &pkc->pkc_data_ioh);
}

void
pc_context_init(bus_space_tag_t crt_iot, bus_space_tag_t crt_memt,
    bus_space_tag_t kbd_iot, struct pccons_config *config)
{
	struct pccons_context *pc = &pccons_console_context;

	if (pc->pc_initialized)
		return;
	pc->pc_initialized = 1;

	kbd_context_init(kbd_iot, config);

	pc->pc_crt_iot = crt_iot;
	pc->pc_crt_memt = crt_memt;

	bus_space_map(crt_iot, config->pc_mono_iobase, 2, 0,
	    &pc->pc_mono_ioh);
	bus_space_map(crt_memt, config->pc_mono_memaddr, 0x20000, 0,
	    &pc->pc_mono_memh);
	bus_space_map(crt_iot, config->pc_cga_iobase, 2, 0,
	    &pc->pc_cga_ioh);
	bus_space_map(crt_memt, config->pc_cga_memaddr, 0x20000, 0,
	    &pc->pc_cga_memh);

	/*
	 * pc->pc_6845_ioh and pc->pc_crt_memh will be initialized later,
	 * when `Crtat' is initialized.
	 */

	pc->pc_config = config;

	(*config->pc_init)();
}

/*
 * bcopy variant that only moves word-aligned 16-bit entities,
 * for stupid VGA cards.  cnt is required to be an even vale.
 */
static inline void
wcopy(void *src, void *tgt, u_int cnt)
{
	uint16_t *from = src;
	uint16_t *to = tgt;

	cnt >>= 1;
	if (to < from || to >= from + cnt)
		while (cnt--)
			*to++ = *from++;
	else {
		to += cnt;
		from += cnt;
		while (cnt--)
			*--to = *--from;
	}
}

static inline int
kbd_wait_output(void)
{
	u_int i;

	for (i = 100000; i; i--)
		if ((kbd_cmd_read_1() & KBS_IBF) == 0) {
			KBD_DELAY;
			return 1;
		}
	return 0;
}

static inline int
kbd_wait_input(void)
{
	u_int i;

	for (i = 100000; i; i--)
		if ((kbd_cmd_read_1() & KBS_DIB) != 0) {
			KBD_DELAY;
			return 1;
		}
	return 0;
}

void
kbd_flush_input(void)
{
	uint8_t c;

	while ((c = kbd_cmd_read_1()) & 0x03)
		if ((c & KBS_DIB) == KBS_DIB) {
			/* XXX - delay is needed to prevent some keyboards from
			   wedging when the system boots */
			delay(6);
			(void)kbd_data_read_1();
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
		return -1;
	kbd_cmd_write_1(K_RDCMDBYTE);
	if (!kbd_wait_input())
		return -1;
	return kbd_data_read_1();
}
#endif

/*
 * Pass command byte to keyboard controller (8042).
 */
int
kbc_put8042cmd(val)
	uint8_t val;
{

	if (!kbd_wait_output())
		return 0;
	kbd_cmd_write_1(K_LDCMDBYTE);
	if (!kbd_wait_output())
		return 0;
	kbd_data_write_1(val);
	return 1;
}

/*
 * Pass command to keyboard itself
 */
int
kbd_cmd(uint8_t val, uint8_t polled)
{
	u_int retries = 3;
	u_int i;

	if (!polled) {
		i = spltty();
		if (kb_oq_get == kb_oq_put) {
			kbd_data_write_1(val);
		}
		kb_oq[kb_oq_put] = val;
		kb_oq_put = (kb_oq_put + 1) & 7;
		splx(i);
		return 1;
	}

	do {
		if (!kbd_wait_output())
			return 0;
		kbd_data_write_1(val);
		for (i = 100000; i; i--) {
			if (kbd_cmd_read_1() & KBS_DIB) {
				uint8_t c;

				KBD_DELAY;
				c = kbd_data_read_1();
				if (c == KBR_ACK || c == KBR_ECHO) {
					return 1;
				}
				if (c == KBR_RESEND) {
					break;
				}
#ifdef DIAGNOSTIC
				printf("kbd_cmd: input char %x lost\n", c);
#endif
			}
		}
	} while (--retries);
	return 0;
}

void
set_cursor_shape(void)
{

	crtc_write_1(0, 10);
	crtc_write_1(1, cursor_shape >> 8);
	crtc_write_1(0, 11);
	crtc_write_1(1, cursor_shape);
	old_cursor_shape = cursor_shape;
}

void
get_cursor_shape(void)
{

	crtc_write_1(0, 10);
	cursor_shape = crtc_read_1(1) << 8;
	crtc_write_1(0, 11);
	cursor_shape |= crtc_read_1(1);

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

void
do_async_update(uint8_t poll)
{
	int pos;
	static int old_pos = -1;

	async = 0;

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

	if (pc_xmode > 0)
		return;

	pos = crtat - Crtat;
	if (pos != old_pos) {
		crtc_write_1(0, 14);
		crtc_write_1(1, pos >> 8);
		crtc_write_1(0, 15);
		crtc_write_1(1, pos);
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
		do_async_update(1);
	} else {
		if (async)
			return;
		async = 1;
		callout_reset(&async_update_ch, 1,
		    (void(*)(void *))do_async_update, NULL);
	}
}

/*
 * these are both bad jokes
 */
int
pccons_common_match(bus_space_tag_t crt_iot, bus_space_tag_t crt_memt,
    bus_space_tag_t kbd_iot, struct pccons_config *config)
{
	int i;

	pc_context_init(crt_iot, crt_memt, kbd_iot, config);

	/* Enable interrupts and keyboard, etc. */
	if (!kbc_put8042cmd(CMDBYTE)) {
		printf("pcprobe: command error\n");
		return 0;
	}

#if 1
	/* Flush any garbage. */
	kbd_flush_input();
	/* Reset the keyboard. */
	if (!kbd_cmd(KBC_RESET, 1)) {
		printf("pcprobe: reset error %d\n", 1);
		goto lose;
	}
	for (i = 600000; i; i--)
		if ((kbd_cmd_read_1() & KBS_DIB) != 0) {
			KBD_DELAY;
			break;
		}
	if (i == 0 || kbd_data_read_1() != KBR_RSTDONE) {
		printf("pcprobe: reset error %d\n", 2);
		goto lose;
	}
	/*
	 * Some keyboards seem to leave a second ack byte after the reset.
	 * This is kind of stupid, but we account for them anyway by just
	 * flushing the buffer.
	 */
	kbd_flush_input();
	/* Just to be sure. */
	if (!kbd_cmd(KBC_ENABLE, 1)) {
		printf("pcprobe: reset error %d\n", 3);
		goto lose;
	}

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

lose:
	/*
	 * Technically, we should probably fail the probe.  But we'll be nice
	 * and allow keyboard-less machines to boot with the console.
	 */
#endif

	return 1;
}

void pccons_common_attach(struct pc_softc *sc, bus_space_tag_t crt_iot,
    bus_space_tag_t crt_memt, bus_space_tag_t kbd_iot,
    struct pccons_config *config)
{

	printf(": %s\n", vs.color ? "color" : "mono");
	do_async_update(1);
}

int
pcopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pc_softc *sc;
	int unit = PCUNIT(dev);
	struct tty *tp;

	if (unit >= pc_cd.cd_ndevs)
		return ENXIO;
	sc = pc_cd.cd_devs[unit];
	if (sc == 0)
		return ENXIO;

	if (!sc->sc_tty) {
		tp = sc->sc_tty = ttymalloc();
	}
	else {
		tp = sc->sc_tty;
	}

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

	return (*tp->t_linesw->l_open)(dev, tp);
}

int
pcclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
#ifdef notyet /* XXX */
	ttyfree(tp);
#endif
	return 0;
}

int
pcread(dev_t dev, struct uio *uio, int flag)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
pcwrite(dev_t dev, struct uio *uio, int flag)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
pcpoll(dev_t dev, int events, struct lwp *l)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
pctty(dev_t dev)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return tp;
}

/*
 * Got a console receive interrupt -
 * the console processor wants to give us a character.
 * Catch the character, and see who it goes to.
 */
int
pcintr(void *arg)
{
	struct pc_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	uint8_t *cp;

	if ((kbd_cmd_read_1() & KBS_DIB) == 0)
		return 0;
	if (polling)
		return 1;
	do {
		cp = sget();
		if (!tp || (tp->t_state & TS_ISOPEN) == 0)
			return 1;
		if (cp)
			do
				(*tp->t_linesw->l_rint)(*cp++, tp);
			while (*cp);
	} while (kbd_cmd_read_1() & KBS_DIB);
	return 1;
}

int
pcioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;
	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	switch (cmd) {
	case CONSOLE_X_MODE_ON:
		pc_xmode_on();
		return 0;
	case CONSOLE_X_MODE_OFF:
		pc_xmode_off();
		return 0;
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
		return 0;
	case CONSOLE_SET_TYPEMATIC_RATE: {
 		u_char	rate;

 		if (!data)
			return EINVAL;
		rate = *((u_char *)data);
		/*
		 * Check that it isn't too big (which would cause it to be
		 * confused with a command).
		 */
		if (rate & 0x80)
			return EINVAL;
		typematic_rate = rate;
		async_update();
		return 0;
 	}
	case CONSOLE_SET_KEYMAP: {
		pccons_keymap_t *map = (pccons_keymap_t *) data;
		int i;

		if (!data)
			return EINVAL;
		for (i = 0; i < KB_NUM_KEYS; i++)
			if (map[i].unshift[KB_CODE_SIZE-1] ||
			    map[i].shift[KB_CODE_SIZE-1] ||
			    map[i].ctl[KB_CODE_SIZE-1] ||
			    map[i].altgr[KB_CODE_SIZE-1] ||
			    map[i].shift_altgr[KB_CODE_SIZE-1])
				return EINVAL;

		bcopy(data, scan_codes, sizeof(pccons_keymap_t[KB_NUM_KEYS]));
		return 0;
	}
	case CONSOLE_GET_KEYMAP:
		if (!data)
			return EINVAL;
		bcopy(scan_codes, data, sizeof(pccons_keymap_t[KB_NUM_KEYS]));
		return 0;

	default:
		return EPASSTHROUGH;
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

/* ARGSUSED */
void pccons_common_cnattach(bus_space_tag_t crt_iot, bus_space_tag_t crt_memt,
    bus_space_tag_t kbd_iot, struct pccons_config *config)
{
	int maj;
	static struct consdev pccons = {
		NULL, NULL, pccngetc, pccnputc, pccnpollc, NULL, NULL,
		    NULL, NODEV, CN_NORMAL
	};

	/*
	 * For now, don't screw with it.
	 */
	/* crtat = 0; */

	pc_context_init(crt_iot, crt_memt, kbd_iot, config);

	/* locate the major number */
	maj = cdevsw_lookup_major(&pc_cdevsw);
	pccons.cn_dev = makedev(maj, 0);

	cn_tab = &pccons;
}

/* ARGSUSED */
void
pccnputc(dev_t dev, int c)
{
	u_char cc, oldkernel = kernel;

	kernel = 1;
	if (c == '\n') {
		sput("\r\n", 2);
	} else {
		cc = c;
		sput(&cc, 1);
	}
	kernel = oldkernel;
}

/* ARGSUSED */
int
pccngetc(dev_t dev)
{
	char *cp;

	if (pc_xmode > 0)
		return 0;

	do {
		/* wait for byte */
		while ((kbd_cmd_read_1() & KBS_DIB) == 0);
		/* see if it's worthwhile */
		cp = sget();
	} while (!cp);
	if (*cp == '\r')
		return '\n';
	return *cp;
}

void
pccnpollc(dev_t dev, int on)
{

	polling = on;
	if (!on) {
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
	return 0;
}

#define	wrtchar(c, at) do {\
	char *cp0 = (char *)crtat; *cp0++ = (c); *cp0 = (at); crtat++; vs.col++; \
} while (0)

/* translate ANSI color codes to standard pc ones */
static char fgansitopc[] = {
	FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
	FG_MAGENTA, FG_CYAN, FG_LIGHTGREY
};

static char bgansitopc[] = {
	BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
	BG_MAGENTA, BG_CYAN, BG_LIGHTGREY
};

static u_char iso2ibm437[] =
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

/*
 * `pc3' termcap emulation.
 */
void
sput(const u_char *cp, int n)
{
	struct pccons_context *pc = &pccons_console_context;
	u_char c, scroll = 0;

	if (pc_xmode > 0)
		return;

	if (crtat == 0) {
		volatile u_short *dp;
		u_short was;
		unsigned cursorat;

		dp = bus_space_vaddr(pc->pc_crt_memt, pc->pc_cga_memh);
		was = *dp;
		*dp = 0xA55A;
		if (*dp != 0xA55A) {
			dp = bus_space_vaddr(pc->pc_crt_memt,
			    pc->pc_mono_memh);
			pc->pc_6845_ioh = pc->pc_mono_ioh;
			pc->pc_crt_memh = pc->pc_mono_memh;
			vs.color = 0;
		} else {
			*dp = was;
			pc->pc_6845_ioh = pc->pc_cga_ioh;
			pc->pc_crt_memh = pc->pc_cga_memh;
			vs.color = 1;
		}

#ifdef FAT_CURSOR
		cursor_shape = 0x0012;
#else
		get_cursor_shape();
#endif

		bios_display_info(&vs.col, &vs.row, &vs.ncol, &vs.nrow);
		vs.nchr = vs.ncol * vs.nrow;
		vs.col--;
		vs.row--;
		cursorat = vs.ncol * vs.row + vs.col;
		vs.at = FG_LIGHTGREY | BG_BLACK;

		Crtat = (u_short *)__UNVOLATILE(dp);
		crtat = Crtat + cursorat;

		if (vs.color == 0)
			vs.so_at = FG_BLACK | BG_LIGHTGREY;
		else
			vs.so_at = FG_YELLOW | BG_BLACK;

		fillw((vs.at << 8) | ' ', crtat, vs.nchr - cursorat);
	}

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

		case 0x9B:	/* CSI */
			vs.cx = vs.cy = 0;
			vs.state = VSS_EBRACE;
			break;

		case '\t': {
			int inccol = 8 - (vs.col & 7);
			crtat += inccol;
			vs.col += inccol;
		}
		maybe_scroll:
			if (vs.col >= vs.ncol) {
				vs.col -= vs.ncol;
				scroll = 1;
			}
			break;

		case '\b':
			if (crtat <= Crtat)
				break;
			--crtat;
			if (--vs.col < 0)
				vs.col += vs.ncol;	/* non-destructive backspace */
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
						if (c & 0x80)
							c = iso2ibm437[c&0x7f];

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
				switch (c) {
					case '[': /* Start ESC [ sequence */
						vs.cx = vs.cy = 0;
						vs.state = VSS_EBRACE;
						break;
					case 'c': /* Create screen & home */
						fillw((vs.at << 8) | ' ',
						    Crtat, vs.nchr);
						crtat = Crtat;
						vs.col = 0;
						vs.state = 0;
						break;
					case '7': /* save cursor pos */
						vs.offset = crtat - Crtat;
						vs.state = 0;
						break;
					case '8': /* restore cursor pos */
						crtat = Crtat + vs.offset;
						vs.row = vs.offset / vs.ncol;
						vs.col = vs.offset % vs.ncol;
						vs.state = 0;
						break;
					default: /* Invalid, clear state */
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
						    Crtat,
						    crtat - Crtat + 1);
						break;
					case 2:
						/* ... whole display */
						fillw((vs.at << 8) | ' ',
						    Crtat,
						    vs.nchr);
						break;
					}
					vs.state = 0;
					break;
				case 'K': /* Clear line ... */
					switch (vs.cx) {
					case 0:
						/* ... current to EOL */
						fillw((vs.at << 8) | ' ',
						    crtat,
						    vs.ncol - vs.col);
						break;
					case 1:
						/* ... beginning to next */
						fillw((vs.at << 8) | ' ',
						    crtat - vs.col,
						    vs.col + 1);
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
#ifdef PCCONS_FORCE_WORD
						wcopy(crtAt + vs.ncol * cx,
						    crtAt, vs.ncol * (nrow -
						    cx) * CHR);
#else
						bcopy(crtAt + vs.ncol * cx,
						    crtAt, vs.ncol * (nrow -
						    cx) * CHR);
#endif
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
#ifdef PCCONS_FORCE_WORD
						wcopy(Crtat + vs.ncol * cx,
						    Crtat, vs.ncol * (vs.nrow -
						    cx) * CHR);
#else
						bcopy(Crtat + vs.ncol * cx,
						    Crtat, vs.ncol * (vs.nrow -
						    cx) * CHR);
#endif
					fillw((vs.at << 8) | ' ',
					    Crtat + vs.ncol * (vs.nrow - cx),
					    vs.ncol * cx);
					/* crtat -= vs.ncol * cx; XXX */
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
#ifdef PCCONS_FORCE_WORD
						wcopy(crtAt,
						    crtAt + vs.ncol * cx,
						    vs.ncol * (nrow - cx) *
						    CHR);
#else
						bcopy(crtAt,
						    crtAt + vs.ncol * cx,
						    vs.ncol * (nrow - cx) *
						    CHR);
#endif
					fillw((vs.at << 8) | ' ', crtAt,
					    vs.ncol * cx);
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
#ifdef PCCONS_FORCE_WORD
						wcopy(Crtat,
						    Crtat + vs.ncol * cx,
						    vs.ncol * (vs.nrow - cx) *
						    CHR);
#else
						bcopy(Crtat,
						    Crtat + vs.ncol * cx,
						    vs.ncol * (vs.nrow - cx) *
						    CHR);
#endif
					fillw((vs.at << 8) | ' ', Crtat,
					    vs.ncol * cx);
					/* crtat += vs.ncol * cx; XXX */
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
				case 's': /* save cursor pos */
					vs.offset = crtat - Crtat;
					vs.state = 0;
					break;
				case 'u': /* restore cursor pos */
					crtat = Crtat + vs.offset;
					vs.row = vs.offset / vs.ncol;
					vs.col = vs.offset % vs.ncol;
					vs.state = 0;
					break;
				case 'x': /* set attributes */
					switch (vs.cx) {
					case 0:
						vs.at = FG_LIGHTGREY | BG_BLACK;
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
				if (!kernel) {
					int s = spltty();
					if (lock_state & KB_SCROLL)
						tsleep(&lock_state,
						    PUSER, "pcputc", 0);
					splx(s);
				}
#if PCCONS_FORCE_WORD
				wcopy(Crtat + vs.ncol, Crtat,
				    (vs.nchr - vs.ncol) * CHR);
#else
				bcopy(Crtat + vs.ncol, Crtat,
				    (vs.nchr - vs.ncol) * CHR);
#endif
				fillw((vs.at << 8) | ' ',
				    Crtat + vs.nchr - vs.ncol,
				    vs.ncol);
				crtat -= vs.ncol;
			}
		}
	}
	async_update();
}

/* the unshifted code for KB_SHIFT keys is used by X to distinguish between
   left and right shift when reading the keyboard map */
static pccons_keymap_t	scan_codes[KB_NUM_KEYS] = {
/*  type       unshift   shift     control   altgr     shift_altgr scancode */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 0 unused */
  { KB_ASCII,  "\033",   "\033",   "\033",   "",       ""}, /* 1 ESCape */
  { KB_ASCII,  "1",      "!",      "!",      "",       ""}, /* 2 1 */
  { KB_ASCII,  "2",      "@",      "\000",   "",       ""}, /* 3 2 */
  { KB_ASCII,  "3",      "#",      "#",      "",       ""}, /* 4 3 */
  { KB_ASCII,  "4",      "$",      "$",      "",       ""}, /* 5 4 */
  { KB_ASCII,  "5",      "%",      "%",      "",       ""}, /* 6 5 */
  { KB_ASCII,  "6",      "^",      "\036",   "",       ""}, /* 7 6 */
  { KB_ASCII,  "7",      "&",      "&",      "",       ""}, /* 8 7 */
  { KB_ASCII,  "8",      "*",      "\010",   "",       ""}, /* 9 8 */
  { KB_ASCII,  "9",      "(",      "(",      "",       ""}, /* 10 9 */
  { KB_ASCII,  "0",      ")",      ")",      "",       ""}, /* 11 0 */
  { KB_ASCII,  "-",      "_",      "\037",   "",       ""}, /* 12 - */
  { KB_ASCII,  "=",      "+",      "+",      "",       ""}, /* 13 = */
  { KB_ASCII,  "\177",   "\177",   "\010",   "",       ""}, /* 14 backspace */
  { KB_ASCII,  "\t",     "\t",     "\t",     "",       ""}, /* 15 tab */
  { KB_ASCII,  "q",      "Q",      "\021",   "",       ""}, /* 16 q */
  { KB_ASCII,  "w",      "W",      "\027",   "",       ""}, /* 17 w */
  { KB_ASCII,  "e",      "E",      "\005",   "",       ""}, /* 18 e */
  { KB_ASCII,  "r",      "R",      "\022",   "",       ""}, /* 19 r */
  { KB_ASCII,  "t",      "T",      "\024",   "",       ""}, /* 20 t */
  { KB_ASCII,  "y",      "Y",      "\031",   "",       ""}, /* 21 y */
  { KB_ASCII,  "u",      "U",      "\025",   "",       ""}, /* 22 u */
  { KB_ASCII,  "i",      "I",      "\011",   "",       ""}, /* 23 i */
  { KB_ASCII,  "o",      "O",      "\017",   "",       ""}, /* 24 o */
  { KB_ASCII,  "p",      "P",      "\020",   "",       ""}, /* 25 p */
  { KB_ASCII,  "[",      "{",      "\033",   "",       ""}, /* 26 [ */
  { KB_ASCII,  "]",      "}",      "\035",   "",       ""}, /* 27 ] */
  { KB_ASCII,  "\r",     "\r",     "\n",     "",       ""}, /* 28 return */
  { KB_CTL,    "",       "",       "",       "",       ""}, /* 29 control */
  { KB_ASCII,  "a",      "A",      "\001",   "",       ""}, /* 30 a */
  { KB_ASCII,  "s",      "S",      "\023",   "",       ""}, /* 31 s */
  { KB_ASCII,  "d",      "D",      "\004",   "",       ""}, /* 32 d */
  { KB_ASCII,  "f",      "F",      "\006",   "",       ""}, /* 33 f */
  { KB_ASCII,  "g",      "G",      "\007",   "",       ""}, /* 34 g */
  { KB_ASCII,  "h",      "H",      "\010",   "",       ""}, /* 35 h */
  { KB_ASCII,  "j",      "J",      "\n",     "",       ""}, /* 36 j */
  { KB_ASCII,  "k",      "K",      "\013",   "",       ""}, /* 37 k */
  { KB_ASCII,  "l",      "L",      "\014",   "",       ""}, /* 38 l */
  { KB_ASCII,  ";",      ":",      ";",      "",       ""}, /* 39 ; */
  { KB_ASCII,  "'",      "\"",     "'",      "",       ""}, /* 40 ' */
  { KB_ASCII,  "`",      "~",      "`",      "",       ""}, /* 41 ` */
  { KB_SHIFT,  "\001",   "",       "",       "",       ""}, /* 42 shift */
  { KB_ASCII,  "\\",     "|",      "\034",   "",       ""}, /* 43 \ */
  { KB_ASCII,  "z",      "Z",      "\032",   "",       ""}, /* 44 z */
  { KB_ASCII,  "x",      "X",      "\030",   "",       ""}, /* 45 x */
  { KB_ASCII,  "c",      "C",      "\003",   "",       ""}, /* 46 c */
  { KB_ASCII,  "v",      "V",      "\026",   "",       ""}, /* 47 v */
  { KB_ASCII,  "b",      "B",      "\002",   "",       ""}, /* 48 b */
  { KB_ASCII,  "n",      "N",      "\016",   "",       ""}, /* 49 n */
  { KB_ASCII,  "m",      "M",      "\r",     "",       ""}, /* 50 m */
  { KB_ASCII,  ",",      "<",      "<",      "",       ""}, /* 51 , */
  { KB_ASCII,  ".",      ">",      ">",      "",       ""}, /* 52 . */
  { KB_ASCII,  "/",      "?",      "\037",   "",       ""}, /* 53 / */
  { KB_SHIFT,  "\002",   "",       "",       "",       ""}, /* 54 shift */
  { KB_KP,     "*",      "*",      "*",      "",       ""}, /* 55 kp * */
  { KB_ALT,    "",       "",       "",       "",       ""}, /* 56 alt */
  { KB_ASCII,  " ",      " ",      "\000",   "",       ""}, /* 57 space */
  { KB_CAPS,   "",       "",       "",       "",       ""}, /* 58 caps */
  { KB_FUNC,   "\033[M", "\033[Y", "\033[k", "",       ""}, /* 59 f1 */
  { KB_FUNC,   "\033[N", "\033[Z", "\033[l", "",       ""}, /* 60 f2 */
  { KB_FUNC,   "\033[O", "\033[a", "\033[m", "",       ""}, /* 61 f3 */
  { KB_FUNC,   "\033[P", "\033[b", "\033[n", "",       ""}, /* 62 f4 */
  { KB_FUNC,   "\033[Q", "\033[c", "\033[o", "",       ""}, /* 63 f5 */
  { KB_FUNC,   "\033[R", "\033[d", "\033[p", "",       ""}, /* 64 f6 */
  { KB_FUNC,   "\033[S", "\033[e", "\033[q", "",       ""}, /* 65 f7 */
  { KB_FUNC,   "\033[T", "\033[f", "\033[r", "",       ""}, /* 66 f8 */
  { KB_FUNC,   "\033[U", "\033[g", "\033[s", "",       ""}, /* 67 f9 */
  { KB_FUNC,   "\033[V", "\033[h", "\033[t", "",       ""}, /* 68 f10 */
  { KB_NUM,    "",       "",       "",       "",       ""}, /* 69 num lock */
  { KB_SCROLL, "",       "",       "",       "",       ""}, /* 70 scroll lock */
  { KB_KP,     "7",      "\033[H", "7",      "",       ""}, /* 71 kp 7 */
  { KB_KP,     "8",      "\033[A", "8",      "",       ""}, /* 72 kp 8 */
  { KB_KP,     "9",      "\033[I", "9",      "",       ""}, /* 73 kp 9 */
  { KB_KP,     "-",      "-",      "-",      "",       ""}, /* 74 kp - */
  { KB_KP,     "4",      "\033[D", "4",      "",       ""}, /* 75 kp 4 */
  { KB_KP,     "5",      "\033[E", "5",      "",       ""}, /* 76 kp 5 */
  { KB_KP,     "6",      "\033[C", "6",      "",       ""}, /* 77 kp 6 */
  { KB_KP,     "+",      "+",      "+",      "",       ""}, /* 78 kp + */
  { KB_KP,     "1",      "\033[F", "1",      "",       ""}, /* 79 kp 1 */
  { KB_KP,     "2",      "\033[B", "2",      "",       ""}, /* 80 kp 2 */
  { KB_KP,     "3",      "\033[G", "3",      "",       ""}, /* 81 kp 3 */
  { KB_KP,     "0",      "\033[L", "0",      "",       ""}, /* 82 kp 0 */
  { KB_KP,     ",",      "\177",   ",",      "",       ""}, /* 83 kp , */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 84 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 85 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 86 0 */
  { KB_FUNC,   "\033[W", "\033[i", "\033[u", "",       ""}, /* 87 f11 */
  { KB_FUNC,   "\033[X", "\033[j", "\033[v", "",       ""}, /* 88 f12 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 89 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 90 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 91 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 92 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 93 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 94 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 95 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 96 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 97 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 98 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 99 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 100 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 101 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 102 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 103 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 104 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 105 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 106 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 107 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 108 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 109 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 110 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 111 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 112 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 113 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 114 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 115 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 116 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 117 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 118 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 119 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 120 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 121 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 122 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 123 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 124 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 125 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 126 */
  { KB_NONE,   "",       "",       "",       "",       ""}  /* 127 */
};

/*
 * Get characters from the keyboard.  If none are present, return NULL.
 */
char *
sget(void)
{
	u_char dt;
	static u_char extended = 0, shift_state = 0;
	static u_char capchar[2];

top:
	KBD_DELAY;
	dt = kbd_data_read_1();

	switch (dt) {
	case KBR_ACK: case KBR_ECHO:
		kb_oq_get = (kb_oq_get + 1) & 7;
		if(kb_oq_get != kb_oq_put) {
			kbd_data_write_1(kb_oq[kb_oq_get]);
		}
		goto loop;
	case KBR_RESEND:
		kbd_data_write_1(kb_oq[kb_oq_get]);
		goto loop;
	}

	if (pc_xmode > 0) {
#if defined(DDB) && defined(XSERVER_DDB)
		/* F12 enters the debugger while in X mode */
		if (dt == 88)
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
		case KB_NUM:
			if (dt & 0x80) {
				shift_state &= ~KB_NUM;
				break;
			}
			if (shift_state & KB_NUM)
				break;
			shift_state |= KB_NUM;
			lock_state ^= KB_NUM;
			async_update();
			break;
		case KB_CAPS:
			if (dt & 0x80) {
				shift_state &= ~KB_CAPS;
				break;
			}
			if (shift_state & KB_CAPS)
				break;
			shift_state |= KB_CAPS;
			lock_state ^= KB_CAPS;
			async_update();
			break;
		case KB_SCROLL:
			if (dt & 0x80) {
				shift_state &= ~KB_SCROLL;
				break;
			}
			if (shift_state & KB_SCROLL)
				break;
			shift_state |= KB_SCROLL;
			lock_state ^= KB_SCROLL;
			if ((lock_state & KB_SCROLL) == 0)
				wakeup((void *)&lock_state);
			async_update();
			break;
		}
		return capchar;
	}

	switch (dt) {
	case KBR_EXTENDED:
		extended = 1;
		goto loop;
	}

#ifdef DDB
	/*
	 * Check for cntl-alt-esc.
	 */
	if ((dt == 1) && (shift_state & (KB_CTL | KB_ALT)) == (KB_CTL | KB_ALT)) {
		/* XXX - check pccons_is_console */
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
		case KB_NUM:
			shift_state &= ~KB_NUM;
			break;
		case KB_CAPS:
			shift_state &= ~KB_CAPS;
			break;
		case KB_SCROLL:
			shift_state &= ~KB_SCROLL;
			break;
		case KB_SHIFT:
			shift_state &= ~KB_SHIFT;
			break;
		case KB_ALT:
			if (extended)
				shift_state &= ~KB_ALTGR;
			else
				shift_state &= ~KB_ALT;
			break;
		case KB_CTL:
			shift_state &= ~KB_CTL;
			break;
		}
	} else {
		/*
		 * make
		 */
		switch (scan_codes[dt].type) {
		/*
		 * locking keys
		 */
		case KB_NUM:
			if (shift_state & KB_NUM)
				break;
			shift_state |= KB_NUM;
			lock_state ^= KB_NUM;
			async_update();
			break;
		case KB_CAPS:
			if (shift_state & KB_CAPS)
				break;
			shift_state |= KB_CAPS;
			lock_state ^= KB_CAPS;
			async_update();
			break;
		case KB_SCROLL:
			if (shift_state & KB_SCROLL)
				break;
			shift_state |= KB_SCROLL;
			lock_state ^= KB_SCROLL;
			if ((lock_state & KB_SCROLL) == 0)
				wakeup((void *)&lock_state);
			async_update();
			break;
		/*
		 * non-locking keys
		 */
		case KB_SHIFT:
			shift_state |= KB_SHIFT;
			break;
		case KB_ALT:
			if (extended)
				shift_state |= KB_ALTGR;
			else
				shift_state |= KB_ALT;
			break;
		case KB_CTL:
			shift_state |= KB_CTL;
			break;
		case KB_ASCII:
			/* control has highest priority */
			if (shift_state & KB_CTL)
				capchar[0] = scan_codes[dt].ctl[0];
			else if (shift_state & KB_ALTGR) {
				if (shift_state & KB_SHIFT)
					capchar[0] = scan_codes[dt].shift_altgr[0];
				else
					capchar[0] = scan_codes[dt].altgr[0];
			}
			else {
				if (shift_state & KB_SHIFT)
					capchar[0] = scan_codes[dt].shift[0];
				else
					capchar[0] = scan_codes[dt].unshift[0];
			}
			if ((lock_state & KB_CAPS) && capchar[0] >= 'a' &&
			    capchar[0] <= 'z') {
				capchar[0] -= ('a' - 'A');
			}
			capchar[0] |= (shift_state & KB_ALT);
			extended = 0;
			return capchar;
		case KB_NONE:
printf("keycode %d\n",dt);
			break;
		case KB_FUNC: {
			char *more_chars;
			if (shift_state & KB_SHIFT)
				more_chars = scan_codes[dt].shift;
			else if (shift_state & KB_CTL)
				more_chars = scan_codes[dt].ctl;
			else
				more_chars = scan_codes[dt].unshift;
			extended = 0;
			return more_chars;
		}
		case KB_KP: {
			char *more_chars;
			if (shift_state & (KB_SHIFT | KB_CTL) ||
			    (lock_state & KB_NUM) == 0 || extended)
				more_chars = scan_codes[dt].shift;
			else
				more_chars = scan_codes[dt].unshift;
			extended = 0;
			return more_chars;
		}
		}
	}

	extended = 0;
loop:
	if ((kbd_cmd_read_1() & KBS_DIB) == 0)
		return 0;
	goto top;
}

paddr_t
pcmmap(dev_t dev, off_t offset, int nprot)
{
	struct pccons_context *pc = &pccons_console_context;
	paddr_t pa;

	if (offset >= 0xa0000 && offset < 0xc0000) {
		if (bus_space_paddr(pc->pc_crt_memt, pc->pc_mono_memh, &pa))
			return -1;
		pa += offset - pc->pc_config->pc_mono_memaddr;
		return mips_btop(pa);
	}
	if (offset >= 0x0000 && offset < 0x10000) {
		if (bus_space_paddr(pc->pc_crt_iot, pc->pc_mono_ioh, &pa))
			return -1;
		pa += offset - pc->pc_config->pc_mono_iobase;
		return mips_btop(pa);
	}
	if (offset >= 0x40000000 && offset < 0x40800000) {
		if (bus_space_paddr(pc->pc_crt_memt, pc->pc_mono_memh, &pa))
			return (-1);
		pa += offset - 0x40000000 - pc->pc_config->pc_mono_memaddr;
		return mips_btop(pa);
	}
	return -1;
}

void
pc_xmode_on(void)
{
	if (pc_xmode)
		return;
	pc_xmode = 1;

#ifdef XFREE86_BUG_COMPAT
	/* If still unchanged, get current shape. */
	if (cursor_shape == 0xffff)
		get_cursor_shape();
#endif
}

void
pc_xmode_off(void)
{
	if (pc_xmode == 0)
		return;
	pc_xmode = 0;

#ifdef XFREE86_BUG_COMPAT
	/* XXX It would be hard to justify why the X server doesn't do this. */
	set_cursor_shape();
#endif
	async_update();
}
