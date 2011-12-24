/*	$NetBSD: rmixl_intr.c,v 1.1.2.31 2011/12/24 01:57:54 matt Exp $	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Platform-specific interrupt support for the RMI XLP, XLR, XLS
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_intr.c,v 1.1.2.31 2011/12/24 01:57:54 matt Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#define	__INTR_PRIVATE

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/cpu.h>
#include <mips/cpuset.h>
#include <mips/locore.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_cpuvar.h>
#include <mips/rmi/rmixl_intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

//#define IOINTR_DEBUG 1
#ifdef IOINTR_DEBUG
int iointr_debug = IOINTR_DEBUG;
# define DPRINTF(x)	do { if (iointr_debug) printf x ; } while(0)
#else
# define DPRINTF(x)
#endif

#define RMIXL_PICREG_READ(off) \
	RMIXL_IOREG_READ(RMIXL_IO_DEV_PIC + (off))
#define RMIXL_PICREG_WRITE(off, val) \
	RMIXL_IOREG_WRITE(RMIXL_IO_DEV_PIC + (off), (val))

/* XXX this will need to deal with node */
#define RMIXLP_PICREG_READ(off) \
	rmixlp_read_8(RMIXL_PIC_PCITAG, (off))
#define	RMIXLP_PICREG_WRITE(off, val) \
	rmixlp_write_8(RMIXL_PIC_PCITAG, (off), (val));

/*
 * do not clear these when acking EIRR
 * (otherwise they get lost)
 */
#define RMIXL_EIRR_PRESERVE_MASK	\
		((MIPS_INT_MASK_5|MIPS_SOFT_INT_MASK) >> 8)

/*
 * IRT assignments depends on the RMI chip family
 * (XLS1xx vs. XLS2xx vs. XLS3xx vs. XLS6xx)
 * use the right display string table for the CPU that's running.
 */

/*
 * rmixl_irtnames_xlrxxx
 * - use for XLRxxx
 */
static const char * const rmixl_irtnames_xlrxxx[RMIXLR_NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio)",		/* 14 */
	"pic int 15 (hyper)",		/* 15 */
	"pic int 16 (pcix)",		/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (xgs0)",		/* 21 */
	"pic int 22 (xgs1)",		/* 22 */
	"pic int 23 (irq23)",		/* 23 */
	"pic int 24 (hyper_fatal)",	/* 24 */
	"pic int 25 (bridge_aerr)",	/* 25 */
	"pic int 26 (bridge_berr)",	/* 26 */
	"pic int 27 (bridge_tb)",	/* 27 */
	"pic int 28 (bridge_nmi)",	/* 28 */
	"pic int 29 (bridge_sram_derr)",/* 29 */
	"pic int 30 (gpio_fatal)",	/* 30 */
	"pic int 31 (reserved)",	/* 31 */
};

/*
 * rmixl_irtnames_xls2xx
 * - use for XLS2xx
 */
static const char * const rmixl_irtnames_xls2xx[RMIXLS_NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio_a)",		/* 14 */
	"pic int 15 (irq15)",		/* 15 */
	"pic int 16 (bridge_tb)",	/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (irq21)",		/* 21 */
	"pic int 22 (irq22)",		/* 22 */
	"pic int 23 (pcie_link2)",	/* 23 */
	"pic int 24 (pcie_link3)",	/* 24 */
	"pic int 25 (bridge_err)",	/* 25 */
	"pic int 26 (pcie_link0)",	/* 26 */
	"pic int 27 (pcie_link1)",	/* 27 */
	"pic int 28 (irq28)",		/* 28 */
	"pic int 29 (pcie_err)",	/* 29 */
	"pic int 30 (gpio_b)",		/* 30 */
	"pic int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xls1xx
 * - use for XLS1xx, XLS4xx-Lite
 */
static const char * const rmixl_irtnames_xls1xx[RMIXLS_NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio_a)",		/* 14 */
	"pic int 15 (irq15)",		/* 15 */
	"pic int 16 (bridge_tb)",	/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (irq21)",		/* 21 */
	"pic int 22 (irq22)",		/* 22 */
	"pic int 23 (irq23)",		/* 23 */
	"pic int 24 (irq24)",		/* 24 */
	"pic int 25 (bridge_err)",	/* 25 */
	"pic int 26 (pcie_link0)",	/* 26 */
	"pic int 27 (pcie_link1)",	/* 27 */
	"pic int 28 (irq28)",		/* 28 */
	"pic int 29 (pcie_err)",	/* 29 */
	"pic int 30 (gpio_b)",		/* 30 */
	"pic int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xls4xx:
 * - use for XLS4xx, XLS6xx
 */
static const char * const rmixl_irtnames_xls4xx[RMIXLS_NIRTS] = {
	"pic int 0 (watchdog)",		/*  0 */
	"pic int 1 (timer0)",		/*  1 */
	"pic int 2 (timer1)",		/*  2 */
	"pic int 3 (timer2)",		/*  3 */
	"pic int 4 (timer3)",		/*  4 */
	"pic int 5 (timer4)",		/*  5 */
	"pic int 6 (timer5)",		/*  6 */
	"pic int 7 (timer6)",		/*  7 */
	"pic int 8 (timer7)",		/*  8 */
	"pic int 9 (uart0)",		/*  9 */
	"pic int 10 (uart1)",		/* 10 */
	"pic int 11 (i2c0)",		/* 11 */
	"pic int 12 (i2c1)",		/* 12 */
	"pic int 13 (pcmcia)",		/* 13 */
	"pic int 14 (gpio_a)",		/* 14 */
	"pic int 15 (irq15)",		/* 15 */
	"pic int 16 (bridge_tb)",	/* 16 */
	"pic int 17 (gmac0)",		/* 17 */
	"pic int 18 (gmac1)",		/* 18 */
	"pic int 19 (gmac2)",		/* 19 */
	"pic int 20 (gmac3)",		/* 20 */
	"pic int 21 (irq21)",		/* 21 */
	"pic int 22 (irq22)",		/* 22 */
	"pic int 23 (irq23)",		/* 23 */
	"pic int 24 (irq24)",		/* 24 */
	"pic int 25 (bridge_err)",	/* 25 */
	"pic int 26 (pcie_link0)",	/* 26 */
	"pic int 27 (pcie_link1)",	/* 27 */
	"pic int 28 (pcie_link2)",	/* 28 */
	"pic int 29 (pcie_link3)",	/* 29 */
	"pic int 30 (gpio_b)",		/* 30 */
	"pic int 31 (usb)",		/* 31 */
};

