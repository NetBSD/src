/*	$NetBSD: i82557reg.h,v 1.8 2001/05/22 15:29:30 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1995, David Greenman
 * Copyright (c) 2001 Jonathan Lemon <jlemon@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: if_fxpreg.h,v 1.24 2001/05/15 18:52:40 jlemon Exp
 */

#define FXP_PCI_MMBA	0x10
#define FXP_PCI_IOBA	0x14

/*
 * Control/status registers.
 */
#define	FXP_CSR_SCB_RUSCUS	0x00	/* scb_rus/scb_cus (1 byte) */
#define	FXP_CSR_SCB_STATACK	0x01	/* scb_statack (1 byte) */
#define	FXP_CSR_SCB_COMMAND	0x02	/* scb_command (1 byte) */
#define	FXP_CSR_SCB_INTRCNTL	0x03	/* scb_intrcntl (1 byte) */
#define	FXP_CSR_SCB_GENERAL	0x04	/* scb_general (4 bytes) */
#define	FXP_CSR_PORT		0x08	/* port (4 bytes) */
#define	FXP_CSR_FLASHCONTROL	0x0c	/* flash control (2 bytes) */
#define	FXP_CSR_EEPROMCONTROL	0x0e	/* eeprom control (2 bytes) */
#define	FXP_CSR_MDICONTROL	0x10	/* mdi control (4 bytes) */
#define	FXP_CSR_FLOWCONTROL	0x19	/* flow control (2 bytes) */

/*
 * FOR REFERENCE ONLY, the old definition of FXP_CSR_SCB_RUSCUS:
 *
 *	volatile u_int8_t	:2,
 *				scb_rus:4,
 *				scb_cus:2;
 */

#define FXP_PORT_SOFTWARE_RESET		0
#define FXP_PORT_SELFTEST		1
#define FXP_PORT_SELECTIVE_RESET	2
#define FXP_PORT_DUMP			3

#define FXP_SCB_RUS_IDLE		0
#define FXP_SCB_RUS_SUSPENDED		1
#define FXP_SCB_RUS_NORESOURCES		2
#define FXP_SCB_RUS_READY		4
#define FXP_SCB_RUS_SUSP_NORBDS		9
#define FXP_SCB_RUS_NORES_NORBDS	10
#define FXP_SCB_RUS_READY_NORBDS	12

#define FXP_SCB_CUS_IDLE		0
#define FXP_SCB_CUS_SUSPENDED		1
#define FXP_SCB_CUS_ACTIVE		2

#define	FXP_SCB_INTR_DISABLE		0x01	/* disable all interrupts */
#define	FXP_SCB_INTR_SWI		0x02	/* generate SWI */
#define	FXP_SCB_INTMASK_FCP		0x04
#define	FXP_SCB_INTMASK_ER		0x08
#define	FXP_SCB_INTMASK_RNR		0x10
#define	FXP_SCB_INTMASK_CNA		0x20
#define	FXP_SCB_INTMASK_FR		0x40
#define	FXP_SCB_INTMASK_CXTNO		0x80

#define	FXP_SCB_STATACK_FCP		0x01	/* flow control pause */
#define	FXP_SCB_STATACK_ER		0x02	/* early receive */
#define FXP_SCB_STATACK_SWI		0x04
#define FXP_SCB_STATACK_MDI		0x08
#define FXP_SCB_STATACK_RNR		0x10
#define FXP_SCB_STATACK_CNA		0x20
#define FXP_SCB_STATACK_FR		0x40
#define FXP_SCB_STATACK_CXTNO		0x80

#define FXP_SCB_COMMAND_CU_NOP		0x00
#define FXP_SCB_COMMAND_CU_START	0x10
#define FXP_SCB_COMMAND_CU_RESUME	0x20
#define FXP_SCB_COMMAND_CU_DUMP_ADR	0x40
#define FXP_SCB_COMMAND_CU_DUMP		0x50
#define FXP_SCB_COMMAND_CU_BASE		0x60
#define FXP_SCB_COMMAND_CU_DUMPRESET	0x70

#define FXP_SCB_COMMAND_RU_NOP		0
#define FXP_SCB_COMMAND_RU_START	1
#define FXP_SCB_COMMAND_RU_RESUME	2
#define FXP_SCB_COMMAND_RU_ABORT	4
#define FXP_SCB_COMMAND_RU_LOADHDS	5
#define FXP_SCB_COMMAND_RU_BASE		6
#define FXP_SCB_COMMAND_RU_RBDRESUME	7

