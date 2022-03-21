/* $NetBSD: hdaudio_pci.h,v 1.2 2022/03/21 09:12:09 jmcneill Exp $ */

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

#ifndef _HDAUDIO_PCI_H
#define _HDAUDIO_PCI_H

/* NVIDIA specific registers */
#define	HDAUDIO_NV_REG_SNOOP		0x4c
#define	  HDAUDIO_NV_SNOOP_MASK			0x00ff0000
#define	  HDAUDIO_NV_SNOOP_ENABLE		0x000f0000

/* Intel ICH specific registers */
#define	HDAUDIO_INTEL_REG_ICH_TCSEL	0x44
#define	 HDAUDIO_INTEL_ICH_TCSEL_MASK		__BITS(2,0)
#define	 HDAUDIO_INTEL_ICH_TCSEL_TC0		0

/* Intel 100 Series Chipset Family Platform Controller Hub (PCH) */
#define	HDAUDIO_INTEL_REG_PCH_CGCTL	0x48
#define	  HDAUDIO_INTEL_PCH_CGCTL_MISCBDCGE	__BIT(6)
#define	HDAUDIO_INTEL_REG_PCH_DEVC	0x78
#define	  HDAUDIO_INTEL_PCH_DEVC_NSNPEN		__BIT(11)

#endif /* !_HDAUDIO_PCI_H */
