/*	$NetBSD: gtpcireg.h,v 1.1 2003/03/05 22:08:22 matt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_GTPCIREG_H
#define	_DEV_GTPCIREG_H

#define PCI__BIT(bit)			(1U << (bit))
#define PCI__MASK(bit)			(PCI__BIT(bit) - 1)
#define PCI__GEN(bus, off, num)		(((off)^((bus) << 7))+((num) << 4))
#define	PCI__EXT(data, bit, len)	(((data) >> (bit)) & PCI__MASK(len))
#define	PCI__CLR(data, bit, len)	((data) &= ~(PCI__MASK(len) << (bit)))
#define	PCI__INS(bit, new)		((new) << (bit))

#define	PCI_SYNC_REG(bus)		(0xc0 | ((bus) << 3))

/*
 * Table 185: PCI Slave ADDRess Decoding Register Map
 */
#define PCI_SCS0_BAR_SIZE(bus)				PCI__GEN(bus, 0x0c08, 0)
#define PCI_SCS2_BAR_SIZE(bus)				PCI__GEN(bus, 0x0c0c, 0)
#define PCI_CS0_BAR_SIZE(bus)				PCI__GEN(bus, 0x0c10, 0)
#define PCI_CS3_BAR_SIZE(bus)				PCI__GEN(bus, 0x0c14, 0)
#define PCI_SCS1_BAR_SIZE(bus)				PCI__GEN(bus, 0x0d08, 0)
#define PCI_SCS3_BAR_SIZE(bus)				PCI__GEN(bus, 0x0d0c, 0)
#define PCI_CS1_BAR_SIZE(bus)				PCI__GEN(bus, 0x0d10, 0)
#define PCI_BOOTCS_BAR_SIZE(bus)			PCI__GEN(bus, 0x0d14, 0)
#define PCI_CS2_BAR_SIZE(bus)				PCI__GEN(bus, 0x0d18, 0)
#define PCI_P2P_MEM0_BAR_SIZE(bus)			PCI__GEN(bus, 0x0d1c, 0)
#define PCI_P2P_MEM1_BAR_SIZE(bus)			PCI__GEN(bus, 0x0d20, 0)
#define PCI_P2P_IO_BAR_SIZE(bus)			PCI__GEN(bus, 0x0d24, 0)
#define PCI_CPU_BAR_SIZE(bus)				PCI__GEN(bus, 0x0d28, 0)
#define PCI_EXPANSION_ROM_BAR_SIZE(bus)			PCI__GEN(bus, 0x0d2c, 0)
#define PCI_DAC_SCS0_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e00, 0)
#define PCI_DAC_SCS1_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e04, 0)
#define PCI_DAC_SCS2_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e08, 0)
#define PCI_DAC_SCS3_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e0c, 0)
#define PCI_DAC_CS0_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e10, 0)
#define PCI_DAC_CS1_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e14, 0)
#define PCI_DAC_CS2_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e18, 0)
#define PCI_DAC_CS3_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e1c, 0)
#define PCI_DAC_BOOTCS_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e20, 0)
#define PCI_DAC_P2P_MEM0_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e24, 0)
#define PCI_DAC_P2P_MEM1_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e28, 0)
#define PCI_DAC_CPU_BAR_SIZE(bus)			PCI__GEN(bus, 0x0e2c, 0)
#define PCI_BASE_ADDR_REGISTERS_ENABLE(bus)		PCI__GEN(bus, 0x0c3c, 0)
#define PCI_SCS0_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0c48, 0)
#define PCI_SCS1_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d48, 0)
#define PCI_SCS2_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0c4c, 0)
#define PCI_SCS3_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d4c, 0)
#define PCI_CS0_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0c50, 0)
#define PCI_CS1_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d50, 0)
#define PCI_CS2_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d58, 0)
#define PCI_CS3_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0c54, 0)
#define PCI_ADDR_DECODE_CONTROL(bus)			PCI__GEN(bus, 0x0d3c, 0)
#define PCI_BOOTCS_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d54, 0)
#define PCI_P2P_MEM0_BASE_ADDR_REMAP_LOW(bus)		PCI__GEN(bus, 0x0d5c, 0)
#define PCI_P2P_MEM0_BASE_ADDR_REMAP_HIGH(bus)		PCI__GEN(bus, 0x0d60, 0)
#define PCI_P2P_MEM1_BASE_ADDR_REMAP_LOW(bus)		PCI__GEN(bus, 0x0d64, 0)
#define PCI_P2P_MEM1_BASE_ADDR_REMAP_HIGH(bus)		PCI__GEN(bus, 0x0d68, 0)
#define PCI_P2P_IO_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d6c, 0)
#define PCI_CPU_BASE_ADDR_REMAP(bus)			PCI__GEN(bus, 0x0d70, 0)
#define PCI_DAC_SCS0_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f00, 0)
#define PCI_DAC_SCS1_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f04, 0)
#define PCI_DAC_SCS2_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f08, 0)
#define PCI_DAC_SCS3_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f0c, 0)
#define PCI_DAC_CS0_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f10, 0)
#define PCI_DAC_CS1_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f14, 0)
#define PCI_DAC_CS2_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f18, 0)
#define PCI_DAC_CS3_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f1c, 0)
#define PCI_DAC_BOOTCS_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f20, 0)
#define PCI_DAC_P2P_MEM0_BASE_ADDR_REMAP_LOW(bus)	PCI__GEN(bus, 0x0f24, 0)
#define PCI_DAC_P2P_MEM0_BASE_ADDR_REMAP_HIGH(bus)	PCI__GEN(bus, 0x0f28, 0)
#define PCI_DAC_P2P_MEM1_BASE_ADDR_REMAP_LOW(bus)	PCI__GEN(bus, 0x0f2c, 0)
#define PCI_DAC_P2P_MEM1_BASE_ADDR_REMAP_HIGH(bus)	PCI__GEN(bus, 0x0f30, 0)
#define PCI_DAC_CPU_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f34, 0)
#define PCI_EXPANSION_ROM_BASE_ADDR_REMAP(bus)		PCI__GEN(bus, 0x0f38, 0)

/*
 * Table 186: PCI Control Register Map
 */
