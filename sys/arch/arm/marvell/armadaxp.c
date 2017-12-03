/*	$NetBSD: armadaxp.c,v 1.2.2.4 2017/12/03 11:35:54 jdolecek Exp $	*/
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: armadaxp.c,v 1.2.2.4 2017/12/03 11:35:54 jdolecek Exp $");

#define _INTR_PRIVATE

#include "opt_mvsoc.h"

#include <sys/param.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <arm/pic/picvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/armadaxpreg.h>
#include <arm/marvell/armadaxpvar.h>

#include <dev/marvell/marvellreg.h>

#define EXTRACT_XP_CPU_FREQ_FIELD(sar)	(((0x01 & (sar >> 52)) << 3) | \
					    (0x07 & (sar >> 21)))
#define EXTRACT_XP_FAB_FREQ_FIELD(sar)	(((0x01 & (sar >> 51)) << 4) | \
					    (0x0F & (sar >> 24)))
#define EXTRACT_370_CPU_FREQ_FIELD(sar)	((sar >> 11) & 0xf)
#define EXTRACT_370_FAB_FREQ_FIELD(sar)	((sar >> 15) & 0x1f)

#define	MPIC_WRITE(reg, val)		(bus_space_write_4(&mvsoc_bs_tag, \
					    mpic_handle, reg, val))
#define	MPIC_CPU_WRITE(reg, val)	(bus_space_write_4(&mvsoc_bs_tag, \
					    mpic_cpu_handle, reg, val))

#define	MPIC_READ(reg)			(bus_space_read_4(&mvsoc_bs_tag, \
					    mpic_handle, reg))
#define	MPIC_CPU_READ(reg)		(bus_space_read_4(&mvsoc_bs_tag, \
					    mpic_cpu_handle, reg))

#define	L2_WRITE(reg, val)		(bus_space_write_4(&mvsoc_bs_tag, \
					    l2_handle, reg, val))
#define	L2_READ(reg)			(bus_space_read_4(&mvsoc_bs_tag, \
					    l2_handle, reg))
bus_space_handle_t mpic_cpu_handle;
static bus_space_handle_t mpic_handle, l2_handle;
int l2cache_state = 0;
int iocc_state = 0;
#define read_miscreg(r)		(*(volatile uint32_t *)(misc_base + (r)))
vaddr_t misc_base;
vaddr_t armadaxp_l2_barrier_reg;

static void armadaxp_intr_init(void);

static void armadaxp_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void armadaxp_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void armadaxp_pic_establish_irq(struct pic_softc *, struct intrsource *);
static void armadaxp_pic_set_priority(struct pic_softc *, int);
static void armadaxp_pic_source_name(struct pic_softc *, int, char*, size_t);

static int armadaxp_find_pending_irqs(void);
static void armadaxp_pic_block_irq(struct pic_softc *, size_t);

/* handle error cause */
static void armadaxp_err_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void armadaxp_err_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void armadaxp_err_pic_establish_irq(struct pic_softc *,
    struct intrsource *);
static void armadaxp_err_pic_source_name(struct pic_softc *,
    int, char*, size_t);
static int armadaxp_err_pic_pending_irqs(struct pic_softc *);

static void armadaxp_getclks(void);
static void armada370_getclks(void);
static int armadaxp_clkgating(struct marvell_attach_args *);

static int armadaxp_l2_init(bus_addr_t);
static paddr_t armadaxp_sdcache_wbalign_base(vaddr_t, paddr_t, psize_t);
static paddr_t armadaxp_sdcache_wbalign_end(vaddr_t, paddr_t, psize_t);
#ifdef AURORA_IO_CACHE_COHERENCY
static void armadaxp_io_coherency_init(void);
#endif

struct vco_freq_ratio {
	uint8_t	vco_cpu;	/* VCO to CLK0(CPU) clock ratio */
	uint8_t	vco_l2c;	/* VCO to NB(L2 cache) clock ratio */
	uint8_t	vco_hcl;	/* VCO to HCLK(DDR controller) clock ratio */
	uint8_t	vco_ddr;	/* VCO to DR(DDR memory) clock ratio */
};

/*
 * Interrupt names for ARMADA XP
 */
