/*	$NetBSD: vme_tworeg.h,v 1.1 1999/02/20 00:12:01 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef __mvme68k_vme_tworeg_h
#define __mvme68k_vme_tworeg_h

/*
 * Where the VMEchip2's registers live relative to the start
 * of internal I/O space.
 */
#define VMECHIP2_BASE		(0xfff40000u)
#define	VMECHIP2_VADDR(off)	((void *)IIOV(VMECHIP2_BASE + (off)))
#define	VME2_REGS_LCSR		0x0000
#define	VME2_REGS_GCSR		0x0100


/*
 * Register map of the Type 2 VMEchip found on the MVME-1[67]7 boards.
 * Note: Only responds to D32 accesses.
 */
struct vme_two_lcsr {
	/*
	 * Slave window configuration registers
	 */
#define VME2_SLAVE_WINDOWS	2
	volatile u_int32_t	vme_slave_win[VME2_SLAVE_WINDOWS];
	volatile u_int32_t	vme_slave_trans[VME2_SLAVE_WINDOWS];
	volatile u_int32_t	vme_slave_ctrl;

	/*
	 * Master window address control registers
	 */
#define VME2_MASTER_WINDOWS	4
	volatile u_int32_t	vme_master_win[VME2_MASTER_WINDOWS];
	volatile u_int32_t	vme_master_trans;
	volatile u_int32_t	vme_master_attr;

	/*
	 * GCSR Group/Board addresses, and
	 * Local Bus to VMEbus Enable Control register
	 */
	volatile u_int32_t	_vme_gcsrbba_loc2vme_enab;	/* 0x2c */
#define	vme_gcsr_group_addr	_vme_gcsrba_loc2vme_enab
#define	vme_gcsr_board_addr	_vme_gcsrbba_loc2vme_enab
#define	vme_local_2_vme_enab	_vme_gcsrbba_loc2vme_enab

	/*
	 * Local Bus to VMEbus I/O Control register
	 * ROM Control register
	 * VMEChip2 PROM Decoder, SRAM and DMA Control register
	 */
	volatile u_int32_t	_vme_lbvbio_rom_psd;		/* 0x30 */
#define vme_local_2_vme_ctrl	_vme_lbvbio_rom_psd
#define VME2_LOC2VME_MASK	(0xff000000u)
#define	VME2_LOC2VME_I1SU	(1u << 24)
#define	VME2_LOC2VME_I1WP	(1u << 25)
#define	VME2_LOC2VME_I1D16	(1u << 26)
#define	VME2_LOC2VME_I1EN	(1u << 27)
#define	VME2_LOC2VME_I2PD	(1u << 28)
#define	VME2_LOC2VME_I2SU	(1u << 29)
#define	VME2_LOC2VME_I2WP	(1u << 30)
#define	VME2_LOC2VME_I2EN	(1u << 31)

#define vme_psd_srams		_vme_lbvbio_rom_psd
#define	VME2_PSD_SRAMS_MASK	(0x00ff0000u)
#define	VME2_PSD_SRAMS_CLKS6	(0u << 16)
#define	VME2_PSD_SRAMS_CLKS5	(1u << 16)
#define	VME2_PSD_SRAMS_CLKS4	(2u << 16)
#define	VME2_PSD_SRAMS_CLKS3	(3u << 16)
#define VME2_PSD_TBLSC_INHIB	(0u << 18)
#define VME2_PSD_TBLSC_WRSINK	(1u << 18)
#define VME2_PSD_TBLSC_WRINV	(2u << 18)
#define VME2_PSD_ROM0		(1u << 20)
#define VME2_PSD_WAITRMW	(1u << 21)

#define vme_lbvr_ctrl		_vme_lbvbio_rom_psd
#define	VME2_LBVR_CTRL_MASK	(0x0000ff00u)
#define	VME2_LBVR_CTRL_LVREQL_0	(0u << 8)
#define	VME2_LBVR_CTRL_LVREQL_1	(1u << 8)
#define	VME2_LBVR_CTRL_LVREQL_2	(2u << 8)
#define	VME2_LBVR_CTRL_LVREQL_3	(3u << 8)
#define VME2_LBVR_CTRL_LVRWD	(1u << 10)
#define VME2_LBVR_CTRL_LVFAIR	(1u << 11)
#define VME2_LBVR_CTRL_DWB	(1u << 13)
#define VME2_LBVR_CTRL_DHB	(1u << 14)
#define VME2_LBVR_CTRL_ROBN	(1u << 15)

#define vme_dmac_ctrl1		_vme_lbvbio_rom_psd
#define	VME2_DMAC_CTRL1_MASK	(0x000000ffu)
#define	VME2_DMAC_CTRL1_DREQL_0	(0u << 0)
#define	VME2_DMAC_CTRL1_DREQL_1	(1u << 0)
#define	VME2_DMAC_CTRL1_DREQL_2	(2u << 0)
#define	VME2_DMAC_CTRL1_DREQL_3	(3u << 0)
#define	VME2_DMAC_CTRL1_DRELM_0	(0u << 2)
#define	VME2_DMAC_CTRL1_DRELM_1	(1u << 2)
#define	VME2_DMAC_CTRL1_DRELM_2	(2u << 2)
#define	VME2_DMAC_CTRL1_DRELM_3	(3u << 2)
#define VME2_DMAC_CTRL1_DFAIR	(1u << 4)
#define VME2_DMAC_CTRL1_DTBL	(1u << 5)
#define VME2_DMAC_CTRL1_DEN	(1u << 6)
#define VME2_DMAC_CTRL1_DHALT	(1u << 7)

