/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
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
 */

/*
 * ttcompat.c -- convert sgtty flags to termios
 *	originally from /sys/kern/tty_compat.c
 */

#include <sys/param.h>
#include <sys/types.h>

#include <unistd.h>
#include <sys/ioctl_compat.h>
#include <termios.h>
#include <syslog.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "extern.h"

void
sttyclearflags(tp, flags)
struct termios *tp;
long flags;
{
	register long iflag = tp->c_iflag;
	register long oflag = tp->c_oflag;
	register long lflag = tp->c_lflag;
	register long cflag = tp->c_cflag;

	if (flags & TANDEM)
		iflag &= ~IXOFF;
	if (flags & ECHO)
		lflag &= ~ECHO;
	if (flags & CRMOD) {
		iflag &= ~ICRNL;
		oflag &= ~ONLCR;
	}
	if (flags & XTABS)
		oflag &= ~OXTABS;


	if (flags & RAW) {
		iflag |= BRKINT|IXON|IMAXBEL;
		lflag |= ISIG|IEXTEN;
		if (flags & CBREAK)
			lflag &= ~ICANON;
		else
			lflag |= ICANON;
	}

	switch (flags & ANYP) {
	case EVENP:
		iflag |= INPCK;
		cflag |= PARODD;
		break;
	case ODDP:
		iflag |= INPCK;
		cflag &= ~PARODD;
		break;
	}

	if (flags & (RAW|LITOUT|PASS8)) {
		cflag &= ~CSIZE;
		cflag |= CS7|PARENB;
		iflag |= ISTRIP;
		oflag |= OPOST;
	}

	tp->c_iflag = iflag;
	tp->c_oflag = oflag;
	tp->c_lflag = lflag;
	tp->c_cflag = cflag;
}

void
sttysetflags(tp, flags)
struct termios *tp;
long flags;
{
	register long iflag = tp->c_iflag;
	register long oflag = tp->c_oflag;
	register long lflag = tp->c_lflag;
	register long cflag = tp->c_cflag;

	if (flags & TANDEM)
		iflag |= IXOFF;
	if (flags & ECHO)
		lflag |= ECHO;
	if (flags & CRMOD) {
		iflag |= ICRNL;
		oflag |= ONLCR;
	}
	if (flags & XTABS)
		oflag |= OXTABS;

	if (flags & RAW) {
		iflag &= IXOFF;
		lflag &= ~(ISIG|ICANON|IEXTEN);
	}

	switch (flags & ANYP) {
	case EVENP:
		iflag |= INPCK;
		cflag &= ~PARODD;
		break;
	case ODDP:
		iflag |= INPCK;
		cflag |= PARODD;
		break;
	}

	if (flags & (RAW|LITOUT|PASS8)) {
		cflag &= ~(CSIZE|PARENB);
		cflag |= CS8;
		if ((flags & (RAW|PASS8)) == 0)
			iflag |= ISTRIP;
		else
			iflag &= ~ISTRIP;
		if ((flags & (RAW|LITOUT)) == 0)
			oflag |= OPOST;
		else
			oflag &= ~OPOST;
	}

	tp->c_iflag = iflag;
	tp->c_oflag = oflag;
	tp->c_lflag = lflag;
	tp->c_cflag = cflag;
}

void
sttyclearlflags(tp, flags)
struct termios *tp;
long flags;
{
	register long iflag = tp->c_iflag;
	register long oflag = tp->c_oflag;
	register long lflag = tp->c_lflag;
	register long cflag = tp->c_cflag;

	/* Nothing we can do with CRTBS. */
	if (flags & PRTERA)
		lflag &= ~ECHOPRT;
	if (flags & CRTERA)
		lflag &= ~ECHOE;
	/* Nothing we can do with TILDE. */
	if (flags & MDMBUF)
		cflag &= ~MDMBUF;
	if (flags & NOHANG)
		cflag |= HUPCL;
	if (flags & CRTKIL)
		lflag &= ~ECHOKE;
	if (flags & CTLECH)
		lflag &= ~ECHOCTL;
	if ((flags & DECCTQ) == 0)
		iflag &= ~IXANY;
	lflag &= ~(flags&(TOSTOP|FLUSHO|PENDIN|NOFLSH));

	if (flags & (RAW|LITOUT|PASS8)) {
		cflag &= ~CSIZE;
		cflag |= CS7|PARENB;
		iflag |= ISTRIP;
		oflag |= OPOST;
	}

	tp->c_iflag = iflag;
	tp->c_oflag = oflag;
	tp->c_lflag = lflag;
	tp->c_cflag = cflag;
}

void
sttysetlflags(tp, flags)
struct termios *tp;
long flags;
{
	register long iflag = tp->c_iflag;
	register long oflag = tp->c_oflag;
	register long lflag = tp->c_lflag;
	register long cflag = tp->c_cflag;

	/* Nothing we can do with CRTBS. */
	if (flags & PRTERA)
		lflag |= ECHOPRT;
	if (flags & CRTERA)
		lflag |= ECHOE;
	/* Nothing we can do with TILDE. */
	if (flags & MDMBUF)
		cflag |= MDMBUF;
	if (flags & NOHANG)
		cflag &= ~HUPCL;
	if (flags & CRTKIL)
		lflag |= ECHOKE;
	if (flags & CTLECH)
		lflag |= ECHOCTL;
	if ((flags & DECCTQ) == 0)
		iflag |= IXANY;
	lflag &= ~(TOSTOP|FLUSHO|PENDIN|NOFLSH);
	lflag |= flags&(TOSTOP|FLUSHO|PENDIN|NOFLSH);

	if (flags & (RAW|LITOUT|PASS8)) {
		cflag &= ~(CSIZE|PARENB);
		cflag |= CS8;
		if ((flags & (RAW|PASS8)) == 0)
			iflag |= ISTRIP;
		else
			iflag &= ~ISTRIP;
		if ((flags & (RAW|LITOUT)) == 0)
			oflag |= OPOST;
		else
			oflag &= ~OPOST;
	}

	tp->c_iflag = iflag;
	tp->c_oflag = oflag;
	tp->c_lflag = lflag;
	tp->c_cflag = cflag;
}