/*
 * rmixl_irtnames_xlp:
 * - use for XLP
 */
static const char * const rmixl_irtnames_xlpxxx[RMIXLP_NIRTS] = {
	[  0] =	"pic int 0 (watchdog0)",
	[  1] = "pic int 1 (watchdog1)",
	[  2] = "pic int 2 (watchdogNMI0)",
	[  3] = "pic int 3 (watchdogNMI1)",
	[  4] = "pic int 4 (timer0)",
	[  5] = "pic int 5 (timer1)",
	[  6] = "pic int 6 (timer2)",
	[  7] = "pic int 7 (timer3)",
	[  8] = "pic int 8 (timer4)",
	[  9] = "pic int 9 (timer5)",
	[ 10] = "pic int 10 (timer6)",
	[ 11] = "pic int 11 (timer7)",
	[ 12] = "pic int 12 (fmn0)",
	[ 13] = "pic int 13 (fmn1)",
	[ 14] = "pic int 14 (fmn2)",
	[ 15] = "pic int 15 (fmn3)",
	[ 16] = "pic int 16 (fmn4)",
	[ 17] = "pic int 17 (fmn5)",
	[ 18] = "pic int 18 (fmn6)",
	[ 19] = "pic int 19 (fmn7)",
	[ 20] = "pic int 20 (fmn8)",
	[ 21] = "pic int 21 (fmn9)",
	[ 22] = "pic int 22 (fmn10)",
	[ 23] = "pic int 23 (fmn11)",
	[ 24] = "pic int 24 (fmn12)",
	[ 25] = "pic int 25 (fmn13)",
	[ 26] = "pic int 26 (fmn14)",
	[ 27] = "pic int 27 (fmn15)",
	[ 28] = "pic int 28 (fmn16)",
	[ 29] = "pic int 29 (fmn17)",
	[ 30] = "pic int 30 (fmn18)",
	[ 31] = "pic int 31 (fmn19)",
	[ 32] = "pic int 22 (fmn20)",
	[ 33] = "pic int 23 (fmn21)",
	[ 34] = "pic int 24 (fmn22)",
	[ 35] = "pic int 25 (fmn23)",
	[ 36] = "pic int 26 (fmn24)",
	[ 37] = "pic int 27 (fmn25)",
	[ 38] = "pic int 28 (fmn26)",
	[ 39] = "pic int 29 (fmn27)",
	[ 40] = "pic int 30 (fmn28)",
	[ 41] = "pic int 31 (fmn29)",
	[ 42] = "pic int 42 (fmn30)",
	[ 43] = "pic int 43 (fmn31)",
	[ 44] = "pic int 44 (message0)",
	[ 45] = "pic int 45 (message1)",
	[ 46] = "pic int 46 (pcie_msix0)",
	[ 47] = "pic int 47 (pcie_msix1)",
	[ 48] = "pic int 48 (pcie_msix2)",
	[ 49] = "pic int 49 (pcie_msix3)",
	[ 50] = "pic int 50 (pcie_msix4)",
	[ 51] = "pic int 51 (pcie_msix5)",
	[ 52] = "pic int 52 (pcie_msix6)",
	[ 53] = "pic int 53 (pcie_msix7)",
	[ 54] = "pic int 54 (pcie_msix8)",
	[ 55] = "pic int 55 (pcie_msix9)",
	[ 56] = "pic int 56 (pcie_msix10)",
	[ 57] = "pic int 57 (pcie_msix11)",
	[ 58] = "pic int 58 (pcie_msix12)",
	[ 59] = "pic int 59 (pcie_msix13)",
	[ 60] = "pic int 60 (pcie_msix14)",
	[ 61] = "pic int 61 (pcie_msix15)",
	[ 62] = "pic int 62 (pcie_msix16)",
	[ 63] = "pic int 63 (pcie_msix17)",
	[ 64] = "pic int 64 (pcie_msix18)",
	[ 65] = "pic int 65 (pcie_msix19)",
	[ 66] = "pic int 66 (pcie_msix20)",
	[ 67] = "pic int 67 (pcie_msix21)",
	[ 68] = "pic int 68 (pcie_msix22)",
	[ 69] = "pic int 69 (pcie_msix23)",
	[ 70] = "pic int 70 (pcie_msix24)",
	[ 71] = "pic int 71 (pcie_msix25)",
	[ 72] = "pic int 72 (pcie_msix26)",
	[ 73] = "pic int 73 (pcie_msix27)",
	[ 74] = "pic int 74 (pcie_msix28)",
	[ 75] = "pic int 75 (pcie_msix29)",
	[ 76] = "pic int 76 (pcie_msix30)",
	[ 77] = "pic int 77 (pcie_msix31)",
	[ 78] = "pic int 78 (pcie_link0)",
	[ 79] = "pic int 79 (pcie_link1)",
	[ 80] = "pic int 80 (pcie_link2)",
	[ 81] = "pic int 81 (pcie_link3)",
	[ 82] = "pic int 82 (na0)",
	[ 83] = "pic int 83 (na1)",
	[ 84] = "pic int 84 (na2)",
	[ 85] = "pic int 85 (na3)",
	[ 86] = "pic int 86 (na4)",
	[ 87] = "pic int 87 (na5)",
	[ 88] = "pic int 88 (na6)",
	[ 89] = "pic int 89 (na7)",
	[ 90] = "pic int 90 (na8)",
	[ 91] = "pic int 91 (na9)",
	[ 92] = "pic int 92 (na10)",
	[ 93] = "pic int 93 (na11)",
	[ 94] = "pic int 94 (na12)",
	[ 95] = "pic int 95 (na13)",
	[ 96] = "pic int 96 (na14)",
	[ 97] = "pic int 97 (na15)",
	[ 98] = "pic int 98 (na16)",
	[ 99] = "pic int 99 (na17)",
	[100] = "pic int 100 (na18)",
	[101] = "pic int 101 (na19)",
	[102] = "pic int 102 (na20)",
	[103] = "pic int 103 (na21)",
	[104] = "pic int 104 (na22)",
	[105] = "pic int 105 (na23)",
	[106] = "pic int 106 (na24)",
	[107] = "pic int 107 (na25)",
	[108] = "pic int 108 (na26)",
	[109] = "pic int 109 (na27)",
	[110] = "pic int 100 (na28)",
	[111] = "pic int 111 (na29)",
	[112] = "pic int 112 (na30)",
	[113] = "pic int 113 (na31)",
	[114] = "pic int 114 (poe)",
	[115] = "pic int 115 (ehci0)",
	[116] = "pic int 116 (ohci0)",
	[117] = "pic int 117 (ohci1)",
	[118] = "pic int 118 (ehci1)",
	[119] = "pic int 119 (ohci2)",
	[120] = "pic int 120 (ohci3)",
	[121] = "pic int 121 (data/raid)",
	[122] = "pic int 122 (security)",
	[123] = "pic int 123 (rsa/ecc)",
	[124] = "pic int 124 (compression0)",
	[125] = "pic int 125 (compression1)",
	[126] = "pic int 126 (compression2)",
	[127] = "pic int 127 (compression3)",
	[128] = "pic int 128 (irq128)",
	[129] = "pic int 129 (icici0)",
	[130] = "pic int 130 (icici1)",
	[131] = "pic int 131 (icici2)",
	[132] = "pic int 132 (kbp)",
	[133] = "pic int 133 (uart0)",
	[134] = "pic int 134 (uart1)",
	[135] = "pic int 135 (i2c0)",
	[136] = "pic int 136 (i2c1)",
	[137] = "pic int 137 (sysmgt0)",
	[138] = "pic int 138 (sysmgt1)",
	[139] = "pic int 139 (jtag)",
	[140] = "pic int 140 (pic)",
	[141] = "pic int 141 (irq141)",
	[142] = "pic int 142 (irq142)",
	[143] = "pic int 143 (irq143)",
	[144] = "pic int 144 (irq144)",
	[145] = "pic int 145 (irq145)",
	[146] = "pic int 146 (gpio0)",
	[147] = "pic int 147 (gpio1)",
	[148] = "pic int 148 (gpio2)",
	[149] = "pic int 149 (gpio3)",
	[150] = "pic int 150 (norflash)",
	[151] = "pic int 151 (nandflash)",
	[152] = "pic int 152 (spi)",
	[153] = "pic int 153 (mmc/sd)",
	[154] = "pic int 154 (mem-io-bridge)",
	[155] = "pic int 155 (l3)",
	[156] = "pic int 156 (gcu)",
	[157] = "pic int 157 (dram3_0)",
	[158] = "pic int 158 (dram3_1)",
	[159] = "pic int 159 (tracebuf)",
};
/*
 * rmixl_vecnames_common:
 * - use for unknown cpu implementation
 * - covers all vectors, not just IRT intrs
 */