static const char * const armadaxp_pic_source_names[] = {
	/* Main Interrupt Cause Per-CPU (IRQ 0-29) */
	"InDBLowSum", "InDBHighSum", "OutDBSum", "CFU_LocalSum",
	"SoC_ErrorSum", "LTimer0", "LTimer1", "LWDT", "GbE0_TH_RxTx",
	"GbE0_RxTx", "GbE1_RxTxTh", "GbE1_RxTx", "GbE2_RxTxTh", "GbE2_RxTx",
	"GbE3_RxTxTh", "GbE3_RxTx", "GPIO0_7", "GPIO8_15", "GPIO16_23",
	"GPIO24_31", "GPIO32_39", "GPIO40_47", "GPIO48_55", "GPIO56_63",
	"GPIO64_66", "SCNT", "PCNT", "Reserved27", "VCNT", "Reserved29",
	/* Main Interrupt Cause Global-Shared (IRQ 30-115) */
	"SPI0", "I2C0", "I2C1", "IDMA0", "IDMA1", "IDMA2", "IDMA3", "GTimer0",
	"GTimer1", "GTimer2", "GTimer3", "UART0", "UART1", "UART2", "UART3",
	"USB0", "USB1", "USB2", "CESA0", "CESA1", "RTC", "XOR0_Ch0",
	"XOR0_Ch1", "BM", "SDIO", "SATA0", "TDM", "SATA1", "PEX0_0", "PEX0_1",
	"PEX0_2", "PEX0_3", "PEX1_0", "PEX1_1", "PEX1_2", "PEX1_3",
	"GbE0_Sum", "GbE0_Rx", "GbE0_Tx", "GbE0_Misc", "GbE1_Sum", "GbE1_Rx",
	"GbE1_Tx", "GbE1_Misc", "GbE2_Sum", "GbE2_Rx", "GbE2_Tx", "GbE2_Misc",
	"GbE3_Sum", "GbE3_Rx", "GbE3_Tx", "GbE3_Misc", "GPIO0_7", "GPIO8_15",
	"GPIO16_23", "GPIO24_31", "Reserved86", "GPIO32_39", "GPIO40_47",
	"GPIO48_55", "GPIO56_63", "GPIO64_66", "SPI1", "WDT", "XOR1_Ch2",
	"XOR1_Ch3", "SharedDB1Sum", "SharedDB2Sum", "SharedDB3Sum", "PEX2_0",
	"Reserved100", "Reserved101", "Reserved102", "PEX3_0", "Reserved104",
	"Reserved105", "Reserved106", "PMU", "DRAM", "GbE0_Wakeup",
	"GbE1_Wakeup", "GbE2_Wakeup", "GbE3_Wakeup", "NAND", "Reserved114",
	"Reserved115"
};
static const char * const armadaxp_err_pic_source_names[] = {
	/*
	 * IRQ 120-151 (bit 0-31 in SoC Error Interrupt Cause register)
	 * connected to SoC_ErrorSum in Main Interrupt Cause
	 */
	"ERR_CESA0", "ERR_DevBus", "ERR_IDMA", "ERR_XOR1",
	"ERR_PEX0", "ERR_PEX1", "ERR_GbE", "ERR_CESA1",
	"ERR_USB", "ERR_DRAM", "ERR_XOR0", "ERR_Reserved11",
	"ERR_BM", "ERR_CIB", "ERR_Reserved14", "ERR_PEX2",
	"ERR_PEX3", "ERR_SATA0", "ERR_SATA1", "ERR_Reserved19",
	"ERR_TDM", "ERR_NAND", "ERR_Reserved22",
	"ERR_Reserved23", "ERR_Reserved24", "ERR_Reserved25",
	"ERR_Reserved26", "ERR_Reserved27", "ERR_Reserved28",
	"ERR_Reserved29", "ERR_Reserved30", "ERR_Reserved31",
};

/*
 * Mbus Target and Attribute bindings for ARMADA XP
 */
static struct mbus_description {
	uint8_t target;
	uint8_t attr;
	const char *string;
} mbus_desc[] = {
	/* DDR */
	{ ARMADAXP_UNITID_DDR, ARMADAXP_ATTR_DDR_CS0,
	       	"DDR(M_CS[0])" },
	{ ARMADAXP_UNITID_DDR, ARMADAXP_ATTR_DDR_CS1,
	       	"DDR(M_CS[1])" },
	{ ARMADAXP_UNITID_DDR, ARMADAXP_ATTR_DDR_CS2,
	       	"DDR(M_CS[2])" },
	{ ARMADAXP_UNITID_DDR, ARMADAXP_ATTR_DDR_CS3,
	       	"DDR(M_CS[3])" },

	/* DEVBUS */
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS0,
	       	"DEVBUS(SPI0 CS0)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS1,
	       	"DEVBUS(SPI0 CS1)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS2,
	       	"DEVBUS(SPI0 CS2)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS3,
	       	"DEVBUS(SPI0 CS3)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS4,
	       	"DEVBUS(SPI0 CS4)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS5,
	       	"DEVBUS(SPI0 CS5)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS6,
	       	"DEVBUS(SPI0 CS6)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS7,
	       	"DEVBUS(SPI0 CS7)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS0,
	       	"DEVBUS(SPI1 CS0)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS1,
	       	"DEVBUS(SPI1 CS1)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS2,
	       	"DEVBUS(SPI1 CS2)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS3,
	       	"DEVBUS(SPI1 CS3)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS4,
	       	"DEVBUS(SPI1 CS4)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS5,
	       	"DEVBUS(SPI1 CS5)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS6,
	       	"DEVBUS(SPI1 CS6)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI1_CS7,
	       	"DEVBUS(SPI1 CS7)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS0,
	       	"DEVBUS(DevCS[0])" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS1,
	       	"DEVBUS(DevCS[1])" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS2,
	       	"DEVBUS(DevCS[2])" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS3,
	       	"DEVBUS(DevCS[3])" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_BOOT_CS,
	       	"DEVBUS(BootCS)" },
	{ ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_BOOT_ROM,
	       	"DEVBUS(BootROM)" },

	/* GbE */
	{ ARMADAXP_UNITID_GBE0, ARMADAXP_ATTR_GBE_RESERVED,
	       	"GBE0 GBE1" },
	{ ARMADAXP_UNITID_GBE2, ARMADAXP_ATTR_GBE_RESERVED,
	       	"GBE2 GBE3" },

	/* PEX(PCIe) */
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx0_MEM,
	       	"PEX0(Lane0, Memory)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx1_MEM,
	       	"PEX0(Lane1, Memory)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx2_MEM,
	       	"PEX0(Lane2, Memory)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx3_MEM,
	       	"PEX0(Lane3, Memory)" },
	{ ARMADAXP_UNITID_PEX2, ARMADAXP_ATTR_PEX2_MEM,
	       	"PEX2(Lane0, Memory)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx0_MEM,
	       	"PEX1(Lane0, Memory)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx1_MEM,
	       	"PEX1(Lane1, Memory)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx2_MEM,
	       	"PEX1(Lane2, Memory)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx3_MEM,
	       	"PEX1(Lane3, Memory)" },
	{ ARMADAXP_UNITID_PEX3, ARMADAXP_ATTR_PEX3_MEM,
	       	"PEX3(Lane0, Memory)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx0_IO,
	       	"PEX0(Lane0, I/O)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx1_IO,
	       	"PEX0(Lane1, I/O)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx2_IO,
	       	"PEX0(Lane2, I/O)" },
	{ ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx3_IO,
	       	"PEX0(Lane3, I/O)" },
	{ ARMADAXP_UNITID_PEX2, ARMADAXP_ATTR_PEX2_IO,
	       	"PEX2(Lane0, I/O)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx0_IO,
	       	"PEX1(Lane0, I/O)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx1_IO,
	       	"PEX1(Lane1, I/O)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx2_IO,
	       	"PEX1(Lane2, I/O)" },
	{ ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx3_IO,
	       	"PEX1(Lane3, I/O)" },
	{ ARMADAXP_UNITID_PEX3, ARMADAXP_ATTR_PEX3_IO,
	       	"PEX3(Lane0, I/O)" },

	/* CRYPT */
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT0_NOSWAP,
	       	"CESA0(No swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT0_SWAP_BYTE,
	       	"CESA0(Byte swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT0_SWAP_BYTE_WORD,
	       	"CESA0(Byte and word swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT0_SWAP_WORD,
	       	"CESA0(Word swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT1_NOSWAP,
	       	"CESA1(No swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT1_SWAP_BYTE,
	       	"CESA1(Byte swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT1_SWAP_BYTE_WORD,
	       	"CESA1(Byte and word swap)" },
	{ ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT1_SWAP_WORD,
	       	"CESA1(Word swap)" },

	/* BM */
	{ ARMADAXP_UNITID_BM, ARMADAXP_ATTR_BM_RESERVED,
		"BM" },

	/* NAND */
	{ ARMADAXP_UNITID_NAND, ARMADAXP_ATTR_NAND_RESERVED,
		"NAND" },
};

