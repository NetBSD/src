/*	$NetBSD: pcio.c,v 1.11.2.2 2002/02/19 20:18:37 thorpej Exp $	 */

/*
 * Copyright (c) 1996, 1997
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

/*
 * console I/O
 * needs lowlevel routines from conio.S and comio.S
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "libi386.h"
#include "bootinfo.h"

extern void conputc __P((int));
extern int congetc __P((void));
extern int coniskey __P((void));

struct btinfo_console btinfo_console;

#ifdef SUPPORT_SERIAL
static int iodev;

#ifdef DIRECT_SERIAL
#include "comio_direct.h"

#define cominit cominit_d
#define computc computc_d
#define comgetc comgetc_d
/* comstatus() is different */

#define SERIAL_ARG btinfo_console.addr
#else
extern void cominit __P((int));
extern int computc __P((int, int));
extern int comgetc __P((int));
extern int comstatus __P((int));

#define SERIAL_ARG (iodev - CONSDEV_COM0)
#endif /* DIRECT_SERIAL */

static int getcomaddr __P((int));
#endif /* SUPPORT_SERIAL */

#define POLL_FREQ 10

#ifdef SUPPORT_SERIAL
static int
getcomaddr(idx)
	int idx;
{
	short addr;
	/* read in BIOS data area */
	pvbcopy((void *)(0x400 + 2 * idx), &addr, 2);
	return(addr);
}
#endif

void
initio(dev)
	int             dev;
{
#ifdef SUPPORT_SERIAL
	int i;

	switch (dev) {
	    case CONSDEV_AUTO:
		for(i = 0; i < 3; i++) {
			iodev = CONSDEV_COM0 + i;
			btinfo_console.addr = getcomaddr(i);
			if(!btinfo_console.addr) break;
			conputc('0' + i); /* to tell user what happens */
			cominit(SERIAL_ARG);
#ifdef DIRECT_SERIAL
			/* check for:
			 *  1. successful output
			 *  2. optionally, keypress within 1s
			 */
			if (	computc(':', SERIAL_ARG) &&
				computc('-', SERIAL_ARG) &&
				computc('(', SERIAL_ARG)
#ifdef COMCONS_KEYPRESS
			   && awaitkey(7, 0)
#endif
			   )
				goto ok;
#else /* ! DIRECT_SERIAL */
			/*
			 * serial console must have hardware handshake!
			 * check:
			 *  1. character output without error
			 *  2. status bits for modem ready set
			 *     (status seems only useful after character output)
			 *  3. optionally, keypress within 1s
			 */
			if (!(computc('@', SERIAL_ARG) & 0x80)
			    && (comstatus(SERIAL_ARG) & 0x00b0)
#ifdef COMCONS_KEYPRESS
			    && awaitkey(7, 0)
#endif
			    )
				goto ok;
#endif /* DIRECT_SERIAL */
		}
		iodev = CONSDEV_PC;
ok:
		break;
	    case CONSDEV_COM0:
	    case CONSDEV_COM1:
	    case CONSDEV_COM2:
	    case CONSDEV_COM3:
		iodev = dev;
		btinfo_console.addr = getcomaddr(iodev - CONSDEV_COM0);
		if(!btinfo_console.addr) goto nocom;
		cominit(SERIAL_ARG);
		break;
	    case CONSDEV_COM0KBD:
	    case CONSDEV_COM1KBD:
	    case CONSDEV_COM2KBD:
	    case CONSDEV_COM3KBD:
		iodev = dev - 4;
		i = iodev - CONSDEV_COM0;
		btinfo_console.addr = getcomaddr(i);
		if(!btinfo_console.addr) goto nocom;
		conputc('0' + i); /* to tell user what happens */
		cominit(SERIAL_ARG);
#ifdef DIRECT_SERIAL
			/* check for:
			 *  1. successful output
			 *  2. optionally, keypress within 1s
			 */
			if (	computc(':', SERIAL_ARG) &&
				computc('-', SERIAL_ARG) &&
				computc('(', SERIAL_ARG)
#ifdef COMCONS_KEYPRESS
			   && awaitkey(7, 0)
#endif
			   )
				break;
#else /* ! DIRECT_SERIAL */
			/*
			 * serial console must have hardware handshake!
			 * check:
			 *  1. character output without error
			 *  2. status bits for modem ready set
			 *     (status seems only useful after character output)
			 *  3. optionally, keypress within 1s
			 */
			if (!(computc('@', SERIAL_ARG) & 0x80)
			    && (comstatus(SERIAL_ARG) & 0x00b0)
#ifdef COMCONS_KEYPRESS
			    && awaitkey(7, 0)
#endif
			    )
				break;
#endif /* DIRECT_SERIAL */
	    default:
nocom:
		iodev = CONSDEV_PC;
		break;
	}
	conputc('\015');
	conputc('\n');
	strncpy(btinfo_console.devname, iodev == CONSDEV_PC ? "pc" : "com", 16);
#if defined(DIRECT_SERIAL) && defined(CONSPEED)
	btinfo_console.speed = CONSPEED;
#else
	btinfo_console.speed = 9600;
#endif
#else /* !SUPPORT_SERIAL */
	strncpy(btinfo_console.devname, "pc", 16);
#endif /* SUPPORT_SERIAL */
}