static const char * const rmixl_vecnames_common[NINTRVECS] = {
	"vec 0 (sw0)",		/*  0 */
	"vec 1 (sw1)",		/*  1 */
	"vec 2 (hw2)",		/*  2 */
	"vec 3 (hw3)",		/*  3 */
	"vec 4 (hw4)",		/*  4 */
	"vec 5 (hw5)",		/*  5 */
	"vec 6 (hw6)",		/*  6 */
	"vec 7 (hw7)",		/*  7 */
	"vec 8",		/*  8 */
	"vec 9",		/*  9 */
	"vec 10",		/* 10 */
	"vec 11",		/* 11 */
	"vec 12",		/* 12 */
	"vec 13",		/* 13 */
	"vec 14",		/* 14 */
	"vec 15",		/* 15 */
	"vec 16",		/* 16 */
	"vec 17",		/* 17 */
	"vec 18",		/* 18 */
	"vec 19",		/* 19 */
	"vec 20",		/* 20 */
	"vec 21",		/* 21 */
	"vec 22",		/* 22 */
	"vec 23",		/* 23 */
	"vec 24",		/* 24 */
	"vec 25",		/* 25 */
	"vec 26",		/* 26 */
	"vec 27",		/* 27 */
	"vec 28",		/* 28 */
	"vec 29",		/* 29 */
	"vec 30",		/* 30 */
	"vec 31",		/* 31 */
	"vec 32",		/* 32 */
	"vec 33",		/* 33 */
	"vec 34",		/* 34 */
	"vec 35",		/* 35 */
	"vec 36",		/* 36 */
	"vec 37",		/* 37 */
	"vec 38",		/* 38 */
	"vec 39",		/* 39 */
	"vec 40",		/* 40 */
	"vec 41",		/* 41 */
	"vec 42",		/* 42 */
	"vec 43",		/* 43 */
	"vec 44",		/* 44 */
	"vec 45",		/* 45 */
	"vec 46",		/* 46 */
	"vec 47",		/* 47 */
	"vec 48",		/* 48 */
	"vec 49",		/* 49 */
	"vec 50",		/* 50 */
	"vec 51",		/* 51 */
	"vec 52",		/* 52 */
	"vec 53",		/* 53 */
	"vec 54",		/* 54 */
	"vec 55",		/* 55 */
	"vec 56",		/* 56 */
	"vec 57",		/* 57 */
	"vec 58",		/* 58 */
	"vec 59",		/* 59 */
	"vec 60",		/* 60 */
	"vec 61",		/* 61 */
	"vec 62",		/* 63 */
	"vec 63",		/* 63 */
};

/*
 * mask of CPUs attached
 * once they are attached, this var is read-only so mp safe
 */
static __cpuset_t cpu_present_mask;

kmutex_t *rmixl_ipi_lock;  /* covers RMIXL_PIC_IPIBASE */
kmutex_t *rmixl_intr_lock; /* covers rest of PIC, and rmixl_intrhand[] */
rmixl_intrvecq_t rmixl_intrvec_lruq[_IPL_N] = {
	[IPL_NONE] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_NONE]),
	[IPL_SOFTCLOCK] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_SOFTCLOCK]),
	[IPL_SOFTNET] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_SOFTNET]),
	[IPL_VM] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_VM]),
	[IPL_SCHED] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_SCHED]),
	[IPL_DDB] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_DDB]),
	[IPL_HIGH] = TAILQ_HEAD_INITIALIZER(rmixl_intrvec_lruq[IPL_HIGH]),
};
rmixl_intrvec_t rmixl_intrvec[NINTRVECS];
rmixl_intrhand_t rmixl_irt_intrhands[MAX(MAX(RMIXLR_NIRTS,RMIXLS_NIRTS), RMIXLP_NIRTS)];
static u_int rmixl_nirts;
const char * const *rmixl_irtnames;

