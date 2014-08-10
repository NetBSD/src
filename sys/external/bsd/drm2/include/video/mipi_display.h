/*	$NetBSD: mipi_display.h,v 1.1.2.2 2014/08/10 06:55:40 tls Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _VIDEO_MIPI_DISPLAY_H_
#define _VIDEO_MIPI_DISPLAY_H_

#define	MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM	0x03
#define	MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM	0x13
#define	MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM	0x23

#define	MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM	0x04
#define	MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM	0x14
#define	MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM	0x24

#define	MIPI_DSI_DCS_SHORT_WRITE		0x05
#define	MIPI_DSI_DCS_SHORT_WRITE_PARAM		0x15

#define	MIPI_DSI_DCS_READ			0x06

#define	MIPI_DSI_NULL_PACKET			0x09
#define	MIPI_DSI_BLANKING_PACKET		0x19
#define	MIPI_DSI_GENERIC_LONG_WRITE		0x29
#define	MIPI_DSI_DCS_LONG_WRITE			0x39

#endif  /* _VIDEO_MIPI_DISPLAY_H_ */