#define PCI_COMMAND(bus)				PCI__GEN(bus, 0x0c00, 0)
#define PCI_MODE(bus)					PCI__GEN(bus, 0x0d00, 0)
#define PCI_TIMEOUT_RETRY(bus)				PCI__GEN(bus, 0x0c04, 0)
#define PCI_READ_BUFFER_DISCARD_TIMER(bus)		PCI__GEN(bus, 0x0d04, 0)
#define PCI_MSI_TRIGGER_TIMER(bus)			PCI__GEN(bus, 0x0c38, 0)
#define PCI_ARBITER_CONTROL(bus)			PCI__GEN(bus, 0x1d00, 0)
#define PCI_INTERFACE_XBAR_CONTROL_LOW(bus)		PCI__GEN(bus, 0x1d08, 0)
#define PCI_INTERFACE_XBAR_CONTROL_HIGH(bus)		PCI__GEN(bus, 0x1d0c, 0)
#define PCI_INTERFACE_XBAR_TIMEOUT(bus)			PCI__GEN(bus, 0x1d04, 0)
#define PCI_READ_RESPONSE_XBAR_CONTROL_LOW(bus)		PCI__GEN(bus, 0x1d18, 0)
#define PCI_READ_RESPONSE_XBAR_CONTROL_HIGH(bus)	PCI__GEN(bus, 0x1d1c, 0)
#define PCI_SYNC_BARRIER(bus)				PCI__GEN(bus, 0x1d10, 0)
#define PCI_P2P_CONFIGURATION(bus)			PCI__GEN(bus, 0x1d14, 0)
#define PCI_P2P_SWAP_CONTROL(bus)			PCI__GEN(bus, 0x1d54, 0)
#define PCI_ACCESS_CONTROL_BASE_LOW(bus, n)		PCI__GEN(bus, 0x1e00, n)
#define PCI_ACCESS_CONTROL_BASE_HIGH(bus, n)		PCI__GEN(bus, 0x1e04, n)
#define PCI_ACCESS_CONTROL_TOP(bus, n)			PCI__GEN(bus, 0x1e08, n)


/*
 * Table 187: PCI Snoop Control Register Map
 */
#define PCI_SNOOP_CONTROL_BASE_LOW(bus, n)		PCI__GEN(bus, 0x1f00, n)
#define PCI_SNOOP_CONTROL_BASE_HIGH(bus, n)		PCI__GEN(bus, 0x1f04, n)
#define PCI_SNOOP_CONTROL_TOP(bus, n)			PCI__GEN(bus, 0x1f08, n)

/*
 * Table 188: PCI Configuration ACCESS_Register Map
 */
#define PCI_CONFIG_ADDR(bus)				PCI__GEN(bus, 0x0cf8, 0)
#define PCI_CONFIG_DATA(bus)				PCI__GEN(bus, 0x0cfc, 0)
#define PCI_INTR_ACK(bus)				PCI__GEN(bus, 0x0c34, 0)

/*
 * Table 189: PCI ERROR Report Register Map
 */
#define PCI_SERR_MASK(bus)				PCI__GEN(bus, 0x0c28, 0)
#define PCI_ERROR_ADDRESS_LOW(bus)			PCI__GEN(bus, 0x1d40, 0)
#define PCI_ERROR_ADDRESS_HIGH(bus)			PCI__GEN(bus, 0x1d44, 0)
#define PCI_ERROR_DATA_LOW(bus)				PCI__GEN(bus, 0x1d48, 0)
#define PCI_ERROR_DATA_HIGH(bus)			PCI__GEN(bus, 0x1d4c, 0)
#define PCI_ERROR_COMMAND(bus)				PCI__GEN(bus, 0x1d50, 0)
#define PCI_ERROR_CAUSE(bus)				PCI__GEN(bus, 0x1d58, 0)
#define PCI_ERROR_MASK(bus)				PCI__GEN(bus, 0x1d5c, 0)



/*
 * Table 223: PCI Base Address Registers Enable
 * If a bit is clear, the BAR is enabled.  If set, disabled.  The GT64260]
 * prevents disabling both memory mapped and I/O mapped BARs (bits 9 and 10
 * cannot simultaneously be set to 1).
 */
#define PCI_BARE_SCS0En		PCI__BIT(0)	/* SCS[0]* BAR Enable */
#define PCI_BARE_SCS1En		PCI__BIT(1)	/* SCS[1]* BAR Enable */
#define PCI_BARE_SCS2En		PCI__BIT(2)	/* SCS[2]* BAR Enable */
#define PCI_BARE_SCS3En		PCI__BIT(3)	/* SCS[3]* BAR Enable */
#define PCI_BARE_CS0En		PCI__BIT(4)	/* CS[0]* BAR Enable */
#define PCI_BARE_CS1En		PCI__BIT(5)	/* CS[1]* BAR Enable */
#define PCI_BARE_CS2En		PCI__BIT(6)	/* CS[2]* BAR Enable */
#define PCI_BARE_CS3En		PCI__BIT(7)	/* CS[3]* BAR Enable */
#define PCI_BARE_BootCSEn	PCI__BIT(8)	/* BootCS* BAR Enable */
#define PCI_BARE_IntMemEn	PCI__BIT(9)	/* Memory Mapped Internal
						 * Registers BAR Enable */
#define PCI_BARE_IntIOEn	PCI__BIT(10)	/* I/O Mapped Internal
						 * Registers BAR Enable */
#define PCI_BARE_P2PMem0En	PCI__BIT(11)	/* P2P Mem0 BAR Enable */
#define PCI_BARE_P2PMem1En	PCI__BIT(12)	/* P2P Mem1 BAR Enable */
#define PCI_BARE_P2PIOEn	PCI__BIT(13)	/* P2P IO BAR Enable */
#define PCI_BARE_CPUEn		PCI__BIT(14)	/* CPU BAR Enable */
#define PCI_BARE_DSCS0En	PCI__BIT(15)	/* DAC SCS[0]* BAR Enable */
#define PCI_BARE_DSCS1En	PCI__BIT(16)	/* DAC SCS[1]* BAR Enable */
#define PCI_BARE_DSCS2En	PCI__BIT(17)	/* DAC SCS[2]* BAR Enable */
#define PCI_BARE_DSCS3En	PCI__BIT(18)	/* DAC SCS[3]* BAR Enable */
#define PCI_BARE_DCS0En		PCI__BIT(19)	/* DAC CS[0]* BAR Enable */
#define PCI_BARE_DCS1En		PCI__BIT(20)	/* DAC CS[1]* BAR Enable */
#define PCI_BARE_DCS2En		PCI__BIT(21)	/* DAC CS[2]* BAR Enable */
#define PCI_BARE_DCS3En		PCI__BIT(22)	/* DAC CS[3]* BAR Enable */
#define PCI_BARE_DBootCSEn	PCI__BIT(23)	/* DAC BootCS* BAR Enable */
#define PCI_BARE_DP2PMem0En	PCI__BIT(24)	/* DAC P2P Mem0 BAR Enable */
#define PCI_BARE_DP2PMem1En	PCI__BIT(25)	/* DAC P2P Mem1 BAR Enable */
#define PCI_BARE_DCPUEn		PCI__BIT(26)	/* DAC CPU BAR Enable */

/*
 * Table 254: PCI Address Decode Control
 * Bits 7:4 and 31:25 are reserved
 * 00:00 RemapWrDis		Address Remap Registers Write Disable
 * 				0: Writes to a BAR result in updating the
 * 				   corresponding remap register with the BAR's
 * 				   new value.
 * 				1: Writes to a BAR have no affect on the
 * 				   corresponding Remap register value.
 * 01:01 ExpRomDev		Expansion ROM Device (0: CS[3]; 1: BootCS)
 * 02:02 VPDDev			VPD Device (0: CS[3]; 1: BootCS)
 * 03:03 MsgAcc			Messaging registers access
 * 				0: Messaging unit registers are accessible on
 * 				   lowest 4Kbyte of SCS[0] BAR space.
 * 				1: Messaging unit registers are only accessible
 * 				   as part of the GT64260 internal space.
 * 07:04 Reserved
 * 24:08 VPDHighAddr		VPD High Address bits
 * 				[31:15] of VPD the address.
 * 31:25 Reserved
 */
