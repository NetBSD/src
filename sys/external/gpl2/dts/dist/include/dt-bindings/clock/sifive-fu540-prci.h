/*	$NetBSD: sifive-fu540-prci.h,v 1.1.1.1 2019/05/25 11:29:13 jmcneill Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2018-2019 SiFive, Inc.
 * Wesley Terpstra
 * Paul Walmsley
 */

#ifndef __DT_BINDINGS_CLOCK_SIFIVE_FU540_PRCI_H
#define __DT_BINDINGS_CLOCK_SIFIVE_FU540_PRCI_H

/* Clock indexes for use by Device Tree data and the PRCI driver */

#define PRCI_CLK_COREPLL	       0
#define PRCI_CLK_DDRPLL		       1
#define PRCI_CLK_GEMGXLPLL	       2
#define PRCI_CLK_TLCLK		       3

#endif
