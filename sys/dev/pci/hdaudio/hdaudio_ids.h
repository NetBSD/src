/* $NetBSD: hdaudio_ids.h,v 1.2 2010/09/13 22:37:02 jmcneill Exp $ */

/*
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Precedence Technologies Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _HDAUDIO_IDS_H
#define _HDAUDIO_IDS_H

#define HDA_VENDOR_ATI			0x1002
#define	HDA_VENDOR_NVIDIA		0x10de
#define	HDA_VENDOR_REALTEK		0x10ec
#define	HDA_VENDOR_VIA			0x1106
#define	HDA_VENDOR_SIGMATEL2		0x111d
#define	HDA_VENDOR_ANALOG_DEVICES	0x11d4
#define	HDA_VENDOR_CONEXANT		0x14f1
#define	HDA_VENDOR_CMEDIA		0x434d
#define	HDA_VENDOR_INTEL		0x8086
#define	HDA_VENDOR_SIGMATEL		0x8384

#define	HDA_PRODUCT_ANY			0xffff

void	hdaudio_id2name(uint16_t, uint16_t, char *, size_t);

#endif /* !_HDAUDIO_IDS_H */