#define	PCI_ADC_RemapWrDis		PCI__BIT(0)
#define PCI_ADC_ExpRomDev		PCI__BIT(1)
#define PCI_ADC_VPDDev			PCI__BIT(2)
#define PCI_ADC_MsgAcc			PCI__BIT(3)
#define	PCI_ADC_VPDHighAddr_GET(v)	PCI__EXT(v, 8, 16)


/*
 * Table 255: PCI Command
 * 00:00 MByteSwap	PCI Master Byte Swap
 *				NOTE: GT-64120 and GT-64130 compatible.
 *			When set to 0, the GTO64260 PCI master swaps the bytes
 *			of the incoming and outgoing PCI data (swap the 8 bytes
 *			of a longword).
 * 01:01 Reserved
 * 02:02 Reserved	Must be 0.
 * 03:03 Reserved
 * 04:04 MWrCom		PCI Master Write Combine Enable
 *			When set to 1, write combining is enabled.
 * 05:05 MRdCom		PCI Master Read Combine Enable
 *			When set to 1, read combining is enabled.
 * 06:06 MWrTrig	PCI Master Write Trigger
 *			0: Accesses the PCI bus only when the whole burst is
 *			   written into the master write buffer.
 *			1: Accesses the PCI bus when the first data is written
 *			   into the master write buffer.
 * 07:07 MRdTrig	PCI Master Read Trigger
 *			0: Returns read data to the initiating unit only when
 *			   the whole burst is written into master read buffer.
 *			1: Returns read data to the initiating unit when the
 *			   first read data is written into master read buffer.
 * 08:08 MRdLine	PCI Master Memory Read Line Enable
 *			(0: Disable; 1: Enable)
 * 09:09 MRdMul		PCI Master Memory Read Multiple Enable
 *			(0: Disable; 1: Enable)
 * 10:10 MWordSwap	PCI Master Word Swap
 *				NOTE: GT-64120 and GT-64130 compatible.
 *			When set to 1, the GT64260 PCI master swaps the 32-bit
 *			words of the incoming and outgoing PCI data.
 * 11:11 SWordSwap	PCI Slave Word Swap
 *				NOTE: GT-64120 and GT-64130 compatible.
 *			When set to 1, the GT64260 PCI slave swaps the 32-bit
 *			words of the incoming and outgoing PCI data.
 * 12:12 IntBusCtl	PCI Interface Unit Internal Bus Control
 *				NOTE: Reserved for Galileo Technology usage
 *			0: Enable internal bus sharing between master and
 *			   slave interfaces.
 *			1: Disable internal bus sharing between master and
 *			   slave interfaces.
 * 13:13 SBDis		PCI Slave Sync Barrier Disable
 *			When set to 1, the PCI configuration read transaction
 *			will stop act as sync barrier transaction.
 * 14:14 Reserved	Must be 0
 * 15:15 MReq64		PCI Master REQ64* Enable (0: Disable; 1: Enable)
 * 16:16 SByteSwap	PCI Slave Byte Swap
 *				NOTE: GT-64120 and GT-64130 compatible.
 *			When set to 0, the GT64260 PCI slave swaps the bytes of
 *			the incoming and outgoing PCI data (swap the 8 bytes of
 *			a long-word).
 * 17:17 MDACEn		PCI Master DAC Enable
 *			0: Disable (The PCI master never drives the DAC cycle)
 *			1: Enable (In case the upper 32-bit address is not 0,
 *			   the PCI master drives the DAC cycle)
 * 18:18 M64Allign	PCI Master REQ64* assertion on non-aligned
 *			0: Disable (The master asserts REQ64* only if
 *			   the address is 64-bit aligned)
 *			1: Enable (The master asserts REQ64* even if
 *			   the address is not 64-bit aligned)
 * 19:19 PErrProp	Parity/ECC Errors Propagation Enable
 *			0: Disable (The PCI interface always drives
 *			   correct parity on the PAR signal)
 *			1: Enable (In case of slave read bad ECC from
 *			   SDRAM, or master write with bad parity/ECC
 *			   indication from the initiator, the PCI interface
 *			   drives bad parity on the PAR signal)
 * 20:20 SSwapEn	PCI Slave Swap Enable
 *				NOTE: Even if the SSwapEn bit is set to 1 and
 *				the PCI address does not match any of the
 *				Access Control registers, slave data swapping
 *				works according to SByteSwap and SWordSwap bits.
 *			0: PCI slave data swapping is determined via
 *			   SByteSwap and SWordSwap bits (bits 16 and 11),
 *			   as in the GT-64120/130.
 *			1: PCI slave data swapping is determined via PCISwap
 *			   bits [25:24] in the PCI Access Control registers.
 * 21:21 MSwapEn	PCI Master Swap Enable
 *			0: PCI master data swapping is determined via
 *			   MByteSwap and MWordSwap bits (bits 0 and 10),
 *			   as in the GT-64120/130.
 *			1: PCI master data swapping is determined via
 *			   PCISwap bits in CPU to PCI Address Decoding
 *			   registers.
 * 22:22 MIntSwapEn	PCI Master Configuration Transactions Data Swap Enable
 *				NOTE: Reserved for Galileo Technology usage.
 *			0: Disable (The PCI master configuration transaction
 *			   to the PCI bus is always in Little Endian convention)
 *			1: Enable (The PCI master configuration transaction to
 *			   the PCI bus is determined according to the setting
 *			   of MSwapEn bit)
 * 23:23 LBEn		PCI Loop Back Enable
 *				NOTE: Reserved for Galileo Technology usage.
 *			0: Disable (The PCI slave does not respond to
 *			   transactions initiated by the PCI master)
 *			1: Enable (The PCI slave does respond to
 *			   transactions initiated by the PCI master,
 *			   if targeted to the slave (address match)
 * 26:24 SIntSwap	PCI Slave data swap control on PCI accesses to the
 *			GT64260 internal and configuration registers.
 *			Bits encoding are the same as bits[26:24] in PCI Access
 *			Control registers.
 * 27:27 Reserved	Must be 0.
 * 31:28 Reserved	Read only.
 */
