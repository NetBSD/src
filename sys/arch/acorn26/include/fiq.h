/*	$NetBSD: fiq.h,v 1.1.8.2 2002/03/24 15:46:58 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _ACORN26_FIQ_H_
#define _ACORN26_FIQ_H_
#include <arch/acorn26/iobus/iocreg.h>

/*
 * These definitions specify how the devices are wired to the IOC
 * interrupt lines.
 */
/* All systems */
#define FIQ_EFIQ	IOC_FIQ_FL	/* Econet interrupt request */
#define FIQ_PFIQ	IOC_FIQ_IL0	/* Podule FIQ request */
/* Archimedes systems */
#define FIQ_FFDQ	IOC_FIQ_FH0	/* Floppy disc data request */
#define FIQ_FFIQ	IOC_FIQ_FH1	/* Floppy disc interrupt request */
/* IOEB systems */
#define FIQ_FDDRQ	IOC_FIQ_FH0	/* Floppy disc data request */
#define FIQ_SINTR	IOC_FIQ_C4	/* Serial line interrupt */

extern void (*fiq_downgrade_handler)(void);

#endif