static inline void internal_putchar __P((int));

static inline void
internal_putchar(c)
	int c;
{
#ifdef SUPPORT_SERIAL
	switch (iodev) {
	    case CONSDEV_PC:
#endif
		conputc(c);
#ifdef SUPPORT_SERIAL
		break;
	    case CONSDEV_COM0:
	    case CONSDEV_COM1:
	    case CONSDEV_COM2:
	    case CONSDEV_COM3:
		computc(c, SERIAL_ARG);
		break;
	}
#endif
}

void
putchar(c)
	int c;
{
	if (c == '\n')
		internal_putchar('\r');
	internal_putchar(c);
}

int
getchar()
{
#ifdef SUPPORT_SERIAL
	int c;
	switch (iodev) {
	    default: /* to make gcc -Wall happy... */
	    case CONSDEV_PC:
#endif
		return (congetc());
#ifdef SUPPORT_SERIAL
	    case CONSDEV_COM0:
	    case CONSDEV_COM1:
	    case CONSDEV_COM2:
	    case CONSDEV_COM3:
#ifdef DIRECT_SERIAL
		c = comgetc(SERIAL_ARG);
#else
		do {
			c = comgetc(SERIAL_ARG);
		} while ((c >> 8) == 0xe0); /* catch timeout */
#ifdef COMDEBUG
		if (c & 0x8000) {
			printf("com input %x, status %x\n",
			       c, comstatus(SERIAL_ARG));
		}
#endif
		c &= 0xff;
#endif /* DIRECT_SERIAL */
		return (c);
	}
#endif /* SUPPORT_SERIAL */
}

int
iskey()
{
#ifdef SUPPORT_SERIAL
	switch (iodev) {
	    default: /* to make gcc -Wall happy... */
	    case CONSDEV_PC:
#endif
		return (coniskey());
#ifdef SUPPORT_SERIAL
	    case CONSDEV_COM0:
	    case CONSDEV_COM1:
	    case CONSDEV_COM2:
	    case CONSDEV_COM3:
#ifdef DIRECT_SERIAL
		return(!!comstatus_d(SERIAL_ARG));
#else
		return (!!(comstatus(SERIAL_ARG) & 0x0100));
#endif
	}
#endif /* SUPPORT_SERIAL */
}

char
awaitkey(timeout, tell)
	int timeout, tell;
{
	int i;
	char c = 0;

	i = timeout * POLL_FREQ;

	while (i) {
		if (tell && (i % POLL_FREQ) == 0) {
			char numbuf[20];
			int len, j;

			sprintf(numbuf, "%d ", i/POLL_FREQ);
			len = strlen(numbuf);
			for (j = 0; j < len; j++)
				numbuf[len + j] = '\b';
			numbuf[len + j] = '\0';
			printf(numbuf);
		}
		if (iskey()) {
			/* flush input buffer */
			while (iskey())
				c = getchar();
			goto out; /* XXX what happens if c == 0? */
		}
		delay(1000000 / POLL_FREQ);
		i--;
	}

out:
	if (tell)
		printf("0\n");

	return(c);
}