#define	PCI_CMD_MByteSwap	PCI__BIT(0)
#define	PCI_CMD_MBZ0_2		PCI__BIT(2)
#define	PCI_CMD_MWrCom		PCI__BIT(4)
#define	PCI_CMD_MRdCom		PCI__BIT(5)
#define	PCI_CMD_MWrTrig		PCI__BIT(6)
#define	PCI_CMD_MRdTrig		PCI__BIT(7)
#define	PCI_CMD_MRdLine		PCI__BIT(8)
#define	PCI_CMD_MRdMul		PCI__BIT(9)
#define	PCI_CMD_MWordSwap	PCI__BIT(10)
#define	PCI_CMD_SWordSwap	PCI__BIT(11)
#define	PCI_CMD_IntBusCtl	PCI__BIT(12)
#define	PCI_CMD_SBDis		PCI__BIT(13)
#define	PCI_CMD_MBZ0_14		PCI__BIT(14)
#define	PCI_CMD_MReq64		PCI__BIT(15)
#define	PCI_CMD_SByteSwap	PCI__BIT(16)
#define	PCI_CMD_MDCAEn		PCI__BIT(17)
#define	PCI_CMD_M64Allign	PCI__BIT(18)
#define	PCI_CMD_PErrProp	PCI__BIT(19)
#define	PCI_CMD_SSwapEn		PCI__BIT(20)
#define	PCI_CMD_MSwapEn		PCI__BIT(21)
#define	PCI_CMD_MIntSwapEn	PCI__BIT(22)
#define	PCI_CMD_LBEn		PCI__BIT(23)
#define	PCI_CMD_SIntSwap_GET(v)	PCI__EXT(v, 24, 3)
#define	PCI_CMD_MBZ0_27		PCI__BIT(27)


/*
 * Table 256: PCI Mode
 * 00:00 PciID		PCI Interface ID -- Read Only (PCI_0: 0x0; PCI_1: 0x1)
 * 01:01 Reserved
 * 02:02 Pci64		64-bit PCI Interface -- Read Only
 *			When set to 1, the PCI interface is configured to a
 *			64 bit interface.
 * 07:03 Reserved
 * 08:08 ExpRom		Expansion ROM Enable -- Read Only from PCI
 *			When set to 1, the expansion ROM BAR is enabled.
 * 09:09 VPD		VPD Enable -- Read Only from PCI
 *			When set to 1, VPD is supported.
 * 10:10 MSI		MSI Enable -- Read Only from PCI
 *			When set to 1, MSI is supported.
 * 11:11 PMG		Power Management Enable -- Read Only from PCI
 *			When set to 1, PMG is supported.
 * 12:12 HotSwap	CompactPCI Hot Swap Enable -- Read Only from PCI
 *			When set to 1, HotSwap is supported.
 * 13:13 BIST		BIST Enable -- Read only from PCI
 *			If set to 1, BIST is enabled.
 * 30:14 Reserved
 * 31:31 PRst		PCI Interface Reset Indication -- Read Only
 *			Set to 0 as long as the RST* pin is asserted.
 */
#define	PCI_MODE_PciID_GET(v)	PCI__EXT(v, 0, 1)
#define	PCI_MODE_Pci64		PCI__BIT(2)
#define	PCI_MODE_ExpRom		PCI__BIT(8)
#define	PCI_MODE_VPD		PCI__BIT(9)
#define	PCI_MODE_MSI		PCI__BIT(10)
#define	PCI_MODE_PMG		PCI__BIT(11)
#define	PCI_MODE_HotSwap	PCI__BIT(12)
#define	PCI_MODE_BIST		PCI__BIT(13)
#define	PCI_MODE_PRst		PCI__BIT(31)

/*
 * Table 257: PCI Timeout and Retry
 * 07:00 Timeout0	Specifies the number of PClk cycles the GT64260 slave
 *			holds the PCI bus before terminating a transaction
 *			with RETRY.
 * 15:08 Timeout1	Specifies the number of PClk cycles the GT64260 slave
 *			holds the PCI bus before terminating a transaction
 *			with DISCONNECT.
 * 23:16 RetryCtr	Retry Counter
 *			Specifies the number of retries of the GT64260 Master.
 *			The GT64260 generates an interrupt when this timer
 *			expires.  A 0x00 value means a retry forever.
 * 31:24 Reserved
 */
#define	PCI_TMORTRY_Timeout0_GET(v)	PCI__EXT(v, 0, 8)
#define	PCI_TMORTRY_Timeout1_GET(v)	PCI__EXT(v, 8, 8)
#define	PCI_TMORTRY_RetryCtr_GET(v)	PCI__EXT(v, 16, 8)


/*
 * Table 258: PCI Read Buffer Discard Timer
 * 15:00 Timer		Specifies the number of PClk cycles the GT64260
 *			slave keeps an non-accessed read buffers (non com-
 *			pleted delayed read) before invalidating the buffer.
 * 23:16 RdBufEn	Slave Read Buffers Enable
 *			Each bit corresponds to one of the eight read buffers.
 *			If set to 1, buffer is enabled.
 * 31:24 Reserved
 */
#define	PCI_RdBufDisTmr_Timer_GET(v)	PCI__EXT(v, 0, 16)
#define	PCI_RdBufDisTmr_RdBufEn_GET(v)	PCI__EXT(v, 16, 8)
#define	PCI_RdBufDisTmr_RdBufEn0(v)	PCI__BIT(16)
#define	PCI_RdBufDisTmr_RdBufEn1(v)	PCI__BIT(17)
#define	PCI_RdBufDisTmr_RdBufEn2(v)	PCI__BIT(18)
#define	PCI_RdBufDisTmr_RdBufEn3(v)	PCI__BIT(19)
#define	PCI_RdBufDisTmr_RdBufEn4(v)	PCI__BIT(20)
#define	PCI_RdBufDisTmr_RdBufEn5(v)	PCI__BIT(21)
#define	PCI_RdBufDisTmr_RdBufEn6(v)	PCI__BIT(22)
#define	PCI_RdBufDisTmr_RdBufEn7(v)	PCI__BIT(23)

/*
 * Table 259: MSI Trigger Timer
 * 15:00 Timer		Specifies the number of TClk cycles between consecutive
 *			MSI requests.
 * 31:16 Reserved
 */
#define	PCI_MSITrigger_Timer_GET(v)	PCI__EXT(v, 0, 16)

