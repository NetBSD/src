/*	$NetBSD: kgdb_stub.c,v 1.1 1996/10/09 07:45:11 matthias Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	@(#)kgdb_stub.c	8.4 (Berkeley) 1/12/94
 */

/*
 * "Stub" to allow remote cpu to debug over a serial line using gdb.
 */

#include <sys/param.h>
#include <vm/vm.h>
#include "kgdb.h"

#ifndef KGDBDEV
#define KGDBDEV -1
#endif
#ifndef KGDBRATE
#define KGDBRATE 19200
#endif

int kgdb_dev = KGDBDEV;		/* remote debugging device (-1 if none) */
int kgdb_rate = KGDBRATE;	/* remote debugging baud rate */
int kgdb_active = 0;		/* remote debugging active if != 0 */
int kgdb_debug_init = 0;	/* != 0 waits for remote at system init */
int kgdb_debug_panic = 0;	/* != 0 waits for remote on panic */

static void kgdb_copy __P((void *, void *, int));
static void kgdb_zero __P((void *, int));
static void kgdb_send __P((u_char *));
static int kgdb_recv __P((u_char *, int));
static int digit2i __P((u_char));
static u_char i2digit __P((int));
static void mem2hex __P((void *, void *, int));
static u_char *hex2mem __P((void *, u_char *, int));
static vm_offset_t hex2i __P((u_char **));

static int (*kgdb_getc) __P((void *));
static void (*kgdb_putc) __P((void *, int));
static void *kgdb_ioarg;

static u_char buffer[KGDB_BUFLEN];
static kgdb_reg_t gdb_regs[KGDB_NUMREGS];

#define ROUND_PAGE(x) ((((vm_offset_t)(x)) + PGOFSET) & ~PGOFSET)
#define GETC()	((*kgdb_getc)(kgdb_ioarg))
#define PUTC(c)	((*kgdb_putc)(kgdb_ioarg, c))
#define PUTESC(c) do { \
	if (c == FRAME_END) { \
		PUTC(FRAME_ESCAPE); \
		c = TRANS_FRAME_END; \
	} else if (c == FRAME_ESCAPE) { \
		PUTC(FRAME_ESCAPE); \
		c = TRANS_FRAME_ESCAPE; \
	} else if (c == FRAME_START) { \
		PUTC(FRAME_ESCAPE); \
		c = TRANS_FRAME_START; \
	} \
	PUTC(c); \
} while (0)

/*
 * This little routine exists simply so that bcopy() can be debugged.
 */
static void
kgdb_copy(vsrc, vdst, len)
	void *vsrc, *vdst;
	int len;
{
	char *src = vsrc;
	char *dst = vdst;

	while (--len >= 0)
		*dst++ = *src++;
}

/* ditto for bzero */
static void
kgdb_zero(vptr, len)
	void *vptr;
	int len;
{
	char *ptr = vptr;

	while (--len >= 0)
		*ptr++ = (char) 0;
}

/*
 * Convert a hex digit into an integer.
 * This returns -1 if the argument passed is no
 * valid hex digit.
 */
static int
digit2i(c)
	u_char c;
{
	if (c >= '0' && c <= '9')
		return(c - '0');
	else if (c >= 'a' && c <= 'f')
		return(c - 'a' + 10);	
	else if (c >= 'A' && c <= 'F')

		return(c - 'A' + 10);	
	else
		return(-1);
}

/*
 * Convert the low 4 bits of an integer into
 * an hex digit.
 */
static u_char
i2digit(n)
	int n;
{
	return("0123456789abcdef"[n & 0x0f]);
}

/*
 * Convert a byte array into an hex string.
 */
static void
mem2hex(vdst, vsrc, len)
	void *vdst, *vsrc;
	int len;
{
	u_char *dst = vdst;
	u_char *src = vsrc;

	while (len--) {
		*dst++ = i2digit(*src >> 4);
		*dst++ = i2digit(*src++);
	}
	*dst = '\0';
}

