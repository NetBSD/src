/*	$NetBSD: nslm7xvar.h,v 1.2 2000/03/07 18:39:14 groo Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Squier.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_ISA_NSLM7XVAR_H_
#define _DEV_ISA_NSLM7XVAR_H_

/* ctl registers */

#define LMC_ADDR	0x05
#define LMC_DATA	0x06

/* data registers */

#define LMD_SENSORBASE	0x20	/* Sensors occupy 0x20 -- 0x2a */

#define LMD_CONFIG	0x40	/* Configuration */ 
#define LMD_ISR1	0x41	/* Interrupt Status 1 */
#define LMD_ISR2	0x42	/* Interrupt Status 2 */
#define LMD_SMI1	0x43	/* SMI Mask 1 */
#define LMD_SMI2	0x44	/* SMI Mask 2 */
#define LMD_NMI1	0x45	/* NMI Mask 1 */
#define LMD_NMI2	0x46	/* NMI Mask 2 */
#define LMD_VIDFAN	0x47	/* VID/Fan Divisor */
#define LMD_SBUSADDR	0x48	/* Serial Bus Address */
#define LMD_CHIPID	0x49	/* Chip Reset/ID */

/* misc constants */

#define LM_NUM_SENSORS	11
#define LM_ID_LM78	0x00
#define LM_ID_LM78J	0x40
#define LM_ID_LM79	0xC0
#define LM_ID_MASK	0xFE

struct lm_softc {
	struct	device sc_dev;

	int	lm_iobase;
	bus_space_tag_t lm_iot;
	bus_space_handle_t lm_ioh;

	int	sc_flags;
	struct	timeval lastread; /* only allow reads every 1.5 seconds */
	struct	envsys_tre_data sensors[LM_NUM_SENSORS];
	struct	envsys_basic_info info[LM_NUM_SENSORS];
};

void lm_attach __P((struct lm_softc *));
int lm_probe __P((bus_space_tag_t, bus_space_handle_t));

#endif /* _DEV_ISA_NSLM7XVAR_H_ */
