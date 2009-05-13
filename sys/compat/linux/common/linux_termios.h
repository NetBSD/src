/*	$NetBSD: linux_termios.h,v 1.18.10.1 2009/05/13 17:18:57 jym Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

#ifndef _LINUX_TERMIOS_H
#define _LINUX_TERMIOS_H

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_termios.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_termios.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_termios.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_termios.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_termios.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_termios.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_termios.h>
#else
#error Undefined linux_termios.h machine type.
#endif

struct linux_winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

/*
 * LINUX_NCC is architecture dependent. It is now
 * defined in sys/compat/linux/<arch>/linux_termios.h
 */
struct linux_termio {
	unsigned short c_iflag;
	unsigned short c_oflag;
	unsigned short c_cflag;
	unsigned short c_lflag;
	unsigned char c_line;
	unsigned char c_cc[LINUX_NCC];
};

struct linux_termios {
	linux_tcflag_t	c_iflag;
	linux_tcflag_t	c_oflag;
	linux_tcflag_t	c_cflag;
	linux_tcflag_t	c_lflag;
	linux_cc_t	c_line;
	linux_cc_t	c_cc[LINUX_NCCS];
#ifdef LINUX_LARGE_STRUCT_TERMIOS
	/*
    * Present on some linux ports but unused:
	 * However we must enable it, else it breaks ioctl
	 * definitions (the size does not match anymore)
    */
	linux_speed_t	c_ispeed;
	linux_speed_t	c_ospeed;
#endif
};

/* Linux modem line defines.. not sure if they'll be used */
#define LINUX_TIOCM_LE		0x0001
#define LINUX_TIOCM_DTR		0x0002
#define LINUX_TIOCM_RTS		0x0004
#define LINUX_TIOCM_ST		0x0008
#define LINUX_TIOCM_SR		0x0010
#define LINUX_TIOCM_CTS		0x0020
#define LINUX_TIOCM_CAR		0x0040
#define LINUX_TIOCM_RNG		0x0080
#define LINUX_TIOCM_DSR		0x0100
#define LINUX_TIOCM_CD		LINUX_TIOCM_CAR
#define LINUX_TIOCM_RI 		LINUX_TIOCM_RNG

#define	LINUX_TCIFLUSH		0
#define	LINUX_TCOFLUSH		1
#define	LINUX_TCIOFLUSH		2

#define	LINUX_TCOOFF		0
#define	LINUX_TCOON		1
#define	LINUX_TCIOFF		2
#define	LINUX_TCION		3

#define	LINUX_TCSANOW		0
#define	LINUX_TCSADRAIN		1
#define	LINUX_TCSAFLUSH		2

/* Linux line disciplines */
#define LINUX_N_TTY		0
#define LINUX_N_SLIP		1
#define LINUX_N_MOUSE		2
#define LINUX_N_PPP		3
#define	LINUX_N_STRIP		4

/* currently unused: */
#define	LINUX_N_AX25		5
#define	LINUX_N_X25		6
#define	LINUX_N_6PACK		7

/* values passed to TIOCLINUX ioctl */
#define LINUX_TIOCLINUX_COPY		 2
#define LINUX_TIOCLINUX_PASTE		 3
#define LINUX_TIOCLINUX_UNBLANK		 4
#define LINUX_TIOCLINUX_LOADLUT		 5
#define LINUX_TIOCLINUX_READSHIFT	 6
#define LINUX_TIOCLINUX_READMOUSE	 7
#define LINUX_TIOCLINUX_VESABLANK	10
#define LINUX_TIOCLINUX_KERNMSG		11
#define LINUX_TIOCLINUX_CURCONS		12

static linux_speed_t linux_speeds[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 230400
};

static const int linux_spmasks[] = {
	LINUX_B0, LINUX_B50, LINUX_B75, LINUX_B110, LINUX_B134, LINUX_B150,
	LINUX_B200, LINUX_B300, LINUX_B600, LINUX_B1200, LINUX_B1800,
	LINUX_B2400, LINUX_B4800, LINUX_B9600, LINUX_B19200, LINUX_B38400,
	LINUX_B57600, LINUX_B115200, LINUX_B230400
};

#ifdef COMPAT_LINUX32
struct linux32_termio;
struct linux32_termios;
static void linux32_termio_to_bsd_termios(struct linux32_termio *,
					     struct termios *);
static void bsd_termios_to_linux32_termio(struct termios *,
					     struct linux32_termio *);
static void linux32_termios_to_bsd_termios(struct linux32_termios *,
					      struct termios *);
