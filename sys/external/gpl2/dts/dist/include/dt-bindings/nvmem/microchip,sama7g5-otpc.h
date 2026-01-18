/*	$NetBSD: microchip,sama7g5-otpc.h,v 1.1.1.1 2026/01/18 05:21:50 skrll Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */

#ifndef _DT_BINDINGS_NVMEM_MICROCHIP_OTPC_H
#define _DT_BINDINGS_NVMEM_MICROCHIP_OTPC_H

/*
 * Need to have it as a multiple of 4 as NVMEM memory is registered with
 * stride = 4.
 */
#define OTP_PKT(id)			((id) * 4)

#endif