/*
 * Convert an hex string into a byte array.
 * This returns a pointer to the character following
 * the last valid hex digit. If the string ends in
 * the middle of a byte, NULL is returned.
 */
static u_char *
hex2mem(vdst, src, maxlen)
	void *vdst;
	u_char *src;
	int maxlen;
{
	u_char *dst = vdst;
	int msb, lsb;

	while (*src && maxlen--) {
		msb = digit2i(*src++);
		if (msb < 0)
			return(src - 1);
		lsb = digit2i(*src++);
		if (lsb < 0)
			return(NULL);
		*dst++ = (msb << 4) | lsb;
	}
	return(src);
}

/*
 * Convert an hex string into an integer.
 * This returns a pointer to the character following
 * the last valid hex digit.
 */ 
static vm_offset_t
hex2i(srcp)
	u_char **srcp;
{
	char *src = *srcp;
	vm_offset_t r = 0;
	int nibble;

	while ((nibble = digit2i(*src)) >= 0) {
		r *= 16;
		r += nibble;
		src++;
	}
	*srcp = src;
	return(r);
}

/*
 * Send a packet.
 */
static void
kgdb_send(bp)
	u_char *bp;
{
	u_char *p;
	u_char csum, c;

	do {
		p = bp;
		PUTC(KGDB_START);
		for (csum = 0; c = *p; p++) {
			PUTC(c);
			csum += c;
		}
		PUTC(KGDB_END);
		PUTC(i2digit(csum >> 4));
		PUTC(i2digit(csum));
	} while ((c = GETC() & 0x7f) == KGDB_BADP);
}

/*
 * Receive a packet.
 */
static int
kgdb_recv(bp, maxlen)
	u_char *bp;
	int maxlen;
{
	u_char *p;
	int c, csum;
	int len;

	do {
		p = bp;
		csum = len = 0;
		while ((c = GETC()) != KGDB_START)
			;

		while ((c = GETC()) != KGDB_END && len < maxlen) {
			c &= 0x7f;
			csum += c;
			*p++ = c;
			len++;
		}
		csum &= 0xff;
		*p = '\0';

		if (len >= maxlen) {
			PUTC(KGDB_BADP);
			continue;
		}

		csum -= digit2i(GETC()) * 16;
		csum -= digit2i(GETC());

		if (csum == 0) {
			PUTC(KGDB_GOODP);
			/* Sequence present? */
			if (bp[2] == ':') {
				PUTC(bp[0]);
				PUTC(bp[1]);
				len -= 3;
				kgdb_copy(bp + 3, bp, len);
			}
			break;
		}
		PUTC(KGDB_BADP);
	} while (1);
	return(len);
}

/*
 * This is called by the approprite tty driver.
 * In our case, by dev/scn.c:scn_kgdb_init()
 */
void
kgdb_attach(getfn, putfn, ioarg)
	int (*getfn) __P((void *));
	void (*putfn) __P((void *, int));
	void *ioarg;
{
	kgdb_getc = getfn;
	kgdb_putc = putfn;
	kgdb_ioarg = ioarg;
}

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(verbose)
	int verbose;
{

	if (kgdb_dev < 0 || kgdb_getc == NULL)
		return;

	if (verbose)
		printf("kgdb waiting...");
	Debugger();
	if (verbose)
		printf("connected.\n");
}

/*
 * Decide what to do on panic.
 * (This is called by panic, like Debugger())
 */
void
kgdb_panic()
{
	if (kgdb_dev >= 0 && kgdb_getc != NULL && kgdb_debug_panic)
		kgdb_connect(kgdb_active == 0);
}

/*
 * This function does all command procesing for interfacing to
 * a remote gdb.
 */