#ifdef DIAGNOSTIC
static int rmixl_pic_init_done;
#endif


static uint32_t rmixl_irt_thread_mask(__cpuset_t);
static void rmixl_irt_init(size_t);
static void rmixl_irt_disestablish(size_t);
static void rmixl_irt_establish(size_t, size_t,
		rmixl_intr_trigger_t, rmixl_intr_polarity_t);
static size_t rmixl_intr_get_vec(int);

#ifdef MULTIPROCESSOR
static int rmixl_send_ipi(struct cpu_info *, int);
static int rmixl_ipi_intr(void *);
#endif

#if defined(DIAGNOSTIC) || defined(IOINTR_DEBUG) || defined(DDB)
int  rmixl_intrvec_print_subr(size_t);
int  rmixl_intrhand_print(void);
int  rmixl_irt_print(void);
void rmixl_ipl_eimr_map_print(void);
#endif


static inline u_int
dclz(uint64_t val)
{
	u_int nlz;

	__asm volatile("dclz %0, %1" : "=r"(nlz) : "r"(val));
	
	return nlz;
}

void
evbmips_intr_init(void)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	const bool is_xlr_p = cpu_rmixlr(mips_options.mips_cpu);
	const bool is_xls_p = cpu_rmixls(mips_options.mips_cpu);

	KASSERT(is_xlp_p || is_xlr_p || is_xls_p);

	/*
	 * The number of IRT entries is different for XLP .vs. XLR/XLS.
	 */
	if (is_xlp_p) {
		rmixl_irtnames = rmixl_irtnames_xlpxxx;
		rmixl_nirts = __arraycount(rmixl_irtnames_xlpxxx);
	} else if (is_xlr_p) {
		rmixl_irtnames = rmixl_irtnames_xlrxxx;
		rmixl_nirts = __arraycount(rmixl_irtnames_xlrxxx);
	} else if (is_xls_p) {
		switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
		case MIPS_XLS104:
		case MIPS_XLS108:
		case MIPS_XLS404LITE:
		case MIPS_XLS408LITE:
			rmixl_irtnames = rmixl_irtnames_xls1xx;
			rmixl_nirts = __arraycount(rmixl_irtnames_xls1xx);
			break;
		case MIPS_XLS204:
		case MIPS_XLS208:
			rmixl_irtnames = rmixl_irtnames_xls2xx;
			rmixl_nirts = __arraycount(rmixl_irtnames_xls2xx);
			break;
		case MIPS_XLS404:
		case MIPS_XLS408:
		case MIPS_XLS416:
		case MIPS_XLS608:
		case MIPS_XLS616:
			rmixl_irtnames = rmixl_irtnames_xls4xx;
			rmixl_nirts = __arraycount(rmixl_irtnames_xls4xx);
			break;
		default:
			rmixl_irtnames = rmixl_vecnames_common;
			rmixl_nirts = __arraycount(rmixl_vecnames_common);
			break;
		}
	}

#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done != 0)
		panic("%s: rmixl_pic_init_done %d",
			__func__, rmixl_pic_init_done);
#endif

	rmixl_ipi_lock  = mutex_obj_alloc(MUTEX_DEFAULT, IPL_HIGH);
	rmixl_intr_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_HIGH);

	mutex_enter(rmixl_intr_lock);

	/*
	 * Insert all non-IPI non-normal MIPS vectors on lru queue.
	 */
	for (size_t i = RMIXL_INTRVEC_IPI; i < NINTRVECS; i++) {
		TAILQ_INSERT_TAIL(&rmixl_intrvec_lruq[IPL_NONE],
		    &rmixl_intrvec[i], iv_lruq_link);
	}

	/*
	 * initialize (zero) all IRT Entries in the PIC
	 */
	for (size_t i = 0; i < rmixl_nirts; i++) {
		rmixl_irt_init(i);
	}

	/*
	 * disable watchdog NMI, timers
	 */
	if (is_xlp_p) {
		/*
		 * Reset the interrupt thread enables to disable all CPUs.
		 */
		for (size_t i = 0; i < 8; i++) {
			RMIXLP_PICREG_WRITE(RMIXLP_PIC_INT_THREAD_ENABLE01(i), 0);
			RMIXLP_PICREG_WRITE(RMIXLP_PIC_INT_THREAD_ENABLE23(i), 0);
		}

		/*
		 * Enable interrupts for node 0 core 0 thread 0.
		 */
		RMIXLP_PICREG_WRITE(RMIXLP_PIC_INT_THREAD_ENABLE01(0), 1);

		/*
		 * Disable watchdogs and system timers.
		 */
		uint64_t r = RMIXLP_PICREG_READ(RMIXLP_PIC_CTRL);
		r &= ~(RMIXLP_PIC_CTRL_WTE|RMIXLP_PIC_CTRL_STE);
		RMIXLP_PICREG_WRITE(RMIXLP_PIC_CTRL, r);
	} else {
		/*
		 * XXX
		 *  WATCHDOG_ENB is preserved because clearing it causes
		 *  hang on the XLS616 (but not on the XLS408)
		 */
		uint32_t r = RMIXL_PICREG_READ(RMIXL_PIC_CONTROL);
		r &= RMIXL_PIC_CONTROL_RESV|RMIXL_PIC_CONTROL_WATCHDOG_ENB;
		RMIXL_PICREG_WRITE(RMIXL_PIC_CONTROL, r);
	}

#ifdef DIAGNOSTIC
	rmixl_pic_init_done = 1;
#endif
	mutex_exit(rmixl_intr_lock);
}

/*
 * establish vector for mips3 count/compare clock interrupt
 * this ensures we enable in EIRR,
 * even though cpu_intr() handles the interrupt
 * note the 'mpsafe' arg here is a placeholder only
 */
void
rmixl_intr_init_clk(void)
{
	const size_t vec = ffs(MIPS_INT_MASK_5 >> MIPS_INT_MASK_SHIFT) - 1;

	mutex_enter(rmixl_intr_lock);

	void *ih = rmixl_vec_establish(vec, NULL, IPL_SCHED, NULL, NULL, false);
	if (ih == NULL)
		panic("%s: establish vec %zu failed", __func__, vec);

	mutex_exit(rmixl_intr_lock);
}