	/*
	 * DMA Control register #2
	 */
	volatile u_int32_t	vme_dmac_ctrl2;			/* 0x34 */
#define VME2_DMAC_CTRL2_MASK	(0x0000ffffu)
#define VME2_DMAC_CTRL2_SHIFT	0
#define VME2_DMAC_CTRL2_AM_MASK	(0x3f00u)
#define VME2_DMAC_CTRL2_BLK_D32	(1u << 14)
#define VME2_DMAC_CTRL2_BLK_D64	(2u << 14)
#define	VME2_DMAC_CTRL2_D16	(1u << 16)
#define	VME2_DMAC_CTRL2_TVME	(1u << 17)
#define	VME2_DMAC_CTRL2_LINC	(1u << 18)
#define	VME2_DMAC_CTRL2_VINC	(1u << 19)
#define	VME2_DMAC_CTRL2_SCINHIB	(0u << 21)
#define	VME2_DMAC_CTRL2_SCWRSNK	(1u << 21)
#define	VME2_DMAC_CTRL2_SCWRINV	(2u << 21)
#define	VME2_DMAC_CTRL2_INTE	(1u << 23)

	/*
	 * DMA Controller Local Bus and VMEbus Addresses, Byte
	 * Counter and Tabel Address Counter registers
	 */
	volatile u_int32_t	vme_lbaddr_counter;		/* 0x38 */
	volatile u_int32_t	vme_vbaddr_counter;		/* 0x3c */
	volatile u_int32_t	vme_byte_counter;		/* 0x40 */
	volatile u_int32_t	vme_tbaddr_counter;		/* 0x44 */

	/*
	 * VMEbus Interrupter Control register
	 */
	volatile u_int32_t	_vme_vbiv_mpu_dmai;		/* 0x48 */
#define vme_vb_icr		_vme_vbiv_mpu_dmai
#define	VME2_VB_ICR_MASK	(0xff000000u)
#define	VME2_VB_ICR_SHIFT	24
#define	VME2_VB_ICR_IRQL_MASK	(0x07000000u)
#define	VME2_VB_ICR_IRQS	(1u << 27)
#define	VME2_VB_ICR_IRQC	(1u << 28)
#define	VME2_VB_ICR_IRQ1S_INT	(0u << 29)
#define	VME2_VB_ICR_IRQ1S_TICK1	(1u << 29)
#define	VME2_VB_ICR_IRQ1S_TICK2	(3u << 29)

	/*
	 * VMEbus Interrupt Vector register
	 */
#define vme_vb_vector		_vme_vbiv_mpu_dmai
#define VME2_VB_VECTOR_MASK	(0x00ff0000u)
#define VME2_VB_VECTOR_SHIFT	16