int
kgdb_trap(type, regs)
	int type;
	db_regs_t *regs;
{
	size_t len;
	vm_offset_t addr;
	u_char *p;

	if (kgdb_dev < 0 || kgdb_getc == NULL) {
		/* not debugging */
		return (0);
	}

	if (kgdb_active == 0) {
		if (!IS_BREAKPOINT_TRAP(type, 0)) {
			/* No debugger active -- let trap handle this. */
			return (0);
		}
#ifdef FIXUP_PC_AFTER_BREAK
#error "FIXUP_PC_AFTER_BREAK" has to be modified to take an argument
		FIXUP_PC_AFTER_BREAK(regs):
#endif
		PC_REGS(regs) += BKPT_SIZE;
		kgdb_active = 1;
	} else {
		/* Tell remote host that an exception has occured. */
		sprintf(buffer, "S%02x", kgdb_signal(type));
		kgdb_send(buffer);
	}
	/* Stick frame regs into our reg cache. */
	kgdb_getregs(regs, gdb_regs);

	for (;;) {
		kgdb_recv(buffer, sizeof(buffer));
		switch (buffer[0]) {

		default:
			/* Unknown command. */
			kgdb_send("");
			continue;

		case KGDB_SIGNAL:
			/*
			 * if this command came from a running gdb,
			 * answer it -- the other guy has no way of
			 * knowing if we're in or out of this loop
			 * when he issues a "remote-signal".
			 */
			sprintf(buffer, "S%02x", kgdb_signal(type));
			kgdb_send(buffer);
			continue;

		case KGDB_REG_R:
			mem2hex(buffer, (u_char *)gdb_regs, sizeof(gdb_regs));
			kgdb_send(buffer);
			continue;

		case KGDB_REG_W:
			p = hex2mem(gdb_regs, buffer + 1, sizeof(gdb_regs));
			if (p == NULL || *p != '\0')
				kgdb_send("E01");
			else {
				kgdb_setregs(regs, gdb_regs);
				kgdb_send("OK");
			}
			continue;

		case KGDB_MEM_R:
			p = buffer + 1;
			addr = hex2i(&p);
			if (*p++ != ',') {
				kgdb_send("E02");
				continue;
			}
			len = hex2i(&p);
			if (*p != '\0') {
				kgdb_send("E03");
				continue;
			}
			if (len > sizeof(buffer) / 2) {
				kgdb_send("E04");
				continue;
			}
			if (kgdb_acc(addr, len) == 0) {
				kgdb_send("E05");
				continue;
			}
			db_read_bytes(addr, (size_t)len,
					(char *)buffer + sizeof(buffer) / 2);
			mem2hex(buffer, buffer + sizeof(buffer) / 2, len);
			kgdb_send(buffer);
			continue;

		case KGDB_MEM_W:
			p = buffer + 1;
			addr = hex2i(&p);
			if (*p++ != ',') {
				kgdb_send("E06");
				continue;
			}
			len = hex2i(&p);
			if (*p++ != ':') {
				kgdb_send("E07");
				continue;
			}
			if (len > (sizeof(buffer) - (p - buffer))) {
				kgdb_send("E08");
				continue;
			}
			p = hex2mem(buffer, p, sizeof(buffer));
			if (p == NULL) {
				kgdb_send("E09");
				continue;
			}
			if (kgdb_acc(addr, len) == 0) {
				kgdb_send("E0A");
				continue;
			}
			db_write_bytes(addr, (size_t)len, (char *)buffer);
			kgdb_send("OK");
			continue;

		case KGDB_KILL:
			kgdb_active = 0;
			printf("kgdb detached\n");
			db_clear_single_step(regs);
			return(1);

		case KGDB_CONT:
			if (buffer[1]) {
				p = buffer + 1;
				addr = hex2i(&p);
				if (*p) {
					kgdb_send("E0B");
					continue;
				}
				PC_REGS(regs) = addr;
			}
			db_clear_single_step(regs);
			return(1);

		case KGDB_STEP:
			if (buffer[1]) {
				p = buffer + 1;
				addr = hex2i(&p);
				if (*p) {
					kgdb_send("E0B");
					continue;
				}
				PC_REGS(regs) = addr;
			}
			db_set_single_step(regs);
			return(1);
		}
	}
	return (1);
}