static void bsd_termios_to_linux32_termios(struct termios *,
					      struct linux32_termios *);
#else
struct linux_termio;
struct linux_termios;
static void linux_termio_to_bsd_termios(struct linux_termio *,
					     struct termios *);
static void bsd_termios_to_linux_termio(struct termios *,
					     struct linux_termio *);
static void linux_termios_to_bsd_termios(struct linux_termios *,
					      struct termios *);
static void bsd_termios_to_linux_termios(struct termios *,
					      struct linux_termios *);
#endif

/*
 * Deal with termio ioctl cruft. This doesn't look very good..
 * XXX too much code duplication, obviously..
 *
 * The conversion routines between Linux and BSD structures assume
 * that the fields are already filled with the current values,
 * so that fields present in BSD but not in Linux keep their current
 * values.
 */

static void
#ifdef COMPAT_LINUX32
linux32_termio_to_bsd_termios(struct linux32_termio *lt, struct termios *bts)
#else
linux_termio_to_bsd_termios(struct linux_termio *lt, struct termios *bts)
#endif
{
	int index;

	bts->c_iflag = 0;
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IGNBRK, IGNBRK);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_BRKINT, BRKINT);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IGNPAR, IGNPAR);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_INPCK, INPCK);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_ISTRIP, ISTRIP);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_INLCR, INLCR);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IGNCR, IGNCR);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_ICRNL, ICRNL);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IXON, IXON);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IXANY, IXANY);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IXOFF, IXOFF);
	bts->c_iflag |= cvtto_bsd_mask(lt->c_iflag, LINUX_IMAXBEL, IMAXBEL);

	bts->c_oflag = 0;
	bts->c_oflag |= cvtto_bsd_mask(lt->c_oflag, LINUX_OPOST, OPOST);
	bts->c_oflag |= cvtto_bsd_mask(lt->c_oflag, LINUX_ONLCR, ONLCR);
	bts->c_oflag |= cvtto_bsd_mask(lt->c_oflag, LINUX_XTABS, OXTABS);

	/*
	 * This could have been:
	 * bts->c_cflag = (lt->c_flag & LINUX_CSIZE) << 4
	 * But who knows, those values might perhaps change one day.
	 */
	switch (lt->c_cflag & LINUX_CSIZE) {
	case LINUX_CS5:
		bts->c_cflag = CS5;
		break;
	case LINUX_CS6:
		bts->c_cflag = CS6;
		break;
	case LINUX_CS7:
		bts->c_cflag = CS7;
		break;
	case LINUX_CS8:
		bts->c_cflag = CS8;
		break;
	}
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_CSTOPB, CSTOPB);
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_CREAD, CREAD);
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_PARENB, PARENB);
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_PARODD, PARODD);
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_HUPCL, HUPCL);
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_CLOCAL, CLOCAL);
	bts->c_cflag |= cvtto_bsd_mask(lt->c_cflag, LINUX_CRTSCTS, CRTSCTS);

	bts->c_lflag = 0;
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ISIG, ISIG);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ICANON, ICANON);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHO, ECHO);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHOE, ECHOE);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHOK, ECHOK);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHONL, ECHONL);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_NOFLSH, NOFLSH);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_TOSTOP, TOSTOP);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHOCTL, ECHOCTL);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHOPRT, ECHOPRT);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_ECHOKE, ECHOKE);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_FLUSHO, FLUSHO);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_PENDIN, PENDIN);
	bts->c_lflag |= cvtto_bsd_mask(lt->c_lflag, LINUX_IEXTEN, IEXTEN);

	index = lt->c_cflag & LINUX_CBAUD;
	if (index & LINUX_CBAUDEX)
		index = (index & ~LINUX_CBAUDEX) + LINUX_NSPEEDS - 1;
	bts->c_ispeed = bts->c_ospeed = linux_speeds[index];

	bts->c_cc[VINTR] = lt->c_cc[LINUX_OLD_VINTR];
	bts->c_cc[VQUIT] = lt->c_cc[LINUX_OLD_VQUIT];
	bts->c_cc[VERASE] = lt->c_cc[LINUX_OLD_VERASE];
	bts->c_cc[VKILL] = lt->c_cc[LINUX_OLD_VKILL];
	bts->c_cc[VEOF] = lt->c_cc[LINUX_OLD_VEOF];
	bts->c_cc[VTIME] = lt->c_cc[LINUX_OLD_VTIME];
	bts->c_cc[VMIN] = lt->c_cc[LINUX_OLD_VMIN];
}