	/*
	 * MPU Status and DMA Interrupt Count register
	 */
#define vme_mpu_status		_vme_vbiv_mpu_dmai
#define	VME2_MPU_STATUS_MASK	(0x00000f00u)
#define	VME2_MPU_STATUS_MLOB	(1u << 0)
#define	VME2_MPU_STATUS_MLPE	(1u << 1)
#define	VME2_MPU_STATUS_MLBE	(1u << 2)
#define	VME2_MPU_STATUS_MCLR	(1u << 3)

#define vme_dma_icount		_vme_vbiv_mpu_dmai
#define	VME2_DMA_ICOUNT(rv)	(((rv) >> 12) & 0x0fu)

	/*
	 * DMA Controller Status register
	 */
#define vme_dmac_status		_vme_vbiv_mpu_dmai
#define VME2_DMAC_STATUS_MASK	(0x000000ffu)
#define VME2_DMAC_STATUS_DONE	(1u << 0)
#define VME2_DMAC_STATUS_VME	(1u << 1)
#define VME2_DMAC_STATUS_TBL	(1u << 2)
#define VME2_DMAC_STATUS_DLTO	(1u << 3)
#define VME2_DMAC_STATUS_DLOB	(1u << 4)
#define VME2_DMAC_STATUS_DLPE	(1u << 5)
#define VME2_DMAC_STATUS_DLBE	(1u << 6)
#define VME2_DMAC_STATUS_MLTO	(1u << 7)


	volatile u_int32_t	_vme_ato_dvtoc_valbwdto_prescale;/* 0x4c */

	/*
	 * VMEbus Arbiter Time-out register
	 */
#define vme_arb_timeout		_vme_ato_dvtoc_valbwdto_prescale
#define	VME2_ARB_TIMEOUT_ENAB	(1u << 24)

	/*
	 * DMA Controller Timers and VMEbus Global Time-out Control registers
	 */
#define vme_global_timeout	_vme_ato_dvtoc_valbwdto_prescale
#define VME2_VMEGLB_TO_MASK	(0x00030000u)
#define	VME2_VMEGLB_TO_8US	(0u << 16)
#define	VME2_VMEGLB_TO_16US	(1u << 16)
#define	VME2_VMEGLB_TO_256US	(2u << 16)
#define	VME2_VMEGLB_TO_DISABLE	(3u << 16)

#define vme_dmac_timers		_vme_ato_dvtoc_valbwdto_prescale
#define VME2_DMACTMR_ON_MASK	(0x001c0000u)
#define VME2_DMACTMR_ON_16US	(0u << 18)
#define VME2_DMACTMR_ON_32US	(1u << 18)
#define VME2_DMACTMR_ON_64US	(2u << 18)
#define VME2_DMACTMR_ON_128US	(3u << 18)
#define VME2_DMACTMR_ON_256US	(4u << 18)
#define VME2_DMACTMR_ON_512US	(5u << 18)
#define VME2_DMACTMR_ON_1024US	(6u << 18)
#define VME2_DMACTMR_ON_DONE	(7u << 18)
#define VME2_DMACTMR_OFF_MASK	(0x00e00000u)
#define VME2_DMACTMR_OFF_0US	(0u << 21)
#define VME2_DMACTMR_OFF_16US	(1u << 21)
#define VME2_DMACTMR_OFF_32US	(2u << 21)
#define VME2_DMACTMR_OFF_64US	(3u << 21)
#define VME2_DMACTMR_OFF_128US	(4u << 21)
#define VME2_DMACTMR_OFF_256US	(5u << 21)
#define VME2_DMACTMR_OFF_512US	(6u << 21)
#define VME2_DMACTMR_OFF_1024US	(7u << 21)

