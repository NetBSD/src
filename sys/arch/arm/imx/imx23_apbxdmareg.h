/* $Id: imx23_apbxdmareg.h,v 1.2 2013/03/03 10:33:56 jkunz Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#ifndef _ARM_IMX_IMX23_APBXDMAREG_H_
#define _ARM_IMX_IMX23_APBXDMAREG_H_

#include <sys/cdefs.h>

#define HW_APBXDMA_BASE 		0x80024000
#define HW_APBXDMA_SIZE 		0x2000 /* 8 kB */

/*
 * APBX DMA Channel 0 Current Command Address Register.
 */
#define HW_APBX_CH0_CURCMDAR		0x100

#define HW_APBX_CH0_CURCMDAR_CMD_ADDR	__BITS(31, 0)

/*
 * APBX DMA Channel 0 Next Command Address Register.
 */
#define HW_APBX_CH0_NXTCMDAR		0x110

#define HW_APBX_CH0_NXTCMDAR_CMD_ADDR	__BITS(31, 0)

/*
 * APBX DMA Channel 0 Semaphore Register.
 */
#define HW_APBX_CH0_SEMA		0x140

#define HW_APBX_CH0_SEMA_RSVD2		__BITS(31, 24)
#define HW_APBX_CH0_SEMA_PHORE		__BITS(23, 16)
#define HW_APBX_CH0_SEMA_RSVD1		__BITS(15, 8)
#define HW_APBX_CH0_SEMA_INCREMENT_SEMA	__BITS(7, 0)

#endif /* !_ARM_IMX_IMX23_APBXDMAREG_H_ */