static void
#ifdef COMPAT_LINUX32
bsd_termios_to_linux32_termio(struct termios *bts, struct linux32_termio *lt)
#else
bsd_termios_to_linux_termio(struct termios *bts, struct linux_termio *lt)
#endif
{
	int i, mask;

	lt->c_iflag = 0;
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IGNBRK, LINUX_IGNBRK);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, BRKINT, LINUX_BRKINT);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IGNPAR, LINUX_IGNPAR);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, INPCK, LINUX_INPCK);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, ISTRIP, LINUX_ISTRIP);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, INLCR, LINUX_INLCR);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IGNCR, LINUX_IGNCR);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, ICRNL, LINUX_ICRNL);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IXON, LINUX_IXON);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IXANY, LINUX_IXANY);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IXOFF, LINUX_IXOFF);
	lt->c_iflag |= cvtto_linux_mask(bts->c_iflag, IMAXBEL, LINUX_IMAXBEL);

	lt->c_oflag = 0;
	lt->c_oflag |= cvtto_linux_mask(bts->c_oflag, OPOST, LINUX_OPOST);
	lt->c_oflag |= cvtto_linux_mask(bts->c_oflag, ONLCR, LINUX_ONLCR);
	lt->c_oflag |= cvtto_linux_mask(bts->c_oflag, OXTABS, LINUX_XTABS);

	switch (bts->c_cflag & CSIZE) {
	case CS5:
		lt->c_cflag = LINUX_CS5;
		break;
	case CS6:
		lt->c_cflag = LINUX_CS6;
		break;
	case CS7:
		lt->c_cflag = LINUX_CS7;
		break;
	case CS8:
		lt->c_cflag = LINUX_CS8;
		break;
	}
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, CSTOPB, LINUX_CSTOPB);
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, CREAD, LINUX_CREAD);
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, PARENB, LINUX_PARENB);
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, PARODD, LINUX_PARODD);
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, HUPCL, LINUX_HUPCL);
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, CLOCAL, LINUX_CLOCAL);
	lt->c_cflag |= cvtto_linux_mask(bts->c_cflag, CRTSCTS, LINUX_CRTSCTS);

	lt->c_lflag = 0;
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ISIG, LINUX_ISIG);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ICANON, LINUX_ICANON);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHO, LINUX_ECHO);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOE, LINUX_ECHOE);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOK, LINUX_ECHOK);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHONL, LINUX_ECHONL);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, NOFLSH, LINUX_NOFLSH);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, TOSTOP, LINUX_TOSTOP);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOCTL, LINUX_ECHOCTL);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOPRT, LINUX_ECHOPRT);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOKE, LINUX_ECHOKE);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, FLUSHO, LINUX_FLUSHO);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, PENDIN, LINUX_PENDIN);
	lt->c_lflag |= cvtto_linux_mask(bts->c_lflag, IEXTEN, LINUX_IEXTEN);

	mask = LINUX_B9600;	/* XXX default value should this be 0? */
	for (i = 0; i < sizeof (linux_speeds) / sizeof (linux_speed_t); i++) {
		if (bts->c_ospeed == linux_speeds[i]) {
			mask = linux_spmasks[i];
			break;
		}
	}
	lt->c_cflag |= mask;

	lt->c_cc[LINUX_VINTR] = bts->c_cc[VINTR];
	lt->c_cc[LINUX_VQUIT] = bts->c_cc[VQUIT];
	lt->c_cc[LINUX_VERASE] = bts->c_cc[VERASE];
	lt->c_cc[LINUX_VKILL] = bts->c_cc[VKILL];
	lt->c_cc[LINUX_VEOF] = bts->c_cc[VEOF];
	lt->c_cc[LINUX_VTIME] = bts->c_cc[VTIME];
	lt->c_cc[LINUX_VMIN] = bts->c_cc[VMIN];
	lt->c_cc[LINUX_VSWTC] = 0;

	/* XXX should be fixed someday */
	lt->c_line = 0;
}