/*
 * Command block definitions
 */

/*
 * NOP command.
 */
struct fxp_cb_nop {
	volatile u_int16_t cb_status;
	volatile u_int16_t cb_command;
	volatile u_int32_t link_addr;
};

/*
 * Individual Address command.
 */
struct fxp_cb_ias {
	volatile u_int16_t cb_status;
	volatile u_int16_t cb_command;
	volatile u_int32_t link_addr;
	volatile u_int8_t macaddr[6];
};

#if BYTE_ORDER == LITTLE_ENDIAN
#define	__FXP_BITFIELD2(a, b)			a, b
#define	__FXP_BITFIELD3(a, b, c)		a, b, c
#define	__FXP_BITFIELD4(a, b, c, d)		a, b, c, d
#define	__FXP_BITFIELD5(a, b, c, d, e)		a, b, c, d, e
#define	__FXP_BITFIELD6(a, b, c, d, e, f)	a, b, c, d, e, f
#define	__FXP_BITFIELD7(a, b, c, d, e, f, g)	a, b, c, d, e, f, g
#define	__FXP_BITFIELD8(a, b, c, d, e, f, g, h)	a, b, c, d, e, f, g, h
#else
#define	__FXP_BITFIELD2(a, b)			b, a
#define	__FXP_BITFIELD3(a, b, c)		c, b, a
#define	__FXP_BITFIELD4(a, b, c, d)		d, c, b, a
#define	__FXP_BITFIELD5(a, b, c, d, e)		e, d, c, b, a,
#define	__FXP_BITFIELD6(a, b, c, d, e, f)	f, e, d, c, b, a
#define	__FXP_BITFIELD6(a, b, c, d, e, f, g)	g, f, e, d, c, b, a
#define	__FXP_BITFIELD8(a, b, c, d, e, f, g, h)	h, g, f, e, d, c, b, a
#endif

/*
 * Configure command.
 */
struct fxp_cb_config {
	volatile u_int16_t	cb_status;
	volatile u_int16_t	cb_command;
	volatile u_int32_t	link_addr;
	volatile u_int8_t	__FXP_BITFIELD2(byte_count:6, :2);
	volatile u_int8_t	__FXP_BITFIELD3(rx_fifo_limit:4,
				    tx_fifo_limit:3,
				    :1);
	volatile u_int8_t	adaptive_ifs;
	volatile u_int8_t	__FXP_BITFIELD5(mwi_enable:1,	/* 8,9 */
				    type_enable:1,		/* 8,9 */
				    read_align_en:1,		/* 8,9 */
				    end_wr_on_cl:1,		/* 8,9 */
				    :4);
	volatile u_int8_t	__FXP_BITFIELD2(rx_dma_bytecount:7,
				    :1);
	volatile u_int8_t	__FXP_BITFIELD2(tx_dma_bytecount:7,
				    dma_mbce:1);
	volatile u_int8_t	__FXP_BITFIELD8(late_scb:1,	/* 7 */
				    direct_dma_dis:1,		/* 8,9 */
				    tno_int_or_tco_en:1,	/* 7,9 */
				    ci_int:1,
				    ext_txcb_dis:1,		/* 8,9 */
				    ext_stats_dis:1,		/* 8,9 */
				    keep_overrun_rx:1,
				    save_bf:1);
	volatile u_int8_t	__FXP_BITFIELD5(disc_short_rx:1,
				    underrun_retry:2,
				    :3,
				    two_frames:1,		/* 8,9 */
				    dyn_tbd:1);			/* 8,9 */
	volatile u_int8_t	__FXP_BITFIELD3(mediatype:1,	/* 7 */
				    :6,
				    csma_dis:1);		/* 8,9 */
	volatile u_int8_t	__FXP_BITFIELD6(tcp_udp_cksum:1,/* 9 */
				    :3,
				    vlan_tco:1,			/* 8,9 */
				    link_wake_en:1,		/* 8,9 */
				    arp_wake_en:1,		/* 8 */
				    mc_wake_en:1);		/* 8 */
	volatile u_int8_t	__FXP_BITFIELD4(:3,
				    nsai:1,
				    preamble_length:2,
				    loopback:2);
	volatile u_int8_t	__FXP_BITFIELD2(linear_priority:3,/* 7 */
				    :5);
	volatile u_int8_t	__FXP_BITFIELD3(linear_pri_mode:1,/* 7 */
				    :3,
				    interfrm_spacing:4);
	volatile u_int8_t	:8;
	volatile u_int8_t	:8;
	volatile u_int8_t	__FXP_BITFIELD8(promiscuous:1,
				    bcast_disable:1,
				    wait_after_win:1,		/* 8,9 */
				    :1,
				    ignore_ul:1,		/* 8,9 */
				    crc16_en:1,			/* 9 */
				    :1,
				    crscdt:1);
	volatile u_int8_t	fc_delay_lsb:8;			/* 8,9 */
	volatile u_int8_t	fc_delay_msb:8;			/* 8,9 */
	volatile u_int8_t	__FXP_BITFIELD6(stripping:1,
				    padding:1,
				    rcv_crc_xfer:1,
				    long_rx_en:1,		/* 8,9 */
				    pri_fc_thresh:3,		/* 8,9 */
				    :1);
	volatile u_int8_t	__FXP_BITFIELD8(ia_wake_en:1,	/* 8 */
				    magic_pkt_dis:1,		/* 8,9,!9ER */
				    tx_fc_dis:1,		/* 8,9 */
				    rx_fc_restop:1,		/* 8,9 */
				    rx_fc_restart:1,		/* 8,9 */
				    fc_filter:1,		/* 8,9 */
				    force_fdx:1,
				    fdx_pin_en:1);
	volatile u_int8_t	__FXP_BITFIELD4(:5,
				    pri_fc_loc:1		/* 8,9 */,
				    multi_ia:1,
				    :1);
	volatile u_int8_t	__FXP_BITFIELD3(:3, mc_all:1, :4);
};

