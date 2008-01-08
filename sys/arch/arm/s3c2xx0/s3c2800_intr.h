/*	$NetBSD: s3c2800_intr.h,v 1.4.66.1 2008/01/08 22:09:31 bouyer Exp $	*/

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _S3C2800_INTR_H_
#define	_S3C2800_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(s3c2800_irq_handler)

#ifndef _LOCORE

#include <arm/s3c2xx0/s3c2800reg.h>

#ifdef __HAVE_FAST_SOFTINTS
/*
 * on S3C2800's interrupt controller, interrupt source bits 9, and 29..31 are
 * reserved. we map software interrupts to those unused bits.
 */   
#define	SI_TO_IRQBIT(si)  ((si)==SI_SOFTSERIAL? (1<<9) : (1U<<(ICU_LEN+(si))))

#define	get_pending_softint()	(softint_pending & intr_mask)
#define	update_softintr_mask()	/* empty */
#endif

#define	s3c2xx0_update_hw_mask()	\
	(*s3c2xx0_intr_mask_reg = intr_mask & global_intr_mask)

#include <arm/s3c2xx0/s3c2xx0_intr.h>

#endif /* ! _LOCORE */

#endif /* _S3C2800_INTR_H_ */