/*
 * Default Mbus address decoding table for ARMADA XP
 * this table may changed by device drivers.
 *
 * NOTE: some version of u-boot is broken. it writes old decoding table.
 *       probably, it's designed for Kirkwood SoC or older. we need to restore
 *       ARMADA XP's parameter set.
 */
static struct mbus_table_def {
	int window;	/* index of address decoding window registers */
	uint32_t base;	/* base address of the window */
	uint32_t size;	/* size of the window */
	uint8_t target;	/* target unit of the window */
	uint8_t attr;	/* target attribute of the window */
} mbus_table[] = {
	/*
	 * based on 'default address mapping' described in Marvell's document
	 * 'ARMADA XP Functional Specifications.'
	 *
	 * some windows are modified to get compatibility with old codes.
	 */
	{
		/* PCIe 0 lane0 MEM */
		/* MODIFIED (moved to MARVELL_PEXMEM_PBASE) */
		 0, 0xe0000000, 0x01000000,
		 ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx0_MEM
	},
	{
		/* PCIe 0 lane1 MEM */
		 1, 0x88000000, 0x08000000,
		 ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx1_MEM
	},
	{
		/* PCIe 0 lane2 MEM */
		 2, 0x90000000, 0x08000000,
		 ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx2_MEM
	},
	{
		/* PCIe 0 lane3 MEM */
		 3, 0x98000000, 0x08000000,
		 ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx3_MEM
	},
	{
		/* PCIe 1 lane0 MEM */
		 4, 0xa0000000, 0x08000000,
		 ARMADAXP_UNITID_PEX1, ARMADAXP_ATTR_PEXx0_MEM
	},
	{	5, 0, 0, 0, 0 /* disabled */ },
	{	6, 0, 0, 0, 0 /* disabled */ },
	{	7, 0, 0, 0, 0 /* disabled */ },
	{
		/* Security Accelerator SRAM, Engine 0, no data swap */
		 8, 0xc8010000, 0x00010000,
		 ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT0_NOSWAP,
	},
	{
		/* Device bus, BOOT_CS */
		 9, 0xd8000000, 0x08000000,
		 ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_BOOT_CS,
	},
	{
		/* Device bus, DEV_CS[0] */
		/* MODIFIED (moved, conflict to MARVELL_PEXMEM_PBASE here.) */
		10, 0x80000000, 0x08000000,
		ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS0
	},
	{
		/* Device bus, DEV_CS[1] */
		11, 0xe8000000, 0x08000000,
		ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS1
	},
	{
		/* Device bus, DEV_CS[2] */
		/* MODIFIED: (disabled, conflict to MARVELL_PEXIO_PBASE) */
		12, 0xf0000000, 0x00000000,
		ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_DEV_CS2
	},
	{
		/* Device bus, BOOT_ROM */
		13, 0xf8000000, 0x08000000,
		ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_BOOT_ROM
	},
	{
		/* Device bus, SPI0_CS[0] */
		14, 0xd4000000, 0x04000000,
		ARMADAXP_UNITID_DEVBUS, ARMADAXP_ATTR_DEVBUS_SPI0_CS0
	},
	{
		/* Security Accelerator SRAM, Engine 1, no data swap */
		/* MODIFIED (added, 0xd0300000-0xd030ffff) */
		15, 0xd0300000, 0x00010000,
	       	ARMADAXP_UNITID_CRYPT, ARMADAXP_ATTR_CRYPT1_NOSWAP
	},
	{
		/* PCIe 0 lane 0 I/O */
		/* MODIFIED (added, MARVELL_PEXIO_PBASE) */
		16, 0xf2000000, 0x00100000,
	       	ARMADAXP_UNITID_PEX0, ARMADAXP_ATTR_PEXx0_IO
       	},
	{	17, 0xd0320000, 0, 0, 0 /* disabled */ },
	{
		/* Buffer Manamgement unit */
		18, 0xd3800000, 0x00800000,
		ARMADAXP_UNITID_BM, ARMADAXP_ATTR_BM_RESERVED
	},
	{
		/* DDR */
		/* MODIFIED (up to 2GB memory space) */
		19, 0x00000000, 0x80000000,
		ARMADAXP_UNITID_DDR, ARMADAXP_ATTR_DDR_CS0
	},

};