/*
 * Multicast setup command.
 */
#define MAXMCADDR 80
struct fxp_cb_mcs {
	volatile u_int16_t cb_status;
	volatile u_int16_t cb_command;
	volatile u_int32_t link_addr;
	volatile u_int16_t mc_cnt;
	volatile u_int8_t mc_addr[MAXMCADDR][6];
};

/*
 * Transmit command.
 */
struct fxp_cb_tx {
	volatile u_int16_t cb_status;
	volatile u_int16_t cb_command;
	volatile u_int32_t link_addr;
	volatile u_int32_t tbd_array_addr;
	volatile u_int16_t byte_count;
	volatile u_int8_t tx_threshold;
	volatile u_int8_t tbd_number;
	/*
	 * If using the extended TxCB feature, there is a
	 * two TBDs right here.  We handle this in the
	 * fxp_control_data in i82557var.h.
	 */
};

/*
 * Transmit buffer descriptors.
 */
struct fxp_tbd {
	volatile u_int32_t tb_addr;
	volatile u_int32_t tb_size;
};

/*
 * Control Block (CB) definitions
 */

/* status */
#define FXP_CB_STATUS_OK	0x2000
#define FXP_CB_STATUS_C		0x8000

/* commands */
#define FXP_CB_COMMAND_NOP	0x0
#define FXP_CB_COMMAND_IAS	0x1
#define FXP_CB_COMMAND_CONFIG	0x2
#define FXP_CB_COMMAND_MCAS	0x3
#define FXP_CB_COMMAND_XMIT	0x4
#define FXP_CB_COMMAND_RESRV	0x5
#define FXP_CB_COMMAND_DUMP	0x6
#define FXP_CB_COMMAND_DIAG	0x7

/* command flags */
#define FXP_CB_COMMAND_SF	0x0008	/* simple/flexible mode */
#define FXP_CB_COMMAND_I	0x2000	/* generate interrupt on completion */
#define FXP_CB_COMMAND_S	0x4000	/* suspend on completion */
#define FXP_CB_COMMAND_EL	0x8000	/* end of list */

/*
 * Receive Frame Area.
 *
 * NOTE!  The RFA will NOT be aligned on a 4-byte boundary in the DMA
 * area!  To prevent EGCS from optimizing the copy of link_addr and
 * rbd_addr (which would cause an unaligned access fault on RISC systems),
 * we must make them an array of bytes!
 */
