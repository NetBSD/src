/* $NetBSD: spdmemreg.h,v 1.1.2.2 2007/08/20 22:42:51 ad Exp $ */

/*
 * Copyright (c) 2007 Paul Goyette
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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

/* possible values for the memory type */
#define	SPDMEM_MEMTYPE_FPM		0x01
#define	SPDMEM_MEMTYPE_EDO		0x02
#define	SPDMEM_MEMTYPE_PIPE_NIBBLE	0x03
#define	SPDMEM_MEMTYPE_SDRAM		0x04
#define	SPDMEM_MEMTYPE_ROM		0x05
#define	SPDMEM_MEMTYPE_DDRSGRAM		0x06
#define	SPDMEM_MEMTYPE_DDRSDRAM		0x07
#define	SPDMEM_MEMTYPE_DDR2SDRAM	0x08

/* possible values for the supply voltage */
#define	SPDMEM_VOLTAGE_TTL_5V		0x00
#define	SPDMEM_VOLTAGE_TTL_LV		0x01
#define	SPDMEM_VOLTAGE_HSTTL_1_5V	0x02
#define	SPDMEM_VOLTAGE_SSTL_3_3V	0x03
#define	SPDMEM_VOLTAGE_SSTL_2_5V	0x04
#define	SPDMEM_VOLTAGE_SSTL_1_8V	0x05

/* possible values for module configuration */
#define	SPDMEM_MODCONFIG_PARITY		0x01
#define	SPDMEM_MODCONFIG_ECC		0x02

/* for DDR2, module configuration is a bit-mask field */
#define	SPDMEM_MODCONFIG_HAS_DATA_PARITY	0x01
#define	SPDMEM_MODCONFIG_HAS_DATA_ECC		0x02
#define	SPDMEM_MODCONFIG_HAS_ADDR_CMD_PARITY	0x04

/* possible values for the refresh field */
#define	SPDMEM_REFRESH_STD		0x00
#define	SPDMEM_REFRESH_QUARTER		0x01
#define	SPDMEM_REFRESH_HALF		0x02
#define	SPDMEM_REFRESH_TWOX		0x03
#define	SPDMEM_REFRESH_FOURX		0x04
#define	SPDMEM_REFRESH_EIGHTX		0x05
#define	SPDMEM_REFRESH_SELFREFRESH	0x80

/* superset types */
#define	SPDMEM_SUPERSET_ESDRAM		0x01
#define	SPDMEM_SUPERSET_DDR_ESDRAM	0x02
#define	SPDMEM_SUPERSET_EDO_PEM		0x03
#define	SPDMEM_SUPERSET_SDRAM_PEM	0x04
