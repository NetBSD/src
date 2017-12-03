/*	$NetBSD: mvebu-icu.h,v 1.1.1.2.2.2 2017/12/03 11:38:37 jdolecek Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides constants for the MVEBU ICU driver.
 */

#ifndef _DT_BINDINGS_INTERRUPT_CONTROLLER_MVEBU_ICU_H
#define _DT_BINDINGS_INTERRUPT_CONTROLLER_MVEBU_ICU_H

/* interrupt specifier cell 0 */

#define ICU_GRP_NSR		0x0
#define ICU_GRP_SR		0x1
#define ICU_GRP_SEI		0x4
#define ICU_GRP_REI		0x5

#endif