#ifdef MULTIPROCESSOR
/*
 * establish IPI interrupt and send function
 */
void
rmixl_intr_init_ipi(void)
{
	mutex_enter(rmixl_intr_lock);

	for (size_t ipi = 0; ipi < NIPIS; ipi++) {
		const size_t vec = RMIXL_INTRVEC_IPI + ipi;
		void * const ih = rmixl_vec_establish(vec, NULL, IPL_SCHED,
			rmixl_ipi_intr, (void *)(uintptr_t)ipi, true);
		if (ih == NULL)
			panic("%s: establish ipi %zu at vec %zu failed",
				__func__, ipi, vec);
	}

	mips_locoresw.lsw_send_ipi = rmixl_send_ipi;

	mutex_exit(rmixl_intr_lock);
}
#endif 	/* MULTIPROCESSOR */

/*
 * initialize per-cpu interrupt stuff in softc
 * accumulate per-cpu bits in 'cpu_present_mask'
 */
void
rmixl_intr_init_cpu(struct cpu_info *ci)
{
	struct rmixl_cpu_softc * const sc = (void *)ci->ci_softc;
	const char * xname = device_xname(sc->sc_dev);

	KASSERT(sc != NULL);
	KASSERT(NINTRVECS <= __arraycount(sc->sc_vec_evcnts));
	KASSERT(rmixl_nirts <= __arraycount(sc->sc_irt_evcnts));

	for (size_t vec = 0; vec < NINTRVECS; vec++) {
		evcnt_attach_dynamic(&sc->sc_vec_evcnts[vec],
		    EVCNT_TYPE_INTR, NULL, xname, rmixl_intr_string(vec));
	}

	for (size_t irt = 0; irt < rmixl_nirts; irt++) {
		evcnt_attach_dynamic(&sc->sc_irt_evcnts[irt],
		    EVCNT_TYPE_INTR, NULL, xname, rmixl_irtnames[irt]);
	}

	KASSERT(cpu_index(ci) < (sizeof(cpu_present_mask) * 8));
	atomic_or_32((volatile uint32_t *)&cpu_present_mask, 1 << cpu_index(ci));
}

const char *
rmixl_irt_string(size_t irt)
{
	KASSERT(irt < rmixl_nirts);

	return rmixl_irtnames[irt];
}

/*
 * rmixl_intr_string - return pointer to display name of a PIC-based interrupt
 */
const char *
rmixl_intr_string(size_t vec)
{

	if (vec >= NINTRVECS)
		panic("%s: vec index %zu out of range, max %d",
			__func__, vec, NINTRVECS - 1);

	return rmixl_vecnames_common[vec];
}

size_t
rmixl_intr_get_vec(int ipl)
{
	KASSERT(mutex_owned(rmixl_intr_lock));
	KASSERT(IPL_VM <= ipl && ipl <= IPL_HIGH);

	/*
	 * In reality higer ipls should have higher vec numbers,
	 * but for now don't worry about it.
	 */
	struct rmixl_intrvecq * freeq = &rmixl_intrvec_lruq[IPL_NONE];
	struct rmixl_intrvecq * iplq = &rmixl_intrvec_lruq[ipl];
	rmixl_intrvec_t *iv; 

	/*
	 * If there's a free vector, grab it otherwise choose the least
	 * recently assigned vector sharing this IPL.
	 */
	if ((iv = TAILQ_FIRST(freeq)) == NULL) {
		iv = TAILQ_FIRST(iplq);
		KASSERT(iv != NULL);
	}
	
	return iv - rmixl_intrvec;
}

/*
 * rmixl_irt_thread_mask
 *
 *	given a bitmask of cpus, return a, IRT thread mask
 */
static uint32_t
rmixl_irt_thread_mask(__cpuset_t cpumask)
{
	uint32_t irtc0;

#if defined(MULTIPROCESSOR)
#ifndef NOTYET
	if (cpumask == -1)
		return 1;	/* XXX TMP FIXME */
#endif

	/*
	 * discount cpus not present
	 */
	cpumask &= cpu_present_mask;
	
	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS204:
	case MIPS_XLS404:
	case MIPS_XLS404LITE:
		irtc0 = ((cpumask >> 2) << 4) | (cpumask & __BITS(1,0));
		irtc0 &= (__BITS(5,4) | __BITS(1,0));
		break;
	case MIPS_XLS108:
	case MIPS_XLS208:
	case MIPS_XLS408:
	case MIPS_XLS408LITE:
	case MIPS_XLS608:
		irtc0 = cpumask & __BITS(7,0);
		break;
	case MIPS_XLS416:
	case MIPS_XLS616:
		irtc0 = cpumask & __BITS(15,0);
		break;
	default:
		panic("%s: unknown cpu ID %#x\n", __func__,
			mips_options.mips_cpu_id);
	}
#else
	irtc0 = 1;
#endif	/* MULTIPROCESSOR */

	return irtc0;
}

/*
 * rmixl_irt_init
 * - initialize IRT Entry for given index
 * - unmask Thread#0 in low word (assume we only have 1 thread)
 */
static void
rmixl_irt_init(size_t irt)
{
	KASSERT(irt < rmixl_nirts);
	if (cpu_rmixlp(mips_options.mips_cpu)) {
		RMIXLP_PICREG_WRITE(RMIXLP_PIC_IRTENTRY(irt), 0);
	} else {
		RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irt), 0);	/* high word */
		RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irt), 0);	/* low  word */
	}
}

/*
 * rmixl_irt_disestablish
 * - invalidate IRT Entry for given index
 */
static void
rmixl_irt_disestablish(size_t irt)
{
	KASSERT(mutex_owned(rmixl_intr_lock));
	DPRINTF(("%s: irt %zu, irtc1 %#x\n", __func__, irt, 0));
	rmixl_irt_init(irt);
}

/*
 * rmixl_irt_establish
 * - construct an IRT Entry for irt and write to PIC
 */
