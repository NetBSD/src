/*	$NetBSD: sbpscsireg.h,v 1.1.2.2 2002/12/11 06:38:09 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Chacon
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

#ifndef _DEV_STD_SBPSCSIREG_H_
#define _DEV_STD_SBPSCSIREG_H_

#define	SBPSCSI_COMMAND_SET_SPEC_ID	0x00609E	/* NCITS OUI */
#define	SBPSCSI_COMMAND_SET		0x0104D8	/* SPC2 */

#define SBPSCSI_STATUS_GET_SMFT(x)	((x & 0xc0000000) >> 30)
#define SBPSCSI_STATUS_GET_STATUS(x)	((x & 0x3f000000) >> 24)
#define SBPSCSI_STATUS_GET_VFLAG(x)	((x & 0x08000000) >> 23)
#define SBPSCSI_STATUS_GET_MFLAG(x)	((x & 0x04000000) >> 22)
#define SBPSCSI_STATUS_GET_EFLAG(x)	((x & 0x02000000) >> 21)
#define SBPSCSI_STATUS_GET_IFLAG(x)	((x & 0x01000000) >> 20)
#define SBPSCSI_STATUS_GET_SENSEKEY(x)	((x & 0x000f0000) >> 16)
#define SBPSCSI_STATUS_GET_SENSECODE(x)	((x & 0x0000ff00) >> 8)
#define SBPSCSI_STATUS_GET_SENSEQUAL(x)	 (x & 0x000000ff)
#define SBPSCSI_STATUS_GET_FRU(x)	((x & 0xff000000) >> 24)
#define SBPSCSI_STATUS_GET_SENSE1(x)	((x & 0x00ff0000) >> 16)
#define SBPSCSI_STATUS_GET_SENSE2(x)	((x & 0x0000ff00) >> 8)
#define SBPSCSI_STATUS_GET_SENSE3(x)	 (x & 0x000000ff)

/* Can only handle 6, 10, and 12 byte cdb's with SBP2 */

#define SBPSCSI_SBP2_MAX_CDB	0xc

#endif	/* _DEV_STD_SBPSCSIREG_H_ */
