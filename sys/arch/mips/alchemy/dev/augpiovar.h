/* $NetBSD: augpiovar.h,v 1.3.6.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef	_MIPS_ALCHEMY_DEV_AUGPIOVAR_H
#define	_MIPS_ALCHEMY_DEV_AUGPIOVAR_H

int augpio_read(void *, int);
void augpio_write(void *, int, int);
void augpio_ctl(void *, int, int);
int augpio_getctl(void *, int);

int augpio2_read(void *, int);
void augpio2_write(void *, int, int);
void augpio2_ctl(void *, int, int);
int augpio2_getctl(void *, int);

#define	AUGPIO_READ(pin)	augpio_read(NULL, (pin))
#define	AUGPIO_WRITE(pin,val)	augpio_write(NULL, (pin), (val))
#define	AUGPIO_CTL(pin,flags)	augpio_ctl(NULL, (pin), (flags))
#define	AUGPIO_GETCTL(pin)	augpio_getctl(NULL, (pin))

#define	AUGPIO2_READ(pin)	augpio2_read(NULL, (pin))
#define	AUGPIO2_WRITE(pin,val)	augpio2_write(NULL, (pin), (val))
#define	AUGPIO2_CTL(pin,flags)	augpio2_ctl(NULL, (pin), (flags))
#define	AUGPIO2_GETCTL(pin)	augpio2_getctl(NULL, (pin))

#endif	/* _MIPS_ALCHEMY_DEV_AUGPIOVAR_H */