static struct vco_freq_ratio freq_conf_table[] = {
/*00*/	{ 1, 1,	 4,  2 },
/*01*/	{ 1, 2,	 2,  2 },
/*02*/	{ 2, 2,	 6,  3 },
/*03*/	{ 2, 2,	 3,  3 },
/*04*/	{ 1, 2,	 3,  3 },
/*05*/	{ 1, 2,	 4,  2 },
/*06*/	{ 1, 1,	 2,  2 },
/*07*/	{ 2, 3,	 6,  6 },
/*08*/	{ 2, 3,	 5,  5 },
/*09*/	{ 1, 2,	 6,  3 },
/*10*/	{ 2, 4,	10,  5 },
/*11*/	{ 1, 3,	 6,  6 },
/*12*/	{ 1, 2,	 5,  5 },
/*13*/	{ 1, 3,	 6,  3 },
/*14*/	{ 1, 2,	 5,  5 },
/*15*/	{ 2, 2,	 5,  5 },
/*16*/	{ 1, 1,	 3,  3 },
/*17*/	{ 2, 5,	10, 10 },
/*18*/	{ 1, 3,	 8,  4 },
/*19*/	{ 1, 1,	 2,  1 },
/*20*/	{ 2, 3,	 6,  3 },
/*21*/	{ 1, 2,	 8,  4 },
/*22*/	{ 2, 5,	10,  5 }
};

static uint16_t clock_table_xp[] = {
	1000, 1066, 1200, 1333, 1500, 1666, 1800, 2000,
	 600,  667,  800, 1600, 2133, 2200, 2400
};
static uint16_t clock_table_370[] = {
	 400,  533,  667,  800, 1000, 1067, 1200, 1333,
	1500, 1600, 1667, 1800, 2000,  333,  600,  900,
	   0
};

static struct pic_ops armadaxp_picops = {
	.pic_unblock_irqs = armadaxp_pic_unblock_irqs,
	.pic_block_irqs = armadaxp_pic_block_irqs,
	.pic_establish_irq = armadaxp_pic_establish_irq,
	.pic_set_priority = armadaxp_pic_set_priority,
	.pic_source_name = armadaxp_pic_source_name,
};

static struct pic_softc armadaxp_pic = {
	.pic_ops = &armadaxp_picops,
	.pic_name = "armadaxp",
};

static struct pic_ops armadaxp_err_picops = {
	.pic_unblock_irqs = armadaxp_err_pic_unblock_irqs,
	.pic_block_irqs = armadaxp_err_pic_block_irqs,
	.pic_establish_irq = armadaxp_err_pic_establish_irq,
	.pic_find_pending_irqs = armadaxp_err_pic_pending_irqs,
	.pic_source_name = armadaxp_err_pic_source_name,
};

static struct pic_softc armadaxp_err_pic = {
	.pic_ops = &armadaxp_err_picops,
	.pic_name = "armadaxp_err",
};

static struct {
	bus_size_t offset;
	uint32_t bits;
} clkgatings[]= {
	{ ARMADAXP_GBE3_BASE,	(1 << 1) },
	{ ARMADAXP_GBE2_BASE,	(1 << 2) },
	{ ARMADAXP_GBE1_BASE,	(1 << 3) },
	{ ARMADAXP_GBE0_BASE,	(1 << 4) },
	{ MVSOC_PEX_BASE,	(1 << 5) },
	{ ARMADAXP_PEX01_BASE,	(1 << 6) },
	{ ARMADAXP_PEX02_BASE,	(1 << 7) },
	{ ARMADAXP_PEX03_BASE,	(1 << 8) },
	{ ARMADAXP_PEX10_BASE,	(1 << 9) },
	{ ARMADAXP_PEX11_BASE,	(1 << 10) },
	{ ARMADAXP_PEX12_BASE,	(1 << 11) },
	{ ARMADAXP_PEX13_BASE,	(1 << 12) },
#if 0
	{ NetA, (1 << 13) },
#endif
	{ ARMADAXP_SATAHC_BASE,	(1 << 14) | (1 << 15) | (1 << 29) | (1 << 30) },
	{ ARMADAXP_LCD_BASE,	(1 << 16) },
	{ ARMADAXP_SDIO_BASE,	(1 << 17) },
	{ ARMADAXP_USB1_BASE,	(1 << 19) },
	{ ARMADAXP_USB2_BASE,	(1 << 20) },
	{ ARMADAXP_CESA0_BASE,	(1 << 23) },
	{ ARMADAXP_CESA1_BASE,	(1 << 23) },
	{ ARMADAXP_PEX2_BASE,	(1 << 26) },
	{ ARMADAXP_PEX3_BASE,	(1 << 27) },
#if 0
	{ DDR, (1 << 28) },
#endif
};

/*
 * armadaxp_bootstrap:
 *
 *	Initialize the rest of the Armada XP dependencies, making it
 *	ready to handle interrupts from devices.
 */