/*
 * Table 260: PCI Arbiter Control
 *	NOTE:	If HPPV (bits [28:21]) is set to 0 and PAEn is set to 1,
 *		priority scheme is reversed. This means that high priority
 *		requests are granted if no low priority request is pending.
 * 00:00 Reserved	Must be 0. 0x0
 * 01:01 BDEn		Broken Detection Enable
 *			If set to 1, broken master detection is enabled. A mas-
 *			ter is said to be broken if it fails to respond to grant
 *			assertion within a window specified in BV (bits [6:3]).
 * 02:02 PAEn		Priority Arbitration Enable
 *			0: Low priority requests are granted only when no high
 *			   priority request is pending
 *			1: Weighted round robin arbitration is performed
 *			   between high priority and low priority groups.
 * 06:03 BV		Broken Value
 *			This value sets the maximum number of cycles that the
 *			arbiter waits for a PCI master to respond to its grant
 *			assertion. If a PCI master fails to assert FRAME* within
 *			this time, the PCI arbiter aborts the transaction and
 *			performs a new arbitration cycle and a maskable
 *			interrupt is generated. Must be greater than 0.
 *			NOTE:	The PCI arbiter waits for the current
 *				transaction to end before starting to
 *				count the wait-for-broken cycles.
 *			Must be greater than 1 for masters that performs address
 *			stepping (such as the GTO 64260 PCI master), since they
 *			require GNT* assertion for two cycles.
 * 13:07 P[6:0]		Priority
 *			These bits assign priority levels to the requests
 *			connected to the PCI arbiter. When a PM bit is set to
 *			1, priority of the associated request is high.  The
 *			mapping between P[6:0] bits and the request/grant pairs
 *			are as follows:
 *			 P[0]: internal PCI master	P[1]: external REQ0/GNT0
 *			 P[2]: external REQ1/GNT1	P[3]: external REQ2/GNT2
 *			 P[4]: external REQ3/GNT3	P[5]: external REQ4/GNT4
 *			 P[6]: external REQ5/GNT5
 * 20:14 PD[6:0]	Parking Disable
 *			Use these bits to disable parking on any of the PCI
 *			masters.  When a PD bit is set to 1, parking on the
 *			associated PCI master is disabled.
 *			NOTE:	The arbiter parks on the last master granted
 *				unless disabled through the PD bit. Also, if
 *				PD bits are all 1, the PCI arbiter parks on
 *				the internal PCI master.
 * 28:21 HPPV		High Priority Preset Value
 *			This is the preset value of the high priority counter
 *			(High_cnt). This counter decrements each time a high
 *			priority request is granted. When the counter reaches
 *			zero, it reloads with this preset value.  The counter
 *			reloads when a low priority request is granted.
 * 30:29 Reserved
 * 31:31 EN		Enable
 *			Setting this bit to 1 enables operation of the arbiter.
 */
#define	PCI_ARBCTL_MBZ0_0		PCI__BIT(0)
#define	PCI_ARBCTL_BDEn			PCI__BIT(1)
#define	PCI_ARBCTL_PAEn			PCI__BIT(2)
#define	PCI_ARBCTL_BV_GET(v)		PCI__EXT(v, 3, 4)
#define	PCI_ARBCTL_P_GET(v)		PCI__EXT(v, 7, 7)
#define	PCI_ARBCTL_PD_GET(v)		PCI__EXT(v, 14, 7)
#define	PCI_ARBCTL_HPPV_GET(v)		PCI__EXT(v, 21, 7)
#define	PCI_ARBCTL_EN			PCI__BIT(31)

#define	PCI_ARBPRI_IntPci		PCI__BIT(0)
#define	PCI_ARBPRI_ExtReqGnt0		PCI__BIT(1)
#define	PCI_ARBPRI_ExtReqGnt1		PCI__BIT(2)
#define	PCI_ARBPRI_EXtReqGnt2		PCI__BIT(3)
#define	PCI_ARBPRI_EXtReqGnt3		PCI__BIT(4)
#define	PCI_ARBPRI_EXtReqGnt4		PCI__BIT(5)
#define	PCI_ARBPRI_EXtReqGnt5		PCI__BIT(6)

/*
 * Table 261: PCI Interface Crossbar Control (Low)
 * 03:00 Arb0		Slice 0 of PCI master pizza arbiter.
 * 07:04 Arb1		Slice 1 of PCI master pizza arbiter.
 * 11:08 Arb2		Slice 2 of PCI master pizza arbiter.
 * 15:12 Arb3		Slice 3 of PCI master pizza arbiter.
 * 19:16 Arb4		Slice 4 of PCI master pizza arbiter.
 * 23:20 Arb5		Slice 5 of PCI master pizza arbiter.
 * 27:24 Arb6		Slice 6 of PCI master pizza arbiter.
 * 31:28 Arb7		Slice 7 of PCI master pizza arbiter.
 */
#define	PCI_IFXBRCTL_GET_SLICE(v, n)	PCI__EXT(v, (n) * 4, 4)
#define	PCI_IFXBRCTL_SET_SLICE(v, n, s)	((void)(PCI__CLR(v, (n)*4, 4),\
						(v) |= PCI__INS((n)*4, s)))

/*
 * Table 262: PCI Interface Crossbar Control (High)
 * 03:00 Arb8		Slice 8 of PCI master pizza arbiter.
 * 07:04 Arb9		Slice 9 of PCI master pizza arbiter.
 * 11:08 Arb10		Slice 10 of PCI master pizza arbiter.
 * 15:12 Arb11		Slice 11 of PCI master pizza arbiter.
 * 19:16 Arb12		Slice 12 of PCI master pizza arbiter.
 * 23:20 Arb13		Slice 13 of PCI master pizza arbiter.
 * 27:24 Arb14		Slice 14 of PCI master pizza arbiter.
 * 31:28 Arb15		Slice 15 of PCI master pizza arbiter.
 */
#define	PCI_IFXBRCH_GET_SLICE(v, n)	PCI__EXT(v, ((n) - 8) * 4, 4)
#define	PCI_IFXBRCH_SET_SLICE(v, n, s)	((void)(PCI__CLR(v, ((n)*-8)4, 4),\
						(v) |= PCI__INS(((n)-8)*4, s)))

/*
 * Table 263: PCI Interface Crossbar Timeout
		(NOTE: Reserved for Galileo Technology usage.)
 * 07:00 Timeout	Crossbar Arbiter Timeout Preset Value
 * 15:08 Reserved
 * 16:16 TimeoutEn	Crossbar Arbiter Timer Enable (1: Disable)
 * 31:17 Reserved
 */
#define PCI_IFXBRTMO_Timeout_GET(v)	PCI__EXT(v, 0, 8)
#define PCI_IFXBRTMO_TimeoutEn		PCI__BIT(16)

/*
 * Table 264: PCI Read Response Crossbar Control (Low)
 * 03:00 Arb0		Slice 0 of PCI slave pizza arbiter.
 * 07:04 Arb1		Slice 1 of PCI slave pizza arbiter.
 * 11:08 Arb2		Slice 2 of PCI slave pizza arbiter.
 * 15:12 Arb3		Slice 3 of PCI slave pizza arbiter.
 * 19:16 Arb4		Slice 4 of PCI slave pizza arbiter.
 * 23:20 Arb5		Slice 5 of PCI slave pizza arbiter.
 * 27:24 Arb6		Slice 6 of PCI slave pizza arbiter.
 * 31:28 Arb7		Slice 7 of PCI slave pizza arbiter.
 */
#define	PCI_RRXBRCL_GET_SLICE(v, n)	PCI__EXT(v, (n) * 4, 4)
#define	PCI_RRXBRCL_SET_SLICE(v, n, s)	((void)(PCI__CLR(v, (n)*4, 4),\
						(v) |= PCI__INS((n)*4, s)))


