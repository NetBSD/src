/* $NetBSD: lptio.h,v 1.4 2004/01/28 17:35:58 jdolecek Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gary Thorpe.
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

#ifndef __DEV_PPBUS_LPTIO_H_
#define __DEV_PPBUS_LPTIO_H_

/* Definitions for get status command */
enum lpt_mode_t {
	standard = 1,
	nibble = 2,
	ps2 = 3,
	fast = 4,
	ecp = 5,
	epp = 6
};

typedef struct {
	u_int16_t mode_status;
	u_int8_t dma_status;
	u_int8_t ieee_status;
} LPT_INFO_T;

/* LPT ioctl commands */
#define LPTIO_ENABLE_DMA	_IO('L', 0)
#define LPTIO_DISABLE_DMA	_IO('L', 1)
#define LPTIO_MODE_STD		_IO('L', 2)
#define LPTIO_MODE_FAST		_IO('L', 3)
#define LPTIO_MODE_PS2		_IO('L', 4)
#define LPTIO_MODE_ECP		_IO('L', 5)
#define LPTIO_MODE_EPP		_IO('L', 6)
#define LPTIO_MODE_NIBBLE	_IO('L', 7)
#define LPTIO_ENABLE_IEEE	_IO('L', 8)
#define LPTIO_DISABLE_IEEE	_IO('L', 9)
#define LPTIO_GET_STATUS	_IOR('L', 10, LPT_INFO_T)

#endif /* __DEV_PPBUS_LPTIO_H_ */