void
armadaxp_bootstrap(vaddr_t vbase, bus_addr_t pbase)
{
	int i;

	/* Map MPIC base and MPIC percpu base registers */
	if (bus_space_map(&mvsoc_bs_tag, pbase + ARMADAXP_MLMB_MPIC_BASE,
	    0x500, 0, &mpic_handle) != 0)
		panic("%s: Could not map MPIC registers", __func__);
	if (bus_space_map(&mvsoc_bs_tag, pbase + ARMADAXP_MLMB_MPIC_CPU_BASE,
	    0x800, 0, &mpic_cpu_handle) != 0)
		panic("%s: Could not map MPIC percpu registers", __func__);

	/* Disable all interrupts */
	for (i = 0; i < ARMADAXP_IRQ_SOURCES; i++)
		MPIC_WRITE(ARMADAXP_MLMB_MPIC_ICE, i);

	mvsoc_intr_init = armadaxp_intr_init;

	mvsoc_clkgating = armadaxp_clkgating;

	misc_base = vbase + ARMADAXP_MISC_BASE;
	switch (mvsoc_model()) {
	case MARVELL_ARMADAXP_MV78130:
	case MARVELL_ARMADAXP_MV78160:
	case MARVELL_ARMADAXP_MV78230:
	case MARVELL_ARMADAXP_MV78260:
	case MARVELL_ARMADAXP_MV78460:
		armadaxp_getclks();
		break;

	case MARVELL_ARMADA370_MV6707:
	case MARVELL_ARMADA370_MV6710:
	case MARVELL_ARMADA370_MV6W11:
		armada370_getclks();
		break;
	}

#ifdef L2CACHE_ENABLE
	/* Initialize L2 Cache */
	armadaxp_l2_init(pbase);
#endif

#ifdef AURORA_IO_CACHE_COHERENCY
	/* Initialize cache coherency */
	armadaxp_io_coherency_init();
#endif
}

static void
armadaxp_intr_init(void)
{
	int ctrl;
	void *ih __diagused;

	/* Get max interrupts */
	armadaxp_pic.pic_maxsources =
	    ((MPIC_READ(ARMADAXP_MLMB_MPIC_CTRL) >> 2) & 0x7FF);

	if (!armadaxp_pic.pic_maxsources)
		armadaxp_pic.pic_maxsources = ARMADAXP_IRQ_SOURCES;

	pic_add(&armadaxp_pic, 0);

	/* Chain error interrupts controller */
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ERR_MASK, 0);
	armadaxp_err_pic.pic_maxsources = ARMADAXP_IRQ_ERROR_SOURCES;
	pic_add(&armadaxp_err_pic, ARMADAXP_IRQ_ERROR_BASE);
	ih = intr_establish(ARMADAXP_IRQ_ERR_SUMMARY, IPL_HIGH, IST_LEVEL_HIGH,
	    pic_handle_intr, &armadaxp_err_pic);
	KASSERT(ih != NULL);

	ctrl = MPIC_READ(ARMADAXP_MLMB_MPIC_CTRL);
	/* Enable IRQ prioritization */
	ctrl |= (1 << 0);
	MPIC_WRITE(ARMADAXP_MLMB_MPIC_CTRL, ctrl);

	find_pending_irqs = armadaxp_find_pending_irqs;
}

static void
armadaxp_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	int n;

	while (irq_mask != 0) {
		n = ffs(irq_mask) - 1;
		KASSERT(pic->pic_maxsources >= n + irqbase);
		MPIC_WRITE(ARMADAXP_MLMB_MPIC_ISE, n + irqbase);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ICM, n + irqbase);
		if ((n + irqbase) == 0)
			MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_DOORBELL_MASK,
			    0xffffffff);
		irq_mask &= ~__BIT(n);
	}
}

static void
armadaxp_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	int n;

	while (irq_mask != 0) {
		n = ffs(irq_mask) - 1;
		KASSERT(pic->pic_maxsources >= n + irqbase);
		MPIC_WRITE(ARMADAXP_MLMB_MPIC_ICE, n + irqbase);
		MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ISM, n + irqbase);
		irq_mask &= ~__BIT(n);
	}
}

static void
armadaxp_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	int tmp;
	KASSERT(pic->pic_maxsources >= is->is_irq);
	tmp = MPIC_READ(ARMADAXP_MLMB_MPIC_ISCR_BASE + is->is_irq * 4);
	/* Clear previous priority */
	tmp &= ~(0xf << MPIC_ISCR_SHIFT);
	MPIC_WRITE(ARMADAXP_MLMB_MPIC_ISCR_BASE + is->is_irq * 4,
	    tmp | (is->is_ipl << MPIC_ISCR_SHIFT));
}

static void
armadaxp_pic_set_priority(struct pic_softc *pic, int ipl)
{
	int ctp;

	ctp = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_CTP);
	ctp &= ~(0xf << MPIC_CTP_SHIFT);
	ctp |= (ipl << MPIC_CTP_SHIFT);
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_CTP, ctp);
}

static void
armadaxp_pic_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{
	if (irq >= __arraycount(armadaxp_pic_source_names)) {
		snprintf(buf, len, "Unknown IRQ %d", irq);
		return;
	}
	strlcpy(buf, armadaxp_pic_source_names[irq], len);
}

static int
armadaxp_find_pending_irqs(void)
{
	struct intrsource *is;
	int irq;

	irq = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_IIACK) & 0x3ff;

	/* Is it a spurious interrupt ?*/
	if (irq == 0x3ff)
		return 0;
	is = armadaxp_pic.pic_sources[irq];
	if (is == NULL) {
		printf("stray interrupt: %d\n", irq);
		return 0;
	}

	armadaxp_pic_block_irq(&armadaxp_pic, irq);
	pic_mark_pending(&armadaxp_pic, irq);

	return is->is_ipl;
}

static void
armadaxp_pic_block_irq(struct pic_softc *pic, size_t irq)
{

	KASSERT(pic->pic_maxsources >= irq);
	MPIC_WRITE(ARMADAXP_MLMB_MPIC_ICE, irq);
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ISM, irq);
}

static void
armadaxp_err_pic_source_name(struct pic_softc *pic, int irq,
    char *buf, size_t len)
{
	if (irq >= __arraycount(armadaxp_err_pic_source_names)) {
		snprintf(buf, len, "Unknown IRQ %d", irq);
		return;
	}
	strlcpy(buf, armadaxp_err_pic_source_names[irq], len);
}


