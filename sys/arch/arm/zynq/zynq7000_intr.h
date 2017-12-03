/*	$NetBSD: zynq7000_intr.h,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef _ARM_ZYNQ_ZYNQ7000_INTR_H_
#define _ARM_ZYNQ_ZYNQ7000_INTR_H_

#define	PIC_MAXSOURCES			128
#define	PIC_MAXMAXSOURCES		128

/*
 * The ZYNQ7000 uses a generic interrupt controller so pull that stuff.
 */
#include <arm/cortex/gic_intr.h>
#include <arm/cortex/a9tmr_intr.h>	/* A9 Timer PPIs */


#define	IRQ_CPU0	32
#define	IRQ_CPU1	33
#define	IRQ_L2CC	34
#define	IRQ_OCM		35
#define	IRQ__RSVD36	36
#define	IRQ_PMU0	37
#define	IRQ_PMU1	38
#define	IRQ_XADC	39
#define	IRQ_DVI		40
#define	IRQ_SWDT	41
#define	IRQ_TTC0	42
#define	IRQ__RSVD44	44
#define	IRQ_DMAC_ABORT	45
#define	IRQ_DMAC0	46
#define	IRQ_DMAC1	47
#define	IRQ_DMAC2	48
#define	IRQ_DMAC3	49
#define	IRQ_SMC		50
#define	IRQ_QSPI	51
#define	IRQ_GPIO	52
#define	IRQ_USB0	53
#define	IRQ_ETH0	54
#define	IRQ_ETH0_WU	55
#define	IRQ_SDIO0	56
#define	IRQ_I2C0	57
#define	IRQ_SPI0	58
#define	IRQ_UART0	59
#define	IRQ_CAN0	60
#define	IRQ_FPGA0	60
#define	IRQ_FPGA1	61
#define	IRQ_FPGA2	62
#define	IRQ_FPGA3	64
#define	IRQ_FPGA4	65
#define	IRQ_FPGA5	66
#define	IRQ_FPGA6	67
#define	IRQ_FPGA7	68
#define	IRQ_TTC1	69
#define	IRQ_DMAC4	72
#define	IRQ_DMAC5	73
#define	IRQ_DMAC6	74
#define	IRQ_DMAC7	75
#define	IRQ_USB1	76
#define	IRQ_ETH1	77
#define	IRQ_ETH1_WU	78
#define	IRQ_SDIO1	79
#define	IRQ_I2C1	80
#define	IRQ_SPI1	81
#define	IRQ_UART1	82
#define	IRQ_CAN1	83
#define	IRQ_FPGA8	84
#define	IRQ_FPGA9	85
#define	IRQ_FPGA10	86
#define	IRQ_FPGA11	87
#define	IRQ_FPGA12	88
#define	IRQ_FPGA13	89
#define	IRQ_FPGA14	90
#define	IRQ_FPGA15	91
#define	IRQ_PARITY	92
#define	IRQ__RSVD93	93
#define	IRQ__RSVD94	94
#define	IRQ__RSVD95	95

#endif /* _ARM_ZYNQ_ZYNQ7000_INTR_H_ */