struct fxp_rfa {
	volatile u_int16_t rfa_status;
	volatile u_int16_t rfa_control;
	volatile u_int8_t link_addr[4];
	volatile u_int8_t rbd_addr[4];
	volatile u_int16_t actual_size;
	volatile u_int16_t size;
};

#define FXP_RFA_STATUS_RCOL	0x0001	/* receive collision */
#define FXP_RFA_STATUS_IAMATCH	0x0002	/* 0 = matches station address */
#define FXP_RFA_STATUS_S4	0x0010	/* receive error from PHY */
#define FXP_RFA_STATUS_TL	0x0020	/* type/length */
#define FXP_RFA_STATUS_FTS	0x0080	/* frame too short */
#define FXP_RFA_STATUS_OVERRUN	0x0100	/* DMA overrun */
#define FXP_RFA_STATUS_RNR	0x0200	/* no resources */
#define FXP_RFA_STATUS_ALIGN	0x0400	/* alignment error */
#define FXP_RFA_STATUS_CRC	0x0800	/* CRC error */
#define FXP_RFA_STATUS_OK	0x2000	/* packet received okay */
#define FXP_RFA_STATUS_C	0x8000	/* packet reception complete */

#define FXP_RFA_CONTROL_SF	0x0008	/* simple/flexible memory mode */
#define FXP_RFA_CONTROL_H	0x0010	/* header RFD */
#define FXP_RFA_CONTROL_S	0x4000	/* suspend after reception */
#define FXP_RFA_CONTROL_EL	0x8000	/* end of list */

/*
 * Statistics dump area definitions
 */
struct fxp_stats {
	volatile u_int32_t tx_good;
	volatile u_int32_t tx_maxcols;
	volatile u_int32_t tx_latecols;
	volatile u_int32_t tx_underruns;
	volatile u_int32_t tx_lostcrs;
	volatile u_int32_t tx_deffered;
	volatile u_int32_t tx_single_collisions;
	volatile u_int32_t tx_multiple_collisions;
	volatile u_int32_t tx_total_collisions;
	volatile u_int32_t rx_good;
	volatile u_int32_t rx_crc_errors;
	volatile u_int32_t rx_alignment_errors;
	volatile u_int32_t rx_rnr_errors;
	volatile u_int32_t rx_overrun_errors;
	volatile u_int32_t rx_cdt_errors;
	volatile u_int32_t rx_shortframes;
	volatile u_int32_t completion_status;
};
#define FXP_STATS_DUMP_COMPLETE	0xa005
#define FXP_STATS_DR_COMPLETE	0xa007
	
/*
 * Serial EEPROM control register bits
 */
#define FXP_EEPROM_EESK		0x01		/* shift clock */
#define FXP_EEPROM_EECS		0x02		/* chip select */
#define FXP_EEPROM_EEDI		0x04		/* data in */
#define FXP_EEPROM_EEDO		0x08		/* data out */

/*
 * Serial EEPROM opcodes, including start bit
 */
#define FXP_EEPROM_OPC_ERASE	0x4
#define FXP_EEPROM_OPC_WRITE	0x5
#define FXP_EEPROM_OPC_READ	0x6

/*
 * Management Data Interface opcodes
 */
#define FXP_MDI_WRITE		0x1
#define FXP_MDI_READ		0x2

/*
 * PHY device types (from EEPROM)
 */
#define	FXP_PHY_DEVICE_MASK	0x3f00
#define	FXP_PHY_DEVICE_SHIFT	8
#define	FXP_PHY_DEVADDR_MASK	0x00ff
#define	FXP_PHY_SERIAL_ONLY	0x8000
#define FXP_PHY_NONE		0
#define FXP_PHY_82553A		1
#define FXP_PHY_82553C		2
#define FXP_PHY_82503		3
#define FXP_PHY_DP83840		4
#define FXP_PHY_80C240		5
#define FXP_PHY_80C24		6
#define FXP_PHY_82555		7
#define FXP_PHY_DP83840A	10
#define	FXP_PHY_DP82555B	11

/*
 * PCI revisions.
 */
#define	FXP_REV_82558_A4	4
#define	FXP_REV_82558_B0	5
#define	FXP_REV_82559_A0	8
#define	FXP_REV_82559S_A	9
#define	FXP_REV_82550		12
#define	FXP_REV_82550_C		13