/*
 * Table 265: PCI Read Response Crossbar Control (High)
 * 03:00 Arb8		Slice 8 of PCI slave pizza arbiter.
 * 07:04 Arb9		Slice 9 of PCI slave pizza arbiter.
 * 11:08 Arb10		Slice 10 of PCI slave pizza arbiter.
 * 15:12 Arb11		Slice 11 of PCI slave pizza arbiter.
 * 19:16 Arb12		Slice 12 of PCI slave pizza arbiter.
 * 23:20 Arb13		Slice 13 of PCI slave pizza arbiter.
 * 27:24 Arb14		Slice 14 of PCI slave pizza arbiter.
 * 31:28 Arb15		Slice 15 of PCI slave pizza arbiter.
 */
#define	PCI_RRXBRCH_GET_SLICE(v, n)	PCI__EXT(v, ((n) - 8) * 4, 4)
#define	PCI_RRXBRCH_SET_SLICE(v, n, s)	((void)(PCI__CLR(v, ((n)*-8)4, 4),\
						(v) |= PCI__INS(((n)-8)*4, s)))

/*
 * Table 266: PCI Sync Barrier Virtual Register
 * 31:0 SyncReg Sync Barrier Virtual Register
 * PCI read from this register results in PCI slave sync barrier
 * action.  The returned data is un-deterministic.  Read Only.
 */

/*
 * Table 267: PCI P2P Configuration
 * 07:00 2ndBusL	Secondary PCI Interface Bus Range Lower Boundary
 * 15:08 2ndBusH	Secondary PCI Interface Bus Range Upper Boundary
 * 23:16 BusNum		The PCI bus number to which the PCI interface
 *			is connected.
 * 28:24 DevNum		The PCI interface's device number.
 * 31:29 Reserved	Reserved.
 */
#define	PCI_P2PCFG_2ndBusL_GET(v)	PCI_EXT(v,  0, 8)
#define	PCI_P2PCFG_2ndBusH_GET(v)	PCI_EXT(v,  8, 8)
#define	PCI_P2PCFG_BusNum_GET(v)	PCI_EXT(v, 16, 8)
#define	PCI_P2PCFG_DevNum_GET(v)	PCI_EXT(v, 24, 5)

/*
 * Table 268: PCI P2P Swap Control
 * 02:00 M0Sw		P2P Mem0 BAR Swap Control
 * 03:03 M0Req64	P2P Mem0 BAR Force REQ64
 * 06:04 M1Sw		P2P Mem1 BAR Swap Control
 * 07:07 M1Req64	P2P Mem1 BAR Force REQ64
 * 10:08 DM0Sw		P2P DAC Mem0 BAR Swap Control
 * 11:11 DM0Req64	P2P DAC Mem0 BAR Force REQ64
 * 14:12 DM1Sw		P2P DAC Mem1 BAR Swap Control
 * 15:15 DM1Req64	P2P DAC Mem1 BAR Force REQ64
 * 18:16 IOSw		P2P I/O BAR Swap Control
 * 19:19 Reserved
 * 22:20 CfgSw		P2P Configuration Swap Control
 * 31:19 Reserved
 */
#define	PCI_P2PSWAP_M0Sw_GET(v)		PCI__EXT(v, 0, 3)
#define	PCI_P2PSWAP_M0Req64		PCI__BIT(3)
#define	PCI_P2PSWAP_M1Sw_GET(v)		PCI__EXT(v, 4, 3)
#define	PCI_P2PSWAP_M1Req64		PCI__BIT(7)
#define	PCI_P2PSWAP_DM0Sw_GET(v)	PCI__EXT(v, 8, 3)
#define	PCI_P2PSWAP_DM0Req64		PCI__BIT(11)
#define	PCI_P2PSWAP_DM1Sw_GET(v)	PCI__EXT(v, 12, 3)
#define	PCI_P2PSWAP_DM1Req64		PCI__BIT(15)
#define	PCI_P2PSWAP_CfgSw_GET(v)	PCI__EXT(v, 20, 3)



/*
 * Table 269: PCI Access Control Base (Low)
 * 11:00 Addr		Base Address Corresponds to address bits[31:20].
 * 12:12 PrefetchEn	Read Prefetch Enable
 *			0: Prefetch disabled (The PCI slave reads single words)
 *			1: Prefetch enabled. 
 * 14:14 Reserved	 Must be 0
 * 15:15 Reserved
 * 16:16 RdPrefetch	PCI Read Aggressive Prefetch Enable; 0: Disable;
 *			1: Enable (The PCI slave prefetches two
 *			bursts in advance)
 * 17:17 RdLinePrefetch	PCI Read Line Aggressive Prefetch Enable; 0: Disable;
 *			1: Enable (PCI slave prefetch two bursts in advance)
 * 18:18 RdMulPrefetch	PCI Read Multiple Aggressive Prefetch Enable
 *			0: Disable; 1: Enable (PCI slave prefetch two bursts in
 *			advance)
 * 19:19 Reserved
 * 21:20 MBurst		PCI Max Burst
 *			Specifies the maximum burst size for a single transac-
 *			tion between a PCI slave and the other interfaces
 *				00 - 4 64-bit words
 *				01 - 8 64-bit words
 *				10 - 16 64-bit words
 *				11 - Reserved
 * 23:22 Reserved
 * 25:24 PCISwap	Data Swap Control
 *				00 - Byte Swap
 *				01 - No swapping
 *				10 - Both byte and word swap
 *				11 - Word swap
 * 26:26 Reserved	Must be 0
 * 27:27 Reserved
 * 28:28 AccProt	Access Protect (0: PCI access is allowed; 1; Region is
			not accessible from PCI)
 * 29:29 WrProt		Write Protect (0: PCI write is allowed; 1: Region is
 *			not writeable from PCI)
 * 31:30 Reserved
 */
#define	PCI_ACCCTLBASEL_Addr_GET(v)	PCI__EXT(v, 0, 12)
#define	PCI_ACCCTLBASEL_PrefetchEn	PCI__BIT(12)
#define PCI_ACCCTLBASEL_MBZ0_14		PCI__BIT(14)
#define PCI_ACCCTLBASEL_RdPrefetch	PCI__BIT(16)
#define PCI_ACCCTLBASEL_RdLinePrefetch	PCI__BIT(17)
#define PCI_ACCCTLBASEL_RdMulPrefetch	PCI__BIT(18)
#define PCI_ACCCTLBASEL_WBurst		PCI__EXT(v, 20, 2)
#define PCI_ACCCTLBASEL_PCISwap		PCI__EXT(v, 24, 2)
#define PCI_ACCCTLBASEL_MBZ0_26		PCI__BIT(26)
#define PCI_ACCCTLBASEL_AccProt		PCI__BIT(28)
#define PCI_ACCCTLBASEL_WrProt		PCI__BIT(29)

#define	PCI_WBURST_4_QW			0x00
#define	PCI_WBURST_8_QW			0x01
#define	PCI_WBURST_16_QW		0x02
#define	PCI_WBURST_Reserved		0x04

#define	PCI_PCISWAP_ByteSwap		0x00
#define	PCI_PCISWAP_NoSwap		0x01
#define	PCI_PCISWAP_ByteWordSwap	0x02
#define	PCI_PCISWAP_WordSwap		0x04

