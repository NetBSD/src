/*	$NetBSD: adi,ad5592r.h,v 1.1.1.2.2.2 2017/12/03 11:38:37 jdolecek Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _DT_BINDINGS_ADI_AD5592R_H
#define _DT_BINDINGS_ADI_AD5592R_H

#define CH_MODE_UNUSED			0
#define CH_MODE_ADC			1
#define CH_MODE_DAC			2
#define CH_MODE_DAC_AND_ADC		3
#define CH_MODE_GPIO			8

#define CH_OFFSTATE_PULLDOWN		0
#define CH_OFFSTATE_OUT_LOW		1
#define CH_OFFSTATE_OUT_HIGH		2
#define CH_OFFSTATE_OUT_TRISTATE	3

#endif /* _DT_BINDINGS_ADI_AD5592R_H */