	/*
	 * VME Access, Local Bus and Watchdog Time-out Control register
	 */
#define vme_vmeacc_timeout	_vme_ato_dvtoc_valbwdto_prescale
#define VME2_VMEACCTO_MASK	(0x0000c000u)
#define VME2_VMEACCTO_64US	(0u << 14)
#define VME2_VMEACCTO_1MS	(1u << 14)
#define VME2_VMEACCTO_32MS	(2u << 14)
#define VME2_VMEACCTO_DISABLE	(3u << 14)

#define vme_locacc_timeout	_vme_ato_dvtoc_valbwdto_prescale
#define VME2_LOCACCTO_MASK	(0x00003000u)
#define VME2_LOCACCTO_64US	(0u << 12)
#define VME2_LOCACCTO_1MS	(1u << 12)
#define VME2_LOCACCTO_32MS	(2u << 12)
#define VME2_LOCACCTO_DISABLE	(3u << 12)

#define vme_watchdog_timeout	_vme_ato_dvtoc_valbwdto_prescale
#define VME2_WATCHDOGTO_MASK	(0x00000f00u)
#define VME2_WATCHDOGTO_512US	(0u << 8)
#define VME2_WATCHDOGTO_1MS	(1u << 8)
#define VME2_WATCHDOGTO_2MS	(2u << 8)
#define VME2_WATCHDOGTO_4MS	(3u << 8)
#define VME2_WATCHDOGTO_8MS	(4u << 8)
#define VME2_WATCHDOGTO_16MS	(5u << 8)
#define VME2_WATCHDOGTO_32MS	(6u << 8)
#define VME2_WATCHDOGTO_64MS	(7u << 8)
#define VME2_WATCHDOGTO_128MS	(8u << 8)
#define VME2_WATCHDOGTO_256MS	(9u << 8)
#define VME2_WATCHDOGTO_512MS	(10u << 8)
#define VME2_WATCHDOGTO_1S	(11u << 8)
#define VME2_WATCHDOGTO_4S	(12u << 8)
#define VME2_WATCHDOGTO_16S	(13u << 8)
#define VME2_WATCHDOGTO_32S	(14u << 8)
#define VME2_WATCHDOGTO_64S	(15u << 8)

	/*
	 * Prescaler Control register
	 */
#define vme_prescaler_ctrl	_vme_ato_dvtoc_valbwdto_prescale
#define	VME2_PRESCALER_MASK	(0x000000ffu)
#define	VME2_PRESCALER_SHIFT	0
#define	VME2_PRESCALER_CTRL(c)	(256 - (c))

	/*
	 * Tick Timer registers
	 */
	volatile u_int32_t	vme_tt1_compare;		/* 0x50 */
	volatile u_int32_t	vme_tt1_counter;		/* 0x54 */
	volatile u_int32_t	vme_tt2_compare;		/* 0x58 */
	volatile u_int32_t	vme_tt2_counter;		/* 0x5c */


	volatile u_int32_t	_vme_bc_wdc_ttx;		/* 0x60 */

	/*
	 * Board Control register
	 */
#define vme_board_ctrl		_vme_bc_wdc_ttx
#define VME2_BRD_CTRL_MASK	(0xff000000u)
#define VME2_BRD_CTRL_RSWE	(1u << 24)
#define VME2_BRD_CTRL_BDFLO	(1u << 25)
#define VME2_BRD_CTRL_CPURS	(1u << 26)
#define VME2_BRD_CTRL_PURS	(1u << 27)
#define VME2_BRD_CTRL_BRFLI	(1u << 28)
#define VME2_BRD_CTRL_SFFL	(1u << 29)
#define VME2_BRD_CTRL_SCON	(1u << 30)

	/*
	 * Watchdog Timer Control register
	 */
#define vme_watchdog_ctrl	_vme_bc_wdc_ttx
#define VME2_WATCHD_CTRL_MASK	(0x00ff0000u)
#define VME2_WATCHD_CTRL_WDEN	(1u << 16)
#define VME2_WATCHD_CTRL_WDRSE	(1u << 17)
#define VME2_WATCHD_CTRL_WDSL	(1u << 18)
#define VME2_WATCHD_CTRL_WDBFE	(1u << 19)
#define VME2_WATCHD_CTRL_WDTO	(1u << 20)
#define VME2_WATCHD_CTRL_WDCC	(1u << 21)
#define VME2_WATCHD_CTRL_WDCS	(1u << 22)
#define VME2_WATCHD_CTRL_SRST	(1u << 23)

	/*
	 * Tick Timer Control registers
	 */
#define vme_tt2_ctrl		_vme_bc_wdc_ttx
#define VME2_TICK2_TMR_MASK	(0x0000ff00u)
#define VME2_TICK2_TMR_EN	(1u << 8)
#define VME2_TICK2_TMR_COC	(1u << 9)
#define VME2_TICK2_TMR_COF	(1u << 10)
#define VME2_TICK2_TMR_OVF_MASK	(0x0000f000u)
#define VME2_TICK2_TMR_OVF_SHFT	12

#define vme_tt1_ctrl		_vme_bc_wdc_ttx
#define VME2_TICK1_TMR_MASK	(0x000000ffu)
#define VME2_TICK1_TMR_EN	(1u << 0)
#define VME2_TICK1_TMR_COC	(1u << 1)
#define VME2_TICK1_TMR_COF	(1u << 2)
#define VME2_TICK1_TMR_OVF_MASK	(0x000000f0u)
#define VME2_TICK1_TMR_OVF_SHFT	4