/*
 * Table 293: PCI Snoop Control Base (Low)
 * 11:00 Addr		Base Address	 Corresponds to address bits[31:20].
 * 13:12 Snoop		Snoop Type
 * 31:14 Reserved
 */
#define PCI_SNOOPCTL_ADDR(v)	PCI__EXT(v, 0, 12)
#define PCI_SNOOPCTL_TYPE(v)	PCI__EXT(v, 12, 2)

#define	PCI_SNOOP_None		0	/* no snoop */
#define	PCI_SNOOP_WT		1	/* Snoop to WT region */
#define	PCI_SNOOP_WB		2	/* Snoop to WB region */


/*
 * Table 305: PCI Configuration Address
 *
 * 07:02 RegNum		Register number.
 * 10:08 FunctNum	Function number.
 * 15:11 DevNum		Device number.
 * 23:16 BusNum		Bus number.
 * 31:31 ConfigEn	When set, an access to the Configuration Data
 *			register is translated into a Configuration
 *			or Special cycle on the PCI bus.
 */
#define	PCI_CFG_MAKE_TAG(bus, dev, fun, reg)	(PCI__BIT(31)|\
						 PCI__INS(16, (bus))|\
						 PCI__INS(11, (dev))|\
						 PCI__INS( 8, (fun))|\
						 PCI__INS( 0, (reg)))
#define	PCI_CFG_GET_BUSNO(tag)			PCI__EXT(tag, 16, 8)
#define	PCI_CFG_GET_DEVNO(tag)			PCI__EXT(tag, 11, 5)
#define	PCI_CFG_GET_FUNCNO(tag)			PCI__EXT(tag,  8, 3)
#define	PCI_CFG_GET_REGNO(tag)			PCI__EXT(tag,  0, 8)

/*
 * Table 306: PCI Configuration Data
 *
 * 31:00 ConfigData	The data is transferred to/from the PCI bus when
 *			the CPU accesses this register and the ConfigEn
 *			bit in the Configuration Address register is set
 *
 * A CPU access to this register causes the GT64260 to perform a Configuration
 * or Special cycle on the PCI bus.
 */


/*
 * Table 307: PCI Interrupt Acknowledge (This register is READ ONLY)
 * 31:00 IntAck		A CPU read access to this register forces an
 *			interrupt acknowledge cycle on the PCI bus.
 */


/*
 * Table 308: PCI SERR* Mask
 *
 * NOTE: The GT64260 asserts SERR* only if SERR* is enabled via the PCI Status
 *       and Command register.
 * If the corresponding bit is set, then asserts SERR* upon ...
 */
#define PCI_SERRMSK_SAPerr	PCI__BIT(0)	/* PCI slave detection of bad
						 * address parity. */
#define PCI_SERRMSK_SWrPerr	PCI__BIT(1)	/* PCI slave detection of bad
						 * write data parity. */
#define PCI_SERRMSK_SRdPerr	PCI__BIT(2)	/* a PERR* response to read
						 * data driven by the PCI
						 * slave. */
#define PCI_SERRMSK_MAPerr	PCI__BIT(4)	/* a PERR* response to an
						 * address driven by the PCI
						 * master. */
#define PCI_SERRMSK_MWrPerr	PCI__BIT(5)	/* a PERR* response to write
						 * data driven by the PCI
						 * master. */
#define PCI_SERRMSK_MRdPerr	PCI__BIT(6)	/* bad data parity detection
						 * during a PCI master read
						 * transaction. */
#define PCI_SERRMSK_MMabort	PCI__BIT(8)	/* a PCI master generation of
						 * master abort. */
#define PCI_SERRMSK_MTabort	PCI__BIT(9)	/* a PCI master detection of
						 * target abort. */
#define PCI_SERRMSK_MRetry	PCI__BIT(11)	/* a PCI master reaching retry
						 * counter limit. */
#define PCI_SERRMSK_SMabort	PCI__BIT(16)	/* a PCI slave detection of
						 * master abort. */
#define PCI_SERRMSK_STabort	PCI__BIT(17)	/* a PCI slave termination of
						 * a transaction with Target
						 * Abort. */
#define PCI_SERRMSK_SAccProt	PCI__BIT(18)	/* a PCI slave access protect
						 * violation. */
#define PCI_SERRMSK_SWrProt	PCI__BIT(19)	/* a PCI slave write protect
						 * violation. */
#define PCI_SERRMSK_SRdBuf	PCI__BIT(20)	/* the PCI slave's read buffer,
						 * discard timer expires */
#define PCI_SERRMSK_Arb		PCI__BIT(21)	/* the internal PCI arbiter
						 * detection of a broken PCI
						 * master. */

#define PCI_SERRMSK_ALL_ERRS \
	(PCI_SERRMSK_SAPerr|PCI_SERRMSK_SWrPerr|PCI_SERRMSK_SRdPerr \
	|PCI_SERRMSK_MAPerr|PCI_SERRMSK_MWrPerr|PCI_SERRMSK_MRdPerr \
	|PCI_SERRMSK_MMabort|PCI_SERRMSK_MTabort|PCI_SERRMSK_MRetry \
	|PCI_SERRMSK_SMabort|PCI_SERRMSK_STabort|PCI_SERRMSK_SAccProt \
	|PCI_SERRMSK_SWrProt|PCI_SERRMSK_SRdBuf|PCI_SERRMSK_Arb)
	


/*
 * Table 309: PCI Error Address (Low) -- Read Only.
 * 31:00 ErrAddr	PCI address bits [31:0] are latched upon an error
 *			condition.  Upon address latch, no new addresses can
 *			be registered (due to additional error condition) until
 *			the register is being read.
 */



/*
 * Table 310: PCI Error Address (High)	Applicable only when running DAC cycles.
 * 31:00 ErrAddr	PCI address bits [63:32] are latched upon
 *			error condition.
 *
 * NOTE: Upon data sample, no new data is latched until the PCI Error Low
 *       Address register is read. This means that PCI Error Low Address
 *       register must bethe last register read by the interrupt handler.
 */

/*
 * Table 311: PCI Error Data (Low)
 * 31:00 ErrData	PCI data bits [31:00] are latched upon error condition.
 */

/*
 * Table 312: PCI Error Data (High)	Applicable only when running
 *					64-bit cycles.
 * 31:00 ErrData	PCI data bits [63:32] are latched upon error condition.
 */

/*
 * Table 313: PCI Error Command
 * 03:00 ErrCmd		PCI command is latched upon error condition.
 * 07:04 Reserved
 * 15:08 ErrBE		PCI byte enable is latched upon error condition.
 * 16:16 ErrPAR		PCI PAR is latched upon error condition.
 * 17:17 ErrPAR64	PCI PAR64 is latched upon error condition.
 *			Applicable only when running 64-bit cycles.
 * 31:18 Reserved
 * NOTE: Upon data sample, no new data is latched until the PCI Error Low
 * Address register is read. This means that PCI Error Low Address register
 * must be the last register read by the interrupt handler.
 */