/*
 * ARMADAXP_MLMB_MPIC_ERR_CAUSE
 */
static void
armadaxp_err_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	uint32_t reg;

	KASSERT(irqbase == 0); /* XXX: support offset */

	reg = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_ERR_MASK);
	reg |= irq_mask;
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ERR_MASK, reg);
}

static void
armadaxp_err_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
    uint32_t irq_mask)
{
	uint32_t reg;

	KASSERT(irqbase == 0); /* XXX: support offset */

	reg = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_ERR_MASK);
	reg &= ~irq_mask;
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ERR_MASK, reg);
}

static void
armadaxp_err_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	uint32_t reg;

	KASSERT(pic->pic_maxsources >= is->is_irq);

	reg = MPIC_CPU_READ(ARMADAXP_MLMB_MPIC_ERR_MASK);
	reg |= ARMADAXP_IRQ_ERROR_BIT(is->is_irq);
	MPIC_CPU_WRITE(ARMADAXP_MLMB_MPIC_ERR_MASK, reg);
}

static int
armadaxp_err_pic_pending_irqs(struct pic_softc *pic)
{
	struct intrsource *is;
	uint32_t reg;
	int irq;

	reg = MPIC_READ(ARMADAXP_MLMB_MPIC_ERR_CAUSE);
	irq = ffs(reg);
	if (irq == 0)
		return 0;
	irq--; /* bit number to index */

	is = pic->pic_sources[irq];
	if (is == NULL) {
		printf("stray interrupt: %d\n", irq);
		return 0;
	}
	return pic_mark_pending_sources(pic, 0, irq);
}


/*
 * Clock functions
 */

static void
armadaxp_getclks(void)
{
	uint64_t sar_reg;
	uint8_t  sar_cpu_freq, sar_fab_freq;

	if (cputype == CPU_ID_MV88SV584X_V7)
		mvTclk = 250000000; /* 250 MHz */
	else
		mvTclk = 200000000; /* 200 MHz */

	sar_reg = (read_miscreg(ARMADAXP_MISC_SAR_HI) << 31) |
	    read_miscreg(ARMADAXP_MISC_SAR_LO);

	sar_cpu_freq = EXTRACT_XP_CPU_FREQ_FIELD(sar_reg);
	sar_fab_freq = EXTRACT_XP_FAB_FREQ_FIELD(sar_reg);

	/* Check if CPU frequency field has correct value */
	if (sar_cpu_freq >= __arraycount(clock_table_xp))
		panic("Reserved value in cpu frequency configuration field: "
		    "%d", sar_cpu_freq);

	/* Check if fabric frequency field has correct value */
	if (sar_fab_freq >= __arraycount(freq_conf_table))
		panic("Reserved value in fabric frequency configuration field: "
		    "%d", sar_fab_freq);

	/* Get CPU clock frequency */
	mvPclk = clock_table_xp[sar_cpu_freq] *
	    freq_conf_table[sar_fab_freq].vco_cpu;

	/* Get L2CLK clock frequency and use as system clock (mvSysclk) */
	mvSysclk = mvPclk / freq_conf_table[sar_fab_freq].vco_l2c;

	/* Round mvSysclk value to integer MHz */
	if (((mvPclk % freq_conf_table[sar_fab_freq].vco_l2c) * 10 /
	    freq_conf_table[sar_fab_freq].vco_l2c) >= 5)
		mvSysclk++;

	mvPclk *= 1000000;
	mvSysclk *= 1000000;

	curcpu()->ci_data.cpu_cc_freq = mvPclk;
}

static void
armada370_getclks(void)
{
	uint32_t sar;
	uint8_t  cpu_freq, fab_freq;

	sar = read_miscreg(ARMADAXP_MISC_SAR_LO);
	if (sar & 0x00100000)
		mvTclk = 200000000; /* 200 MHz */
	else
		mvTclk = 166666667; /* 166 MHz */

	cpu_freq = EXTRACT_370_CPU_FREQ_FIELD(sar);
	fab_freq = EXTRACT_370_FAB_FREQ_FIELD(sar);

	/* Check if CPU frequency field has correct value */
	if (cpu_freq >= __arraycount(clock_table_370))
		panic("Reserved value in cpu frequency configuration field: "
		    "%d", cpu_freq);

	/* Check if fabric frequency field has correct value */
	if (fab_freq >= __arraycount(freq_conf_table))
		panic("Reserved value in fabric frequency configuration field: "
		    "%d", fab_freq);

	/* Get CPU clock frequency */
	mvPclk = clock_table_370[cpu_freq] *
	    freq_conf_table[fab_freq].vco_cpu;

	/* Get L2CLK clock frequency and use as system clock (mvSysclk) */
	mvSysclk = mvPclk / freq_conf_table[fab_freq].vco_l2c;

	/* Round mvSysclk value to integer MHz */
	if (((mvPclk % freq_conf_table[fab_freq].vco_l2c) * 10 /
	    freq_conf_table[fab_freq].vco_l2c) >= 5)
		mvSysclk++;

	mvPclk *= 1000000;
	mvSysclk *= 1000000;
}

/*
 * L2 Cache initialization
 */

