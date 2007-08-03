/*	$NetBSD: ns16550.h,v 1.2 2007/08/03 13:15:56 tsutsui Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
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
 *      This product includes software developed by Gary Thomas.
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
 */

/*
 * NS16550 Serial Port
 */

struct NS16550 {
		volatile uint8_t rbr;  /* 0 */
		volatile uint8_t ier;  /* 1 */
		volatile uint8_t fcr;  /* 2 */
		volatile uint8_t lcr;  /* 3 */
		volatile uint8_t mcr;  /* 4 */
		volatile uint8_t lsr;  /* 5 */
		volatile uint8_t msr;  /* 6 */
		volatile uint8_t scr;  /* 7 */
};

#define thr rbr
#define iir fcr
#define dll rbr
#define dlm ier

#define LSR_DR   0x01  /* Data ready */
#define LSR_OE   0x02  /* Overrun */
#define LSR_PE   0x04  /* Parity error */
#define LSR_FE   0x08  /* Framing error */
#define LSR_BI   0x10  /* Break */
#define LSR_THRE 0x20  /* Xmit holding register empty */
#define LSR_TEMT 0x40  /* Xmitter empty */
#define LSR_ERR  0x80  /* Error */

#define LCR_EERS 0xBF  /* Enable access to Enhanced Register Set */
#define LCR_DLAB 0x80  /* Divisor latch access enable */

#ifndef COMBASE
#define COMBASE	0x80000000
#endif

#ifndef COMPROBE
#define COMPROBE 0xa020001c
#endif

volatile struct NS16550 *NS16550_init(int, int);
void NS16550_putc(volatile struct NS16550 *, int);
int NS16550_getc(volatile struct NS16550 *);
int NS16550_scankbd(volatile struct NS16550 *);
int NS16550_test(volatile struct NS16550 *);