#define PCI_ERRCMD_Cmd_GET(v)		PCI__EXT(v, 0, 4)
#define PCI_ERRCMD_ByteEn_GET(v)	PCI__EXT(v, 8, 8)
#define	PCI_ERRCMD_PAR			PCI__BIT(16)
#define	PCI_ERRCMD_PAR64		PCI__BIT(17)

/*
 * Table 314: PCI Interrupt Cause
 * 1. All bits are Clear Only. A cause bit set upon error event occurrence.
 *    A write of 0 clears the bit. A write of 1 has no affect.
 * 2. PCI Interrupt bits are organized in four groups:
 *    bits[ 7: 0] for address and data parity errors,
 *    bits[15: 8] for PCI master transaction failure (possible external
 *		  target problem),
 *    bits[23:16] for slave response failure (possible external master problem),
 *    bits[26:24] for external PCI events that require CPU handle.
 */
#define PCI_IC_SAPerr		PCI__BIT(0)	/* The PCI slave detected
						 * bad address parity. */
#define PCI_IC_SWrPerr		PCI__BIT(1)	/* The PCI slave detected
						 * bad write data parity. */
#define PCI_IC_SRdPerr		PCI__BIT(2)	/* PERR* response to read
						 * data driven by PCI slave. */
#define PCI_IC_MAPerr		PCI__BIT(4)	/* PERR* response to address
						 * driven by the PCI master. */
#define PCI_IC_MWrPerr		PCI__BIT(5)	/* PERR* response to write data
						 * driven by the PCI master. */
#define PCI_IC_MRdPerr		PCI__BIT(6)	/* Bad data parity detected
						 * during the PCI master read
						 * transaction. */
#define PCI_IC_MMabort		PCI__BIT(8)	/* The PCI master generated
						 * master abort. */
#define PCI_IC_MTabort		PCI__BIT(9)	/* The PCI master detected
						 * target abort. */
#define PCI_IC_MMasterEn	PCI__BIT(10)	/* An attempt to generate a PCI
						 * transaction while master is
						 * not enabled. */
#define PCI_IC_MRetry		PCI__BIT(11)	/* The PCI master reached
						 * retry counter limit. */
#define PCI_IC_SMabort		PCI__BIT(16)	/* The PCI slave detects an il-
						 * legal master termination. */
#define PCI_IC_STabort		PCI__BIT(17)	/* The PCI slave terminates a
						 * transaction with Target
						 * Abort. */
#define PCI_IC_SAccProt		PCI__BIT(18)	/* A PCI slave access protect
						 * violation. */
#define PCI_IC_SWrProt		PCI__BIT(19)	/* A PCI slave write protect
						 * violation. */
#define PCI_IC_SRdBuf		PCI__BIT(20)	/* A PCI slave read buffer
						 * discard timer expired. */
#define PCI_IC_Arb		PCI__BIT(21)	/* Internal PCI arbiter detec-
						 * tion of a broken master. */
#define PCI_IC_BIST		PCI__BIT(24)	/* PCI BIST Interrupt */
#define PCI_IC_PMG		PCI__BIT(25)	/* PCI Power Management
						 * Interrupt */
#define PCI_IC_PRST		PCI__BIT(26)	/* PCI Reset Assert */

/*
31:27 Sel Specifies the error event currently being reported in the
Error Address, Error Data, and Error Command registers.
*/
#define PCI_IC_SEL_GET(v)	PCI__EXT((v), 27, 5)
#define PCI_IC_SEL_SAPerr	0x00
#define PCI_IC_SEL_SWrPerr	0x01
#define PCI_IC_SEL_SRdPerr	0x02
#define PCI_IC_SEL_MAPerr	0x04
#define PCI_IC_SEL_MWrPerr	0x05
#define PCI_IC_SEL_MRdPerr	0x06
#define PCI_IC_SEL_MMabort	0x08
#define PCI_IC_SEL_MTabort	0x09
#define PCI_IC_SEL_MMasterEn	0x0a
#define PCI_IC_SEL_MRetry	0x0b
#define PCI_IC_SEL_SMabort	0x10
#define PCI_IC_SEL_STabort	0x11
#define PCI_IC_SEL_SAccProt	0x12
#define PCI_IC_SEL_SWrProt	0x13
#define PCI_IC_SEL_SRdBuf	0x14
#define PCI_IC_SEL_Arb		0x15
#define PCI_IC_SEL_BIST		0x18
#define PCI_IC_SEL_PMG		0x19
#define PCI_IC_SEL_PRST		0x1a

#define	PCI_IC_SEL_Strings { \
	"SAPerr",  "SWrPerr", "SRdPerr",   "Rsvd#03", \
	"MAPerr",  "MWrPerr", "MRdPerr",   "Rsvd#07", \
	"MMabort", "MTabort", "MMasterEn", "MRetry", \
	"Rsvd#0c", "Rsvd#0d", "Rsvd#0e",   "Rsvd#0f", \
	"SMabort", "STabort", "SAccProt",  "SWrProt", \
	"SRdBuf",  "Arb",     "Rsvd#16",   "Rsvd#17", \
	"BIST",    "PMG",     "PRST",      "Rsvd#1b", \
	"Rsvd#1c", "Rsvd#1d", "Rsvd#1e",   "Rsvd#1f" }

/*
 * Table 315: PCI Error Mask
 * If the corresponding bit is 1, that interrupt is enabled
 * Bits 3, 7, 12:15, 22:23, 27:31 are reserved.
 */
#define PCI_ERRMASK_SAPErr		PCI__BIT(0)
#define PCI_ERRMASK_SWrPErr		PCI__BIT(1)
#define PCI_ERRMASK_SRdPErr		PCI__BIT(2)
#define PCI_ERRMASK_MAPErr		PCI__BIT(4)
#define PCI_ERRMASK_MWRPErr		PCI__BIT(5)
#define PCI_ERRMASK_MRDPErr		PCI__BIT(6)
#define PCI_ERRMASK_MMAbort		PCI__BIT(8)
#define PCI_ERRMASK_MTAbort		PCI__BIT(9)
#define PCI_ERRMASK_MMasterEn		PCI__BIT(10)
#define PCI_ERRMASK_MRetry		PCI__BIT(11)
#define PCI_ERRMASK_SMAbort		PCI__BIT(16)
#define PCI_ERRMASK_STAbort		PCI__BIT(17)
#define PCI_ERRMASK_SAccProt		PCI__BIT(18)
#define PCI_ERRMASK_SWrProt		PCI__BIT(19)
#define PCI_ERRMASK_SRdBuf		PCI__BIT(20)
#define PCI_ERRMASK_Arb			PCI__BIT(21)
#define PCI_ERRMASK_BIST		PCI__BIT(24)
#define PCI_ERRMASK_PMG			PCI__BIT(25)
#define PCI_ERRMASK_PRST		PCI__BIT(26)

#endif /* _DEV_GTPCIREG_H_ */
