/*	$NetBSD: phy-cadence.h,v 1.1.1.2 2026/01/18 05:21:50 skrll Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides constants for Cadence SERDES.
 */

#ifndef _DT_BINDINGS_CADENCE_SERDES_H
#define _DT_BINDINGS_CADENCE_SERDES_H

#define CDNS_SERDES_NO_SSC		0
#define CDNS_SERDES_EXTERNAL_SSC	1
#define CDNS_SERDES_INTERNAL_SSC	2

/* Torrent */
#define CDNS_TORRENT_REFCLK_DRIVER      0
#define CDNS_TORRENT_DERIVED_REFCLK	1
#define CDNS_TORRENT_RECEIVED_REFCLK	2

/* Sierra */
#define CDNS_SIERRA_PLL_CMNLC		0
#define CDNS_SIERRA_PLL_CMNLC1		1
#define CDNS_SIERRA_DERIVED_REFCLK	2

#endif /* _DT_BINDINGS_CADENCE_SERDES_H */
