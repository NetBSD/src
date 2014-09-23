/*	$NetBSD: wm8731reg.h,v 1.1 2014/09/23 14:49:46 nonaka Exp $	*/
/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by TOYOKURA Atsushi.
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

#ifndef	_DEV_IC_WM8731REG_H_
#define	_DEV_IC_WM8731REG_H_

/*
 * Wolfson Microelectronics' WM8731/WM8731L I2C/I2S audio codec:
 * - I2C register definitions.  Used in the Sharp Zaurus SL-C7x0/860.
 */

#define WM8731_LIN_REG 0x00	/* Left Line In */
#define   WM8731_LRINBOTH     0x100
#define   WM8731_LINMUTE      0x080
#define   WM8731_LINVOL_MASK  0x01F
#define   WM8731_GET_LINVOL(x) ((x) & WM8731_LINVOL_MASK)
#define   WM8731_SET_LINVOL(x) ((x) & WM8731_LINVOL_MASK)

#define WM8731_RIN_REG 0x01	/* Right Line In */
#define   WM8731_RLINBOTH     0x100
#define   WM8731_RINMUTE      0x080
#define   WM8731_RINVOL_MASK  0x01F
#define   WM8731_GET_RINVOL(x) ((x) & WM8731_RINVOL_MASK)
#define   WM8731_SET_RINVOL(x) ((x) & WM8731_RINVOL_MASK)

#define WM8731_LHP_REG 0x02	/* Left Headphone Out */
#define   WM8731_LRHPBOTH     0x100
#define   WM8731_LZCEN        0x080
#define   WM8731_LHPVOL_MASK  0x07F
#define   WM8731_GET_LHPVOL(x) ((x) & WM8731_LHPVOL_MASK)
#define   WM8731_SET_LHPVOL(x) ((x) & WM8731_LHPVOL_MASK)

#define WM8731_RHP_REG 0x03	/* Right Headphone Out */
#define   WM8731_RLHPBOTH     0x100
#define   WM8731_RZCEN        0x080
#define   WM8731_RHPVOL_MASK  0x07F
#define   WM8731_GET_RHPVOL(x) ((x) & WM8731_RHPVOL_MASK)
#define   WM8731_SET_RHPVOL(x) ((x) & WM8731_RHPVOL_MASK)

#define WM8731_AAP_REG 0x04	/* Analogue Audio Path Control */
#define   WM8731_SIDEATT_MASK 0x0C0
#define   WM8731_SIDETONE     0x020
#define   WM8731_DACSEL       0x010
#define   WM8731_BYPASS       0x008
#define   WM8731_INSEL        0x004
#define   WM8731_MUTEMIC      0x002
#define   WM8731_MICBOOST     0x001
#define   WM8731_GET_SIDEATT(x) (((x) & WM8731_SIDEATT_MASK) >> 6)
#define   WM8731_SET_SIDEATT(x) (((x) << 6) & WM8731_SIDEATT_MASK)

#define WM8731_DAP_REG 0x05	/* Digital Audio Path Control */
#define   WM8731_HPOR         0x010
#define   WM8731_DACMU        0x008
#define   WM8731_DEEMP_MASK   0x006
#define   WM8731_ADCHP        0x001
#define   WM8731_GET_DEEMP(x) (((x) & WM8731_DEEMP_MASK) >> 1)
#define   WM8731_SET_DEEMP(x) (((x) << 1) & WM8731_DEEMP_MASK)

#define WM8731_PD_REG  0x06	/* Power Down Control */
#define   WM8731_POWEROFF     0x080
#define   WM8731_CLKOUTPD     0x040
#define   WM8731_OSCPD        0x020
#define   WM8731_OUTPD        0x010
#define   WM8731_DACPD        0x008
#define   WM8731_ADCPD        0x004
#define   WM8731_MICPD        0x002
#define   WM8731_LINEINPD     0x001

#define WM8731_DAI_REG 0x07	/* Digital Audio Interface Format */
#define   WM8731_BCLKINV      0x080
#define   WM8731_MS           0x040
#define   WM8731_LRSWAP       0x020
#define   WM8731_LRP          0x010
#define   WM8731_IWL_MASK     0x00C
#define   WM8731_FORMAT_MASK  0x003
#define   WM8731_GET_IWL(x) (((x) & WM8731_IWL_MASK) >> 2)
#define   WM8731_SET_IWL(x) (((x) << 2) & WM8731_IWL_MASK)
#define   WM8731_GET_FORMAT(x) ((x) & WM8731_FORMAT_MASK)
#define   WM8731_SET_FORMAT(x) ((x) & WM8731_FORMAT_MASK)

#define WM8731_SP_REG 0x08	/* Sampling Control */
#define   WM8731_CLKODIV2     0x080
#define   WM8731_CLKIDIV2     0x040
#define   WM8731_SR_MASK      0x03C
#define   WM8731_BOSR         0x002
#define   WM8731_USB          0x001
#define   WM8731_GET_SR(x) (((x) & WM8731_SR_MASK) >> 2)
#define   WM8731_SET_SR(x) (((x) << 2) & WM8731_SR_MASK)

#define WM8731_AC_REG 0x09	/* Active Control */
#define   WM8731_ACTIVE       0x001

#define WM8731_RESET_REG 0x0F	/* Reset Register */
#define   WM8731_RESET_MASK   0x1FF
#define   WM8731_GET_RESET(x) ((x) & WM8731_RESET_MASK)
#define   WM8731_SET_RESET(x) ((x) & WM8731_RESET_MASK)

#endif	/* _DEV_IC_WM8731REG_H_ */
