/*	$NetBSD: mq200reg.h,v 1.1 2000/07/22 08:53:36 takemura Exp $	*/

/*-
 * Copyright (c) 2000 Takemura Shin
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
 */

#define MQ200_VENDOR_ID		0x4d51
#define MQ200_PRODUCT_ID	0x0200

#define MQ200_POWERSTATE_D0	0
#define MQ200_POWERSTATE_D1	1
#define MQ200_POWERSTATE_D2	2
#define MQ200_POWERSTATE_D3	3

#define MQ200_FRAMEBUFFER	0x000000	/* frame buffer base address */
#define MQ200_PM		0x600000	/* power management	*/
#define MQ200_CC		0x602000	/* CPU interface	*/
#define MQ200_MM		0x604000	/* memory interface unit */
#define MQ200_IN		0x608000	/* interrupt controller	*/
#define MQ200_GC		0x60a000	/* graphice controller	*/
#define MQ200_GE		0x60c000	/* graphics engine	*/
#define MQ200_FP		0x60e000	/* graphics engine	*/
#define MQ200_DC		0x614000	/* device configration	*/
#define MQ200_PC		0x616000	/* PCI configration	*/

/* PCI configuration space */
#define MQ200_PC00R		(MQ200_PC+0x00)	/* device/vendor ID	*/
#define MQ200_PC04R		(MQ200_PC+0x04)	/* command/status	*/
#define MQ200_PC08R		(MQ200_PC+0x04)	/* calss code/revision	*/

#define MQ200_PMR		(MQ200_PC+0x40)	/* power management	*/
#define MQ200_PMCSR		(MQ200_PC+0x44)	/* control/status	*/