static int
armadaxp_l2_init(bus_addr_t pbase)
{
	u_int32_t reg;
	int ret;

	/* Map L2 space */
	ret = bus_space_map(&mvsoc_bs_tag, pbase + ARMADAXP_L2_BASE,
	    0x1000, 0, &l2_handle);
	if (ret) {
		printf("%s: Cannot map L2 register space, ret:%d\n",
		    __func__, ret);
		return (-1);
	}

	/* Variables for cpufunc_asm_pj4b.S */
	/* XXX: per cpu register. need to support SMP */
	armadaxp_l2_barrier_reg = mlmb_base + MVSOC_MLMB_CIB_BARRIER(0);

	/* Set L2 policy */
	reg = L2_READ(ARMADAXP_L2_AUX_CTRL);
	reg &= ~(L2_AUX_WBWT_MODE_MASK);
	reg &= ~(L2_AUX_REP_STRAT_MASK);
	reg |= L2_AUX_ECC_ENABLE;
	reg |= L2_AUX_PARITY_ENABLE;
	reg |= L2_AUX_WBWT_MODE_BY_ATTR;
	reg |= L2_AUX_FORCE_WA_BY_ATTR;
	reg |= L2_AUX_REP_STRAT_SEMIPLRU;
	L2_WRITE(ARMADAXP_L2_AUX_CTRL, reg);

	/* Invalidate L2 cache */
	L2_WRITE(ARMADAXP_L2_INV_WAY, L2_ALL_WAYS);

	/* Clear pending L2 interrupts */
	L2_WRITE(ARMADAXP_L2_INT_CAUSE, 0x1ff);

	/* Enable Cache and TLB maintenance broadcast */
	__asm__ __volatile__ ("mrc p15, 1, %0, c15, c2, 0" : "=r"(reg));
	reg |= (1 << 8);
	__asm__ __volatile__ ("mcr p15, 1, %0, c15, c2, 0" : :"r"(reg));

	/*
	 * Set the Point of Coherency and Point of Unification to DRAM.
	 * This is a reset value but anyway, configure this just in case.
	 */
	reg = read_mlmbreg(ARMADAXP_L2_CFU);
	reg |= (1 << 17) | (1 << 18);
	write_mlmbreg(ARMADAXP_L2_CFU, reg);

	/* Enable L2 cache */
	reg = L2_READ(ARMADAXP_L2_CTRL);
	L2_WRITE(ARMADAXP_L2_CTRL, reg | L2_CTRL_ENABLE);

	/* Mark as enabled */
	l2cache_state = 1;

#ifdef DEBUG
	/* Configure and enable counter */
	L2_WRITE(ARMADAXP_L2_CNTR_CONF(0), 0xf0000 | (4 << 2));
	L2_WRITE(ARMADAXP_L2_CNTR_CONF(1), 0xf0000 | (2 << 2));
	L2_WRITE(ARMADAXP_L2_CNTR_CTRL, 0x303);
#endif

	return (0);
}

void
armadaxp_sdcache_inv_all(void)
{
	L2_WRITE(ARMADAXP_L2_INV_WAY, L2_ALL_WAYS);
}

void
armadaxp_sdcache_wb_all(void)
{
	L2_WRITE(ARMADAXP_L2_WB_WAY, L2_ALL_WAYS);
	L2_WRITE(ARMADAXP_L2_SYNC, 0);
	__asm__ __volatile__("dsb");
}

void
armadaxp_sdcache_wbinv_all(void)
{
	L2_WRITE(ARMADAXP_L2_WBINV_WAY, L2_ALL_WAYS);
	L2_WRITE(ARMADAXP_L2_SYNC, 0);
	__asm__ __volatile__("dsb");
}

static paddr_t
armadaxp_sdcache_wbalign_base(vaddr_t va, paddr_t pa, psize_t sz)
{
	paddr_t line_start = pa & ~ARMADAXP_L2_ALIGN;
	vaddr_t save_start;
	uint8_t save_buf[ARMADAXP_L2_LINE_SIZE];
	size_t unalign;

	unalign = va & ARMADAXP_L2_ALIGN;
	if (unalign == 0)
		return line_start;  /* request is aligned to cache line size */

	/* save data that is not intended to invalidate */
	save_start = va & ~ARMADAXP_L2_ALIGN;
	memcpy(save_buf, (void *)save_start, unalign);

	/* invalidate include saved data */
	L2_WRITE(ARMADAXP_L2_INV_PHYS, line_start);

	/* write back saved data */
	memcpy((void *)save_start, save_buf, unalign);
	L2_WRITE(ARMADAXP_L2_WB_PHYS, line_start);
	L2_WRITE(ARMADAXP_L2_SYNC, 0);
	__asm__ __volatile__("dsb");

	return line_start;
}

static paddr_t
armadaxp_sdcache_wbalign_end(vaddr_t va, paddr_t pa, psize_t sz)
{
	paddr_t line_start = (pa + sz - 1) & ~ARMADAXP_L2_ALIGN;
	vaddr_t save_start = va + sz;
	uint8_t save_buf[ARMADAXP_L2_LINE_SIZE];
	size_t save_len;
	size_t unalign;

	unalign = save_start & ARMADAXP_L2_ALIGN;
	if (unalign == 0)
		return line_start; /* request is aligned to cache line size */

	/* save data that is not intended to invalidate */
	save_len = ARMADAXP_L2_LINE_SIZE - unalign;
	memcpy(save_buf, (void *)save_start, save_len);

	/* invalidate include saved data */
	L2_WRITE(ARMADAXP_L2_INV_PHYS, line_start);

	/* write back saved data */
	memcpy((void *)save_start, save_buf, save_len);
	L2_WRITE(ARMADAXP_L2_WB_PHYS, line_start);
	__asm__ __volatile__("dsb");

	return line_start;
}

void
armadaxp_sdcache_inv_range(vaddr_t va, paddr_t pa, psize_t sz)
{
	paddr_t pa_base;
	paddr_t pa_end;

	/* align and write-back the boundary */
	pa_base = armadaxp_sdcache_wbalign_base(va, pa, sz);
	pa_end = armadaxp_sdcache_wbalign_end(va, pa, sz);

	/* invalidate other cache */
	if (pa_base == pa_end) {
		L2_WRITE(ARMADAXP_L2_INV_PHYS, pa_base);
		return;
	}

	L2_WRITE(ARMADAXP_L2_RANGE_BASE, pa_base);
	L2_WRITE(ARMADAXP_L2_INV_RANGE, pa_end);
}

