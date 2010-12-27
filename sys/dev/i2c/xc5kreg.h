/* $NetBSD: xc5kreg.h,v 1.1 2010/12/27 15:42:11 jmcneill Exp $ */

/*-
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _XC5KREG_H
#define _XC5KREG_H

/* write */
#define	XC5K_REG_INIT		0x00
#define	XC5K_REG_VIDEO_MODE	0x01
#define	 XC5K_VIDEO_MODE_BTSC		0x8028
#define	XC5K_REG_AUDIO_MODE	0x02
#define	 XC5K_AUDIO_MODE_BTSC		0x0400
#define	XC5K_REG_RF_FREQ	0x03
#define	XC5K_REG_D_CODE		0x04
#define	XC5K_REG_IF_OUT		0x05
#define	XC5K_REG_SEEK_MODE	0x07
#define	XC5K_REG_POWER_DOWN	0x0a
#define	XC5K_REG_SIGNAL_SOURCE	0x0d
#define	 XC5K_SIGNAL_SOURCE_AIR		0
#define	 XC5K_SIGNAL_SOURCE_CABLE	1
#define	XC5K_REG_SMOOTHED_CVBS	0x0e
#define	XC5K_REG_EXT_FREQ	0x0f
#define	XC5K_REG_FINER_FREQ	0x10
#define	XC5K_REG_DDI_MODE	0x11

/* read */
#define	XC5K_REG_ADC_ENV	0x00
#define	XC5K_REG_QUALITY	0x01
#define	XC5K_REG_FRAME_LINES	0x02
#define	XC5K_REG_HSYNC_FREQ	0x03
#define	XC5K_REG_LOCK		0x04
#define	XC5K_REG_FREQ_ERR	0x05
#define	XC5K_REG_SNR		0x06
#define	XC5K_REG_VERSION	0x07
#define	XC5K_REG_PRODUCT_ID	0x08
#define  XC5K_PRODUCT_ID_NOFW		0x2000
#define	 XC5K_PRODUCT_ID		0x1388
#define	XC5K_REG_BUSY		0x09
#define	XC5K_REG_BUILD		0x0d

#endif /* !_XC5KREG_H */