static void
#ifdef COMPAT_LINUX32
linux32_termios_to_bsd_termios(struct linux32_termios *lts, struct termios *bts)
#else
linux_termios_to_bsd_termios(struct linux_termios *lts, struct termios *bts)
#endif
{
	int index;

	bts->c_iflag = 0;
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IGNBRK, IGNBRK);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_BRKINT, BRKINT);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IGNPAR, IGNPAR);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_INPCK, INPCK);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_ISTRIP, ISTRIP);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_INLCR, INLCR);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IGNCR, IGNCR);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_ICRNL, ICRNL);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IXON, IXON);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IXANY, IXANY);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IXOFF, IXOFF);
	bts->c_iflag |= cvtto_bsd_mask(lts->c_iflag, LINUX_IMAXBEL, IMAXBEL);

	bts->c_oflag = 0;
	bts->c_oflag |= cvtto_bsd_mask(lts->c_oflag, LINUX_OPOST, OPOST);
	bts->c_oflag |= cvtto_bsd_mask(lts->c_oflag, LINUX_ONLCR, ONLCR);
	bts->c_oflag |= cvtto_bsd_mask(lts->c_oflag, LINUX_XTABS, OXTABS);

	bts->c_cflag = 0;
	switch (lts->c_cflag & LINUX_CSIZE) {
	case LINUX_CS5:
		bts->c_cflag = CS5;
		break;
	case LINUX_CS6:
		bts->c_cflag = CS6;
		break;
	case LINUX_CS7:
		bts->c_cflag = CS7;
		break;
	case LINUX_CS8:
		bts->c_cflag = CS8;
		break;
	}
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_CSTOPB, CSTOPB);
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_CREAD, CREAD);
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_PARENB, PARENB);
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_PARODD, PARODD);
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_HUPCL, HUPCL);
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_CLOCAL, CLOCAL);
	bts->c_cflag |= cvtto_bsd_mask(lts->c_cflag, LINUX_CRTSCTS, CRTSCTS);

	bts->c_lflag = 0;
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ISIG, ISIG);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ICANON, ICANON);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHO, ECHO);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHOE, ECHOE);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHOK, ECHOK);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHONL, ECHONL);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_NOFLSH, NOFLSH);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_TOSTOP, TOSTOP);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHOCTL, ECHOCTL);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHOPRT, ECHOPRT);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_ECHOKE, ECHOKE);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_FLUSHO, FLUSHO);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_PENDIN, PENDIN);
	bts->c_lflag |= cvtto_bsd_mask(lts->c_lflag, LINUX_IEXTEN, IEXTEN);

	index = lts->c_cflag & LINUX_CBAUD;
	if (index & LINUX_CBAUDEX)
		index = (index & ~LINUX_CBAUDEX) + LINUX_NSPEEDS - 1;
	bts->c_ispeed = bts->c_ospeed = linux_speeds[index];
	/*
	 * A null c_ospeed causes NetBSD to hangup the terminal.
	 * Linux does not do this, and it sets c_ospeed to zero
	 * sometimes. If it is null, we store -1 in the kernel
	 */
	if (bts->c_ospeed == 0)
		bts->c_ospeed = -1;

	bts->c_cc[VINTR] = lts->c_cc[LINUX_VINTR];
	bts->c_cc[VQUIT] = lts->c_cc[LINUX_VQUIT];
	bts->c_cc[VERASE] = lts->c_cc[LINUX_VERASE];
	bts->c_cc[VKILL] = lts->c_cc[LINUX_VKILL];
	bts->c_cc[VEOF] = lts->c_cc[LINUX_VEOF];
	bts->c_cc[VTIME] = lts->c_cc[LINUX_VTIME];
	bts->c_cc[VMIN] = lts->c_cc[LINUX_VMIN];
	bts->c_cc[VEOL] = lts->c_cc[LINUX_VEOL];
	bts->c_cc[VEOL2] = lts->c_cc[LINUX_VEOL2];
	bts->c_cc[VWERASE] = lts->c_cc[LINUX_VWERASE];
	bts->c_cc[VSUSP] = lts->c_cc[LINUX_VSUSP];
	bts->c_cc[VSTART] = lts->c_cc[LINUX_VSTART];
	bts->c_cc[VSTOP] = lts->c_cc[LINUX_VSTOP];
	bts->c_cc[VLNEXT] = lts->c_cc[LINUX_VLNEXT];
	bts->c_cc[VDISCARD] = lts->c_cc[LINUX_VDISCARD];
	bts->c_cc[VREPRINT] = lts->c_cc[LINUX_VREPRINT];
}