static void
rmixl_irt_establish(size_t irt, size_t vec, rmixl_intr_trigger_t trigger,
	rmixl_intr_polarity_t polarity)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);

	KASSERT(mutex_owned(rmixl_intr_lock));

	if (irt >= rmixl_nirts)
		panic("%s: bad irt %zu\n", __func__, irt);

	/*
	 * All XLP interrupt are level.
	 */
	if (trigger != RMIXL_TRIG_LEVEL
	    && (is_xlp_p || trigger != RMIXL_TRIG_EDGE)) {
		panic("%s: bad trigger %d\n", __func__, trigger);
	}

	/*
	 * All XLP interrupt have high (positive) polarity.
	 */
	if (polarity != RMIXL_POLR_HIGH
	    && (is_xlp_p
		|| (polarity != RMIXL_POLR_RISING
		    && polarity != RMIXL_POLR_FALLING
		    && polarity != RMIXL_POLR_LOW))) {
		panic("%s: bad polarity %d\n", __func__, polarity);
	}

	/*
	 * XXX IRT entries are not shared
	 */
	if (is_xlp_p) {
		KASSERT(RMIXLP_PICREG_READ(RMIXLP_PIC_IRTENTRY(irt)) == 0);
		uint64_t irtc0 = RMIXLP_PIC_IRTENTRY_EN
		    | RMIXLP_PIC_IRTENTRY_LOCAL
		    | RMIXLP_PIC_IRTENTRY_DT_ITE
		    | RMIXLP_PIC_IRTENTRY_ITE(0)
		    | __SHIFTIN(vec, RMIXLP_PIC_IRTENTRY_INTVEC)

		/*
		 * write IRT Entry to PIC
		 */
		DPRINTF(("%s: vec %zu (%#x), irt %zu (%s), irtc0 %#"PRIx64"\n",
			__func__, vec, vec, irt, rmixl_irtnames[irt], irtc0));

		RMIXLP_PICREG_WRITE(RMIXLP_PIC_IRTENTRY(irt), irtc0);
	} else {
		KASSERT(RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irt)) == 0);
		KASSERT(RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irt)) == 0);

		__cpuset_t cpumask = 1; /* XXX */
		uint32_t irtc0 = rmixl_irt_thread_mask(cpumask);

		uint32_t irtc1 = RMIXL_PIC_IRTENTRYC1_VALID;
		irtc1 |= RMIXL_PIC_IRTENTRYC1_GL;	/* local */
		KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

		if (trigger == RMIXL_TRIG_LEVEL)
			irtc1 |= RMIXL_PIC_IRTENTRYC1_TRG;
		KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

		if (polarity == RMIXL_POLR_FALLING
		    || polarity == RMIXL_POLR_LOW)
			irtc1 |= RMIXL_PIC_IRTENTRYC1_P;
		KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

		irtc1 |= vec;			/* vector in EIRR */
		KASSERT((irtc1 & RMIXL_PIC_IRTENTRYC1_NMI) == 0);

		/*
		 * write IRT Entry to PIC
		 */
		DPRINTF(("%s: vec %zu (%#x), irt %zu, irtc0 %#x, irtc1 %#x\n",
			__func__, vec, vec, irt, irtc0, irtc1));
		RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC0(irt), irtc0);	/* low  word */
		RMIXL_PICREG_WRITE(RMIXL_PIC_IRTENTRYC1(irt), irtc1);	/* high word */
	}
}

void *
rmixl_vec_establish(size_t vec, rmixl_intrhand_t *ih, int ipl,
	int (*func)(void *), void *arg, bool mpsafe)
{

	KASSERT(mutex_owned(rmixl_intr_lock));

	DPRINTF(("%s: vec %zu ih %p ipl %d func %p arg %p mpsafe %d\n",
	    __func__, vec, ih, ipl, func, arg, mpsafe));

#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done == 0)
		panic("%s: called before evbmips_intr_init", __func__);
#endif

	/*
	 * check args
	 */
	if (vec >= NINTRVECS)
		panic("%s: vec %zu out of range, max %d",
		    __func__, vec, NINTRVECS - 1);
	if (ipl < IPL_VM || ipl > IPL_HIGH)
		panic("%s: ipl %d out of range, min %d, max %d",
		    __func__, ipl, IPL_VM, IPL_HIGH);

	const int s = splhigh();

	rmixl_intrvec_t * const iv = &rmixl_intrvec[vec];
	if (ih == NULL) {
		ih = &iv->iv_intrhand;
	}

	if (vec >= 8) {
		TAILQ_REMOVE(&rmixl_intrvec_lruq[iv->iv_ipl], iv, iv_lruq_link);
	}

	if (LIST_EMPTY(&iv->iv_hands)) {
		KASSERT(iv->iv_ipl == IPL_NONE);
		iv->iv_ipl = ipl;
	} else {
		KASSERT(iv->iv_ipl == ipl);
	}

	if (vec >= 8) {
		TAILQ_INSERT_TAIL(&rmixl_intrvec_lruq[iv->iv_ipl],
		    iv, iv_lruq_link);
	}

	if (ih->ih_func != NULL) {
#ifdef DIAGNOSTIC
		printf("%s: intrhand[%zu] busy\n", __func__, vec);
#endif
		splx(s);
		return NULL;
	}

	ih->ih_arg = arg;
	ih->ih_mpsafe = mpsafe;
	ih->ih_vec = vec;

	LIST_INSERT_HEAD(&iv->iv_hands, ih, ih_link);

	const uint64_t eimr_bit = (uint64_t)1 << vec;
	for (int i = ipl; --i >= 0; ) {
		KASSERT((ipl_eimr_map[i] & eimr_bit) == 0);
		ipl_eimr_map[i] |= eimr_bit;
	}

	ih->ih_func = func;	/* do this last */

	splx(s);

	return ih;
}

/*
 * rmixl_intr_establish
 * - used to establish an IRT-based interrupt only
 */