void
armadaxp_sdcache_wb_range(vaddr_t va, paddr_t pa, psize_t sz)
{
	paddr_t pa_base = pa & ~ARMADAXP_L2_ALIGN;
	paddr_t pa_end  = (pa + sz - 1) & ~ARMADAXP_L2_ALIGN;

	if (pa_base == pa_end)
		L2_WRITE(ARMADAXP_L2_WB_PHYS, pa_base);
	else {
		L2_WRITE(ARMADAXP_L2_RANGE_BASE, pa_base);
		L2_WRITE(ARMADAXP_L2_WB_RANGE, pa_end);
	}
	L2_WRITE(ARMADAXP_L2_SYNC, 0);
	__asm__ __volatile__("dsb");
}

void
armadaxp_sdcache_wbinv_range(vaddr_t va, paddr_t pa, psize_t sz)
{
	paddr_t pa_base = pa & ~ARMADAXP_L2_ALIGN;;
	paddr_t pa_end  = (pa + sz - 1) & ~ARMADAXP_L2_ALIGN;

	if (pa_base == pa_end)
		L2_WRITE(ARMADAXP_L2_WBINV_PHYS, pa_base);
	else {
		L2_WRITE(ARMADAXP_L2_RANGE_BASE, pa_base);
		L2_WRITE(ARMADAXP_L2_WBINV_RANGE, pa_end);
	}
	L2_WRITE(ARMADAXP_L2_SYNC, 0);
	__asm__ __volatile__("dsb");
}

#ifdef AURORA_IO_CACHE_COHERENCY
static void
armadaxp_io_coherency_init(void)
{
	uint32_t reg;

	/* set CIB read snoop command to ReadUnique */
	reg = read_mlmbreg(MVSOC_MLMB_CIB_CTRL_CFG);
	reg |= MVSOC_MLMB_CIB_CTRL_CFG_WB_EN;
	write_mlmbreg(MVSOC_MLMB_CIB_CTRL_CFG, reg);

	/* enable CPUs in SMP group on Fabric coherency */
	reg = read_mlmbreg(MVSOC_MLMB_CFU_FAB_CTRL);
	reg |= MVSOC_MLMB_CFU_FAB_CTRL_SNOOP_CPU0;
	write_mlmbreg(MVSOC_MLMB_CFU_FAB_CTRL, reg);

	/* send all snoop request to L2 cache */
	reg = read_mlmbreg(MVSOC_MLMB_CFU_CFG);
#ifdef L2CACHE_ENABLE
	reg |= MVSOC_MLMB_CFU_CFG_L2_NOTIFY;
#else
	reg &= ~MVSOC_MLMB_CFU_CFG_L2_NOTIFY;
#endif
	write_mlmbreg(MVSOC_MLMB_CFU_CFG, reg);

	/* Mark as enabled */
	iocc_state = 1;
}
#endif

static int
armadaxp_clkgating(struct marvell_attach_args *mva)
{
	uint32_t val;
	int i;

	for (i = 0; i < __arraycount(clkgatings); i++) {
		if (clkgatings[i].offset == mva->mva_offset) {
			val = read_miscreg(ARMADAXP_MISC_PMCGC);
			if ((val & clkgatings[i].bits) == clkgatings[i].bits)
				/* Clock enabled */
				return 0;
			return 1;
		}
	}
	/* Clock Gating not support */
	return 0;
}

int
armadaxp_init_mbus(void)
{
	struct mbus_table_def *def;
	uint32_t reg;
	int i;

	for (i = 0; i < nwindow; i++) {
		/* disable all windows */
		reg = read_mlmbreg(MVSOC_MLMB_WCR(i));
		reg &= ~MVSOC_MLMB_WCR_WINEN;
		write_mlmbreg(MVSOC_MLMB_WCR(i), reg);
		write_mlmbreg(MVSOC_MLMB_WBR(i), 0);
	}

	for (i = 0; i < __arraycount(mbus_table); i++) {
		def = &mbus_table[i];
		if (def->window >= nwindow)
			continue;
		if (def->size == 0)
			continue;

		/* restore window base */
		reg = def->base & MVSOC_MLMB_WBR_BASE_MASK;
		write_mlmbreg(MVSOC_MLMB_WBR(def->window), reg);

		/* restore window configuration */
		reg  = MVSOC_MLMB_WCR_SIZE(def->size);
		reg |= MVSOC_MLMB_WCR_TARGET(def->target);
		reg |= MVSOC_MLMB_WCR_ATTR(def->attr);
#ifdef AURORA_IO_CACHE_COHERENCY
		reg |= MVSOC_MLMB_WCR_SYNC; /* enbale I/O coherency barrior */
#endif
		reg |= MVSOC_MLMB_WCR_WINEN;
		write_mlmbreg(MVSOC_MLMB_WCR(def->window), reg);
	}

	return 0;
}

int
armadaxp_attr_dump(struct mvsoc_softc *sc, uint32_t target, uint32_t attr)
{
	struct mbus_description *desc;
	int i;

	for (i = 0; i < __arraycount(mbus_desc); i++) {
		desc = &mbus_desc[i];
		if (desc->target != target)
			continue;
		if (desc->attr != attr)
			continue;
		aprint_verbose_dev(sc->sc_dev, "%s", desc->string);
		return 0;
	}

	/* unknown decoding target/attribute pair */
	aprint_verbose_dev(sc->sc_dev, "target 0x%x(attr 0x%x)", target, attr);
	return -1;
}