	/*
	 * Prescaler Counter register
	 */
	volatile u_int32_t	vme_prescaler_count;		/* 0x64 */

	/*
	 * Local Bus Interrupter Status register
	 */
	volatile u_int32_t	vme_lbint_status;		/* 0x68 */
#define	VME2_LBINT_STATUS(v)	(1u << (v))

	/*
	 * Local Bus Interrupter Enable register
	 */
	volatile u_int32_t	vme_lbint_enable;		/* 0x6c */
#define VME2_LBINT_ENABLE(v)	(1u << (v))

	/*
	 * Software Interrupt Set register
	 */
	volatile u_int32_t	vme_softint_set;		/* 0x70 */
#define VME2_SOFTINT_SET(x)	(1u << ((x) + 8))

	/*
	 * Interrupt Clear register
	 */
	volatile u_int32_t	vme_lbint_clear;		/* 0x74 */
#define	VME2_LBINT_CLEAR(v)	(1u << (v))

	/*
	 * Interrupt Level registers
	 */
#define VME2_NUM_IL_REGS	4
	volatile u_int32_t	vme_irq_levels[VME2_NUM_IL_REGS];/* 0x78 */

#define	VME2_ILREG_FROM_VEC(v)	((((VME2_NUM_IL_REGS * 8) - 1) - (v)) / 8)
#define	VME2_ILSHFT_FROM_VEC(v)	(((v) & 7) * 4)
#define	VME2_ILMASK		(0x0fu)


	volatile u_int32_t	_vme_vecb_iocx;			/* 0x88 */

	/*
	 * Vector Base register
	 */
#define vme_vector_base		_vme_vecb_iocx
#define VME2_VECTOR_BASE_MASK	(0xff000000u)
#define	VME2_VECTOR_BASE_REG	(((VME2_VECTOR_BASE >> 4) | \
				  (VME2_VECTOR_BASE + 0x10)) << 24)

	/*
	 * I/O Control register #1
	 */
#define vme_io_ctrl1		_vme_vecb_iocx
#define VME2_IO_CTRL1_MASK	(0x00ff0000u)
#define VME2_IO_CTRL1_GPOEN(x)	(1u << ((x) + 16))
#define VME2_IO_CTRL1_ABRTL	(1u << 20)
#define VME2_IO_CTRL1_ACFL	(1u << 21)
#define VME2_IO_CTRL1_SYSFL	(1u << 22)
#define VME2_IO_CTRL1_MIEN	(1u << 23)

	/*
	 * I/O Control register #2
	 */
#define vme_io_ctrl2		_vme_vecb_iocx
#define VME2_IO_CTRL2_MASK	(0x0000ff00u)
#define VME2_IO_CTRL2_GPIOI(x)	(1u << ((x) + 8))
#define VME2_IO_CTRL2_GPIOO(x)	(1u << ((x) + 12))

	/*
	 * I/O Control register #3
	 */
#define vme_io_ctrl3		_vme_vecb_iocx
#define VME2_IO_CTRL3_MASK	(0x000000ff)
#define VME2_IO_CTRL3_GPI(x)	(1u << (x))

	/*
	 * Miscellaneous Control register
	 */
	volatile u_int32_t	vme_misc_ctrl;
#define VME2_MISC_CTRL_DISBGN	(1u << 0)
#define VME2_MISC_CTRL_ENINT	(1u << 1)
#define VME2_MISC_CTRL_DISBSYT	(1u << 2)
#define VME2_MISC_CTRL_NOELBBSY	(1u << 3)
#define VME2_MISC_CTRL_DISMST	(1u << 4)
#define VME2_MISC_CTRL_DISSRAM	(1u << 5)
#define VME2_MISC_CTRL_REVEROM	(1u << 6)
#define VME2_MISC_CTRL_MPIRQEN	(1u << 7)
};


/*
 * Define the layout of the VMEChip2's GCSR [registers].
 * These follow the standard VMEbus GCSR layout.
 */
struct vme_two_gcsr {
	u_int32_t		gcsr_not_yet;
};

#endif /* __mvme68k_vme_tworeg_h */