static void
#ifdef COMPAT_LINUX32
bsd_termios_to_linux32_termios(struct termios *bts, struct linux32_termios *lts)
#else
bsd_termios_to_linux_termios(struct termios *bts, struct linux_termios *lts)
#endif
{
	int i, mask;

	lts->c_iflag = 0;
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IGNBRK, LINUX_IGNBRK);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, BRKINT, LINUX_BRKINT);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IGNPAR, LINUX_IGNPAR);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, INPCK, LINUX_INPCK);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, ISTRIP, LINUX_ISTRIP);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, INLCR, LINUX_INLCR);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IGNCR, LINUX_IGNCR);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, ICRNL, LINUX_ICRNL);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IXON, LINUX_IXON);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IXANY, LINUX_IXANY);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IXOFF, LINUX_IXOFF);
	lts->c_iflag |= cvtto_linux_mask(bts->c_iflag, IMAXBEL, LINUX_IMAXBEL);

	lts->c_oflag = 0;
	lts->c_oflag |= cvtto_linux_mask(bts->c_oflag, OPOST, LINUX_OPOST);
	lts->c_oflag |= cvtto_linux_mask(bts->c_oflag, ONLCR, LINUX_ONLCR);
	lts->c_oflag |= cvtto_linux_mask(bts->c_oflag, OXTABS, LINUX_XTABS);

	switch (bts->c_cflag & CSIZE) {
	case CS5:
		lts->c_cflag = LINUX_CS5;
		break;
	case CS6:
		lts->c_cflag = LINUX_CS6;
		break;
	case CS7:
		lts->c_cflag = LINUX_CS7;
		break;
	case CS8:
		lts->c_cflag = LINUX_CS8;
		break;
	}
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CS5, LINUX_CS5);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CS6, LINUX_CS6);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CS7, LINUX_CS7);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CS8, LINUX_CS8);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CSTOPB, LINUX_CSTOPB);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CREAD, LINUX_CREAD);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, PARENB, LINUX_PARENB);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, PARODD, LINUX_PARODD);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, HUPCL, LINUX_HUPCL);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CLOCAL, LINUX_CLOCAL);
	lts->c_cflag |= cvtto_linux_mask(bts->c_cflag, CRTSCTS, LINUX_CRTSCTS);

	lts->c_lflag = 0;
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ISIG, LINUX_ISIG);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ICANON, LINUX_ICANON);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHO, LINUX_ECHO);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOE, LINUX_ECHOE);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOK, LINUX_ECHOK);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHONL, LINUX_ECHONL);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, NOFLSH, LINUX_NOFLSH);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, TOSTOP, LINUX_TOSTOP);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOCTL, LINUX_ECHOCTL);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOPRT, LINUX_ECHOPRT);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, ECHOKE, LINUX_ECHOKE);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, FLUSHO, LINUX_FLUSHO);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, PENDIN, LINUX_PENDIN);
	lts->c_lflag |= cvtto_linux_mask(bts->c_lflag, IEXTEN, LINUX_IEXTEN);

	mask = LINUX_B9600;	/* XXX default value */
	for (i = 0; i < sizeof (linux_speeds) / sizeof (linux_speed_t); i++) {
		if (bts->c_ospeed == linux_speeds[i]) {
			mask = linux_spmasks[i];
			break;
		}
	}
	/*
	 * A null c_ospeed causes NetBSD to hangup the terminal.
	 * Linux does not do this, and it sets c_ospeed to zero
	 * sometimes. If it is null, we store -1 in the kernel
	 */
	if (bts->c_ospeed == -1)
		bts->c_ospeed = 0;
	lts->c_cflag |= mask;

	lts->c_cc[LINUX_VINTR] = bts->c_cc[VINTR];
	lts->c_cc[LINUX_VQUIT] = bts->c_cc[VQUIT];
	lts->c_cc[LINUX_VERASE] = bts->c_cc[VERASE];
	lts->c_cc[LINUX_VKILL] = bts->c_cc[VKILL];
	lts->c_cc[LINUX_VEOF] = bts->c_cc[VEOF];
	lts->c_cc[LINUX_VTIME] = bts->c_cc[VTIME];
	lts->c_cc[LINUX_VMIN] = bts->c_cc[VMIN];
	lts->c_cc[LINUX_VEOL] = bts->c_cc[VEOL];
	lts->c_cc[LINUX_VEOL2] = bts->c_cc[VEOL2];
	lts->c_cc[LINUX_VWERASE] = bts->c_cc[VWERASE];
	lts->c_cc[LINUX_VSUSP] = bts->c_cc[VSUSP];
	lts->c_cc[LINUX_VSTART] = bts->c_cc[VSTART];
	lts->c_cc[LINUX_VSTOP] = bts->c_cc[VSTOP];
	lts->c_cc[LINUX_VLNEXT] = bts->c_cc[VLNEXT];
	lts->c_cc[LINUX_VDISCARD] = bts->c_cc[VDISCARD];
	lts->c_cc[LINUX_VREPRINT] = bts->c_cc[VREPRINT];
	lts->c_cc[LINUX_VSWTC] = 0;

	/* XXX should be fixed someday */
	lts->c_line = 0;
}
#endif /* !_LINUX_TERMIOS_H */
