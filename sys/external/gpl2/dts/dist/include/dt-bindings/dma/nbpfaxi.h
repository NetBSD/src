/*	$NetBSD: nbpfaxi.h,v 1.1.1.2 2020/01/03 14:33:03 skrll Exp $	*/

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013-2014 Renesas Electronics Europe Ltd.
 * Author: Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 */

#ifndef DT_BINDINGS_NBPFAXI_H
#define DT_BINDINGS_NBPFAXI_H

/**
 * Use "#dma-cells = <2>;" with the second integer defining slave DMA flags:
 */
#define NBPF_SLAVE_RQ_HIGH	1
#define NBPF_SLAVE_RQ_LOW	2
#define NBPF_SLAVE_RQ_LEVEL	4

#endif