void *
rmixl_intr_establish(size_t irt, int ipl,
	rmixl_intr_trigger_t trigger, rmixl_intr_polarity_t polarity,
	int (*func)(void *), void *arg, bool mpsafe)
{
#ifdef DIAGNOSTIC
	if (rmixl_pic_init_done == 0)
		panic("%s: called before rmixl_pic_init_done", __func__);
#endif

	/*
	 * check args
	 */
	if (irt >= rmixl_nirts)
		panic("%s: irt %zu out of range, max %d",
		    __func__, irt, rmixl_nirts - 1);
	if (ipl < IPL_VM || ipl > IPL_HIGH)
		panic("%s: ipl %d out of range, min %d, max %d",
		    __func__, ipl, IPL_VM, IPL_HIGH);

	mutex_enter(rmixl_intr_lock);

	rmixl_intrhand_t *ih = &rmixl_irt_intrhands[irt];

	KASSERT(ih->ih_func == NULL);

	const size_t vec = rmixl_intr_get_vec(ipl);

	DPRINTF(("%s: irt %zu, ih %p vec %zu, ipl %d\n",
	    __func__, irt, ih, vec, ipl));

	/*
	 * establish vector
	 */
	ih = rmixl_vec_establish(vec, ih, ipl, func, arg, mpsafe);

	/*
	 * establish IRT Entry
	 */
	rmixl_irt_establish(irt, vec, trigger, polarity);

	mutex_exit(rmixl_intr_lock);

	return ih;
}

void
rmixl_vec_disestablish(void *cookie)
{
	rmixl_intrhand_t * const ih = cookie;
	const size_t vec = ih->ih_vec;
	rmixl_intrvec_t * const iv = &rmixl_intrvec[vec];

	KASSERT(mutex_owned(rmixl_intr_lock));
	KASSERT(vec < NINTRVECS);
	KASSERT(ih->ih_func != NULL);
	KASSERT(IPL_VM <= iv->iv_ipl && iv->iv_ipl <= IPL_HIGH);

	LIST_REMOVE(ih, ih_link);

	ih->ih_func = NULL;	/* do this first */

	const uint64_t eimr_bit = __BIT(ih->ih_vec);
	for (int i = iv->iv_ipl; --i >= 0; ) {
		KASSERT((ipl_eimr_map[i] & eimr_bit) != 0);
		ipl_eimr_map[i] ^= eimr_bit;
	}

	ih->ih_vec = 0;
	ih->ih_mpsafe = false;
	ih->ih_arg = NULL;

	/*
	 * If this vector isn't servicing any interrupts, then check to
	 * see if this IPL has other vectors using it.  If it does, then
	 * return this vector to the freeq (lruq for IPL_NONE).  This makes
	 * there will always be at least one vector per IPL.
	 */
	if (vec > 8 && LIST_EMPTY(&iv->iv_hands)) {
		rmixl_intrvecq_t * const freeq = &rmixl_intrvec_lruq[IPL_NONE];
		rmixl_intrvecq_t * const iplq = &rmixl_intrvec_lruq[iv->iv_ipl];

		if (TAILQ_NEXT(iv, iv_lruq_link) != NULL
		    || TAILQ_FIRST(iplq) != iv) {
			TAILQ_REMOVE(iplq, iv, iv_lruq_link);
			iv->iv_ipl = IPL_NONE;
			TAILQ_INSERT_TAIL(freeq, iv, iv_lruq_link);
		}
	}
}

void
rmixl_intr_disestablish(void *cookie)
{
	rmixl_intrhand_t * const ih = cookie;
	const size_t vec = ih->ih_vec;
	rmixl_intrvec_t * const iv = &rmixl_intrvec[vec];

	KASSERT(vec < NINTRVECS);

	mutex_enter(rmixl_intr_lock);

	/*
	 * disable/invalidate the IRT Entry if needed
	 */
	if (ih != &iv->iv_intrhand) {
		size_t irt = ih - rmixl_irt_intrhands;
		KASSERT(irt < rmixl_nirts);
		rmixl_irt_disestablish(irt);
	}

	/*
	 * disasociate from vector and free the handle
	 */
	rmixl_vec_disestablish(cookie);

	mutex_exit(rmixl_intr_lock);
}

void
evbmips_iointr(int ipl, vaddr_t pc, uint32_t pending)
{
	struct rmixl_cpu_softc * const sc = (void *)curcpu()->ci_softc;
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);

	DPRINTF(("%s: cpu%u: ipl %d, pc %#"PRIxVADDR", pending %#x\n",
		__func__, cpu_number(), ipl, pc, pending));

	/*
	 * 'pending' arg is a summary that there is something to do
	 * the real pending status is obtained from EIRR
	 */
	KASSERT(pending == MIPS_INT_MASK_1);

	for (;;) {
		rmixl_intrhand_t *ih;
		uint64_t eirr;
		uint64_t eimr;
		uint64_t vecbit;
		int vec;

		__asm volatile("dmfc0 %0, $9, 6;" : "=r"(eirr));
		__asm volatile("dmfc0 %0, $9, 7;" : "=r"(eimr));

#ifdef IOINTR_DEBUG
		printf("%s: cpu%u: eirr %#"PRIx64", eimr %#"PRIx64", mask %#"PRIx64"\n",
			__func__, cpu_number(), eirr, eimr, ipl_eimr_map[ipl-1]);
#endif	/* IOINTR_DEBUG */

		/*
		 * reduce eirr to
		 * - ints that are enabled at or below this ipl
		 * - exclude count/compare clock and soft ints
		 *   they are handled elsewhere
		 */
		eirr &= ipl_eimr_map[ipl-1];
		eirr &= ~ipl_eimr_map[ipl];
		eirr &= ~((MIPS_INT_MASK_5 | MIPS_SOFT_INT_MASK) >> 8);
		if (eirr == 0)
			break;

		vec = 63 - dclz(eirr);
		rmixl_intrvec_t * const iv = &rmixl_intrvec[vec];
		vecbit = 1ULL << vec;
		KASSERT (iv->iv_ipl == ipl);
		LIST_FOREACH(ih, &iv->iv_hands, ih_link) {
			KASSERT ((vecbit & eimr) == 0);
			KASSERT ((vecbit & RMIXL_EIRR_PRESERVE_MASK) == 0);

			/*
			 * ack in EIRR, and in PIC if needed,
			 * the irq we are about to handle
			 */
			rmixl_eirr_ack(eimr, vecbit, RMIXL_EIRR_PRESERVE_MASK);
			if (ih != &iv->iv_intrhand) {
				size_t irt = ih - rmixl_irt_intrhands;
				KASSERT(irt < rmixl_nirts);
				if (is_xlp_p) {
					RMIXLP_PICREG_WRITE(RMIXLP_PIC_INT_ACK,
						irt);
				} else {
					RMIXL_PICREG_WRITE(RMIXL_PIC_INTRACK,
						1 << irt);
				}
				sc->sc_irt_evcnts[irt].ev_count++;
			}

			if (ih->ih_func != NULL) {
#ifdef MULTIPROCESSOR
				if (ih->ih_mpsafe) {
					(void)(*ih->ih_func)(ih->ih_arg);
				} else {
					KASSERTMSG(ipl == IPL_VM,
					    ("%s: %s: ipl (%d) != IPL_VM for KERNEL_LOCK",
					    __func__, sc->sc_vec_evcnts[vec].ev_name,
					    ipl));
					KERNEL_LOCK(1, NULL);
					(void)(*ih->ih_func)(ih->ih_arg);
					KERNEL_UNLOCK_ONE(NULL);
				}
#else
				(void)(*ih->ih_func)(ih->ih_arg);
#endif /* MULTIPROCESSOR */
			}
			KASSERT(ipl == iv->iv_ipl);
			KASSERTMSG(curcpu()->ci_cpl >= ipl,
			    ("%s: after %s: cpl (%d) < ipl %d",
			    __func__, sc->sc_vec_evcnts[vec].ev_name,
			    ipl, curcpu()->ci_cpl));
			sc->sc_vec_evcnts[vec].ev_count++;
		}
	}
}

