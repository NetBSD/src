/*      $NetBSD: vfpreg.h,v 1.1 2008/03/15 10:16:43 rearnsha Exp $ */

/*
 * Copyright (c) 2008 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VFPREG_H
#define _VFPREG_H

/* FPSID regsiter */

#define VFP_FPSID_IMP_MSK	0xff000000	/* Implementer */
#define VFP_FPSID_IMP_ARM	0x41000000	/* Implementer: ARM */
#define VFP_FPSID_SW		0x00800000	/* VFP implemented in SW */
#define VFP_FPSID_FMT_MSK	0x00600000	/* FLDMX/FSTMX Format */
#define VFP_FPSID_FMT_1		0x00000000	/* Standard format 1 */
#define VFP_FPSID_FMT_2		0x00200000	/* Standard format 2 */
#define VFP_FPSID_FMT_WEIRD	0x00600000	/* Non-standard format */
#define VFP_FPSID_SP		0x00100000	/* Only single precision */
#define VFP_FPSID_ARCH_MSK	0x000f0000	/* Architecture */
#define VFP_FPSID_ARCH_V1	0x00000000	/* Arch VFPv1 */
#define VFP_FPSID_ARCH_V2	0x00010000	/* Arch VFPv2 */
#define VFP_FPSID_PART_MSK	0x0000ff00	/* Part number */
#define VFP_FPSID_PART_VFP10	0x00001000	/* VFP10 */
#define VFP_FPSID_VAR_MSK	0x000000f0	/* Variant */
#define VFP_FPSID_VAR_ARM10	0x000000a0	/* Variant ARM10 */
#define VFP_FPSID_REV_MSK	0x0000000f	/* Revision */

#define VFP_FPEXC_EX		0x80000000	/* Exception status bit */
#define VFP_FPEXC_EN		0x40000000	/* Enable bit */

#define VFP_FPSCR_DN		0x02000000	/* Default NaN mode */
#define VFP_FPSCR_FZ		0x01000000	/* Flush-to-zero mode */


#define FPU_VFP10_ARM10E	0x410001a0	/* Really a VFPv2 part */
#define FPU_VFP11_ARM11		0x410120b0

#endif /* _VFPREG_H */
