/*	$Id: at91intr.h,v 1.3.22.1 2014/08/20 00:02:45 tls Exp $	*/
/*	$NetBSD: at91intr.h,v 1.3.22.1 2014/08/20 00:02:45 tls Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy.
 * All rights reserved.
 *
 * Based on ep93xx_intr.h, 
 * Copyright (c) 2004 Jesse Off
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
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AT91INTR_H_
#define _AT91INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(at91aic_intr_dispatch)

#ifndef _LOCORE

int	_splraise(int);
int	_spllower(int);
void	splx(int);

#endif /* ! _LOCORE */

#endif /* _AT91INTR_H_ */