#ifdef MULTIPROCESSOR
static int
rmixl_send_ipi(struct cpu_info *ci, int tag)
{
	const cpuid_t cpuid = ci->ci_cpuid;
	const uint64_t req = 1 << tag;
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	uint32_t r;

	if (! CPUSET_HAS_P(cpus_running, cpu_index(ci)))
		return -1;

	KASSERT(tag >= 0 && tag < NIPIS);

	if (is_xlp_p) {
		r = RMXLP_PIC_IPI_CTRL_MAKE(0, __BIT(cpuid & 15),
		   RMIXL_INTERVEC_IPI + tag);
	} else {
		const uint32_t core = (uint32_t)(cpuid >> 2);
		const uint32_t thread = (uint32_t)(cpuid & __BITS(1,0));
		r = RMXLP_PIC_IPI_CTRL_MAKE(0, core, thread,
		   RMIXL_INTERVEC_IPI + tag);
	}

	mutex_enter(rmixl_ipi_lock);
	atomic_or_64(&ci->ci_request_ipis, req);
	__asm __volatile("sync");
	if (is_xlp_p) {
		RMIXLP_PICREG_WRITE(RMIXLP_PIC_IPI_CTRL, r);
	} else {
		RMIXL_PICREG_WRITE(RMIXL_PIC_IPIBASE, r);
	}
	mutex_exit(rmixl_ipi_lock);

	return 0;
}

static int
rmixl_ipi_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	const uint64_t ipi_mask = 1 << (uintptr_t)arg;

	KASSERT(ci->ci_cpl >= IPL_SCHED);
	KASSERT((uintptr_t)arg < NIPIS);

	/* if the request is clear, it was previously processed */
	if ((ci->ci_request_ipis & ipi_mask) == 0)
		return 0;

	atomic_or_64(&ci->ci_active_ipis, ipi_mask);
	atomic_and_64(&ci->ci_request_ipis, ~ipi_mask);

	ipi_process(ci, ipi_mask);

	atomic_and_64(&ci->ci_active_ipis, ~ipi_mask);

	return 1;
}
#endif	/* MULTIPROCESSOR */

#if defined(DIAGNOSTIC) || defined(IOINTR_DEBUG) || defined(DDB)
int
rmixl_intrvec_print_subr(size_t vec)
{
	rmixl_intrvec_t * const iv = &rmixl_intrvec[vec];
	rmixl_intrhand_t *ih;

	printf("vec %zu: ipl %u\n", vec, iv->iv_ipl);

	LIST_FOREACH(ih, &iv->iv_hands, ih_link) {
		if (ih == &iv->iv_intrhand) {
			printf("   [%s]: func %p, arg %p\n",
			    rmixl_vecnames_common[vec],
			    ih->ih_func, ih->ih_arg);
		} else {
			const size_t irt = ih - rmixl_irt_intrhands;
			printf("   irt %zu [%s]: func %p, arg %p\n",
			    irt, rmixl_irtnames[irt],
			    ih->ih_func, ih->ih_arg);
		}
	}
	return 0;
}
int
rmixl_intrhand_print(void)
{
	for (size_t vec = 0; vec < NINTRVECS; vec++)
		rmixl_intrvec_print_subr(vec);
	return 0;
}

static inline void
rmixl_irt_entry_print(size_t irt)
{
	if (irt >= rmixl_nirts)
		return;
	if (cpu_rmixlp(mips_options.mips_cpu)) {
		uint64_t c = RMIXLP_PICREG_READ(RMIXLP_PIC_IRTENTRY(irt));
		printf("irt[%zu]: %#"PRIx64"\n", irt, c);
	} else {
		uint32_t c0 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC0(irt));
		uint32_t c1 = RMIXL_PICREG_READ(RMIXL_PIC_IRTENTRYC1(irt));
		printf("irt[%zu]: %#x, %#x\n", irt, c0, c1);
	}
}

int
rmixl_irt_print(void)
{
	printf("%s:\n", __func__);
	for (size_t irt = 0; irt < rmixl_nirts ; irt++)
		rmixl_irt_entry_print(irt);
	return 0;
}

void
rmixl_ipl_eimr_map_print(void)
{
	printf("IPL_NONE=%d, mask %#"PRIx64"\n",
		IPL_NONE, ipl_eimr_map[IPL_NONE]);
	printf("IPL_SOFTCLOCK=%d, mask %#"PRIx64"\n",
		IPL_SOFTCLOCK, ipl_eimr_map[IPL_SOFTCLOCK]);
	printf("IPL_SOFTNET=%d, mask %#"PRIx64"\n",
		IPL_SOFTNET, ipl_eimr_map[IPL_SOFTNET]);
	printf("IPL_VM=%d, mask %#"PRIx64"\n",
		IPL_VM, ipl_eimr_map[IPL_VM]);
	printf("IPL_SCHED=%d, mask %#"PRIx64"\n",
		IPL_SCHED, ipl_eimr_map[IPL_SCHED]);
	printf("IPL_DDB=%d, mask %#"PRIx64"\n",
		IPL_DDB, ipl_eimr_map[IPL_DDB]);
	printf("IPL_HIGH=%d, mask %#"PRIx64"\n",
		IPL_HIGH, ipl_eimr_map[IPL_HIGH]);
}

#endif
