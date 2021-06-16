/*	$NetBSD: if_aq.c,v 1.27 2021/06/16 00:21:18 riastradh Exp $	*/

/**
 * aQuantia Corporation Network Driver
 * Copyright (C) 2014-2017 aQuantia Corporation. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   (1) Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer.
 *
 *   (2) Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 *
 *   (3) The name of the author may not be used to endorse or promote
 *   products derived from this software without specific prior
 *   written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*-
 * Copyright (c) 2020 Ryo Shimizu <ryo@nerv.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_aq.c,v 1.27 2021/06/16 00:21:18 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_if_aq.h"
#include "sysmon_envsys.h"
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/module.h>
#include <sys/pcq.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/rss_config.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/sysmon/sysmonvar.h>

/* driver configuration */
#define CONFIG_INTR_MODERATION_ENABLE	true	/* delayed interrupt */
#undef CONFIG_LRO_SUPPORT			/* no LRO not suppoted */
#undef CONFIG_NO_TXRX_INDEPENDENT		/* share TX/RX interrupts */

#define AQ_NINTR_MAX			(AQ_RSSQUEUE_MAX + AQ_RSSQUEUE_MAX + 1)
					/* TX + RX + LINK. must be <= 32 */
#define AQ_LINKSTAT_IRQ			31	/* for legacy mode */

#define AQ_TXD_NUM			2048	/* per ring. 8*n && 32~8184 */
#define AQ_RXD_NUM			2048	/* per ring. 8*n && 32~8184 */
/* minimum required to send a packet (vlan needs additional TX descriptor) */
#define AQ_TXD_MIN			(1 + 1)


/* hardware specification */
#define AQ_RINGS_NUM			32
#define AQ_RSSQUEUE_MAX			8
#define AQ_RX_DESCRIPTOR_MIN		32
#define AQ_TX_DESCRIPTOR_MIN		32
#define AQ_RX_DESCRIPTOR_MAX		8184
#define AQ_TX_DESCRIPTOR_MAX		8184
#define AQ_TRAFFICCLASS_NUM		8
#define AQ_RSS_HASHKEY_SIZE		40
#define AQ_RSS_INDIRECTION_TABLE_MAX	64

#define AQ_JUMBO_MTU_REV_A		9000
#define AQ_JUMBO_MTU_REV_B		16338

/*
 * TERMINOLOGY
 *	MPI = MAC PHY INTERFACE?
 *	RPO = RX Protocol Offloading
 *	TPO = TX Protocol Offloading
 *	RPF = RX Packet Filter
 *	TPB = TX Packet buffer
 *	RPB = RX Packet buffer
 */

/* registers */
#define AQ_FW_SOFTRESET_REG			0x0000
#define  AQ_FW_SOFTRESET_RESET			__BIT(15) /* soft reset bit */
#define  AQ_FW_SOFTRESET_DIS			__BIT(14) /* reset disable */

#define AQ_FW_VERSION_REG			0x0018
#define AQ_HW_REVISION_REG			0x001c
#define AQ_GLB_NVR_INTERFACE1_REG		0x0100

#define AQ_FW_MBOX_CMD_REG			0x0200
#define  AQ_FW_MBOX_CMD_EXECUTE			0x00008000
#define  AQ_FW_MBOX_CMD_BUSY			0x00000100
#define AQ_FW_MBOX_ADDR_REG			0x0208
#define AQ_FW_MBOX_VAL_REG			0x020c

#define FW2X_LED_MIN_VERSION			0x03010026	/* >= 3.1.38 */
#define FW2X_LED_REG				0x031c
#define  FW2X_LED_DEFAULT			0x00000000
#define  FW2X_LED_NONE				0x0000003f
#define  FW2X_LINKLED				__BITS(0,1)
#define   FW2X_LINKLED_ACTIVE			0
#define   FW2X_LINKLED_ON			1
#define   FW2X_LINKLED_BLINK			2
#define   FW2X_LINKLED_OFF			3
#define  FW2X_STATUSLED				__BITS(2,5)
#define   FW2X_STATUSLED_ORANGE			0
#define   FW2X_STATUSLED_ORANGE_BLINK		2
#define   FW2X_STATUSLED_OFF			3
#define   FW2X_STATUSLED_GREEN			4
#define   FW2X_STATUSLED_ORANGE_GREEN_BLINK	8
#define   FW2X_STATUSLED_GREEN_BLINK		10

#define FW_MPI_MBOX_ADDR_REG			0x0360
#define FW1X_MPI_INIT1_REG			0x0364
#define FW1X_MPI_CONTROL_REG			0x0368
#define FW1X_MPI_STATE_REG			0x036c
#define  FW1X_MPI_STATE_MODE			__BITS(7,0)
#define  FW1X_MPI_STATE_SPEED			__BITS(32,16)
#define  FW1X_MPI_STATE_DISABLE_DIRTYWAKE	__BITS(25)
#define  FW1X_MPI_STATE_DOWNSHIFT		__BITS(31,28)
#define FW1X_MPI_INIT2_REG			0x0370
#define FW1X_MPI_EFUSEADDR_REG			0x0374

#define FW2X_MPI_EFUSEADDR_REG			0x0364
#define FW2X_MPI_CONTROL_REG			0x0368	/* 64bit */
#define FW2X_MPI_STATE_REG			0x0370	/* 64bit */
#define FW_BOOT_EXIT_CODE_REG			0x0388
#define  RBL_STATUS_DEAD			0x0000dead
#define  RBL_STATUS_SUCCESS			0x0000abba
#define  RBL_STATUS_FAILURE			0x00000bad
#define  RBL_STATUS_HOST_BOOT			0x0000f1a7

#define AQ_FW_GLB_CPU_SEM_REG(i)		(0x03a0 + (i) * 4)
#define AQ_FW_SEM_RAM_REG			AQ_FW_GLB_CPU_SEM_REG(2)

#define AQ_FW_GLB_CTL2_REG			0x0404
#define  AQ_FW_GLB_CTL2_MCP_UP_FORCE_INTERRUPT	__BIT(1)

#define AQ_GLB_GENERAL_PROVISIONING9_REG	0x0520
#define AQ_GLB_NVR_PROVISIONING2_REG		0x0534

#define FW_MPI_DAISY_CHAIN_STATUS_REG		0x0704

#define AQ_PCI_REG_CONTROL_6_REG		0x1014

// msix bitmap */
#define AQ_INTR_STATUS_REG			0x2000	/* intr status */
#define AQ_INTR_STATUS_CLR_REG			0x2050	/* intr status clear */
#define AQ_INTR_MASK_REG			0x2060	/* intr mask set */
#define AQ_INTR_MASK_CLR_REG			0x2070	/* intr mask clear */
#define AQ_INTR_AUTOMASK_REG			0x2090

/* AQ_INTR_IRQ_MAP_TXRX_REG[AQ_RINGS_NUM] 0x2100-0x2140 */
#define AQ_INTR_IRQ_MAP_TXRX_REG(i)		(0x2100 + ((i) / 2) * 4)
#define AQ_INTR_IRQ_MAP_TX_REG(i)		AQ_INTR_IRQ_MAP_TXRX_REG(i)
#define  AQ_INTR_IRQ_MAP_TX_IRQMAP(i)		(__BITS(28,24) >> (((i) & 1)*8))
#define  AQ_INTR_IRQ_MAP_TX_EN(i)		(__BIT(31)     >> (((i) & 1)*8))
#define AQ_INTR_IRQ_MAP_RX_REG(i)		AQ_INTR_IRQ_MAP_TXRX_REG(i)
#define  AQ_INTR_IRQ_MAP_RX_IRQMAP(i)		(__BITS(12,8)  >> (((i) & 1)*8))
#define  AQ_INTR_IRQ_MAP_RX_EN(i)		(__BIT(15)     >> (((i) & 1)*8))

/* AQ_GEN_INTR_MAP_REG[AQ_RINGS_NUM] 0x2180-0x2200 */
#define AQ_GEN_INTR_MAP_REG(i)			(0x2180 + (i) * 4)
#define  AQ_B0_ERR_INT				8U

#define AQ_INTR_CTRL_REG			0x2300
#define  AQ_INTR_CTRL_IRQMODE			__BITS(1,0)
#define  AQ_INTR_CTRL_IRQMODE_LEGACY		0
#define  AQ_INTR_CTRL_IRQMODE_MSI		1
#define  AQ_INTR_CTRL_IRQMODE_MSIX		2
#define  AQ_INTR_CTRL_MULTIVEC			__BIT(2)
#define  AQ_INTR_CTRL_AUTO_MASK			__BIT(5)
#define  AQ_INTR_CTRL_CLR_ON_READ		__BIT(7)
#define  AQ_INTR_CTRL_RESET_DIS			__BIT(29)
#define  AQ_INTR_CTRL_RESET_IRQ			__BIT(31)

#define AQ_MBOXIF_POWER_GATING_CONTROL_REG	0x32a8

#define FW_MPI_RESETCTRL_REG			0x4000
#define  FW_MPI_RESETCTRL_RESET_DIS		__BIT(29)

#define RX_SYSCONTROL_REG			0x5000
#define  RX_SYSCONTROL_RPB_DMA_LOOPBACK		__BIT(6)
#define  RX_SYSCONTROL_RPF_TPO_LOOPBACK		__BIT(8)
#define  RX_SYSCONTROL_RESET_DIS		__BIT(29)

#define RX_TCP_RSS_HASH_REG			0x5040
#define  RX_TCP_RSS_HASH_RPF2			__BITS(19,16)
#define  RX_TCP_RSS_HASH_TYPE			__BITS(15,0)

/* for RPF_*_REG.ACTION */
#define RPF_ACTION_DISCARD			0
#define RPF_ACTION_HOST				1
#define RPF_ACTION_MANAGEMENT			2
#define RPF_ACTION_HOST_MANAGEMENT		3
#define RPF_ACTION_WOL				4

#define RPF_L2BC_REG				0x5100
#define  RPF_L2BC_EN				__BIT(0)
#define  RPF_L2BC_PROMISC			__BIT(3)
#define  RPF_L2BC_ACTION			__BITS(12,14)
#define  RPF_L2BC_THRESHOLD			__BITS(31,16)

/* RPF_L2UC_*_REG[34] (actual [38]?) */
#define RPF_L2UC_LSW_REG(i)			(0x5110 + (i) * 8)
#define RPF_L2UC_MSW_REG(i)			(0x5114 + (i) * 8)
#define  RPF_L2UC_MSW_MACADDR_HI		__BITS(15,0)
#define  RPF_L2UC_MSW_ACTION			__BITS(18,16)
#define  RPF_L2UC_MSW_EN			__BIT(31)
#define AQ_HW_MAC_OWN			0	/* index of own address */
#define AQ_HW_MAC_NUM			34

/* RPF_MCAST_FILTER_REG[8] 0x5250-0x5270 */
#define RPF_MCAST_FILTER_REG(i)			(0x5250 + (i) * 4)
#define  RPF_MCAST_FILTER_EN			__BIT(31)
#define RPF_MCAST_FILTER_MASK_REG		0x5270
#define  RPF_MCAST_FILTER_MASK_ALLMULTI		__BIT(14)

#define RPF_VLAN_MODE_REG			0x5280
#define  RPF_VLAN_MODE_PROMISC			__BIT(1)
#define  RPF_VLAN_MODE_ACCEPT_UNTAGGED		__BIT(2)
#define  RPF_VLAN_MODE_UNTAGGED_ACTION		__BITS(5,3)

#define RPF_VLAN_TPID_REG			0x5284
#define  RPF_VLAN_TPID_OUTER			__BITS(31,16)
#define  RPF_VLAN_TPID_INNER			__BITS(15,0)

/* RPF_VLAN_FILTER_REG[RPF_VLAN_MAX_FILTERS] 0x5290-0x52d0 */
#define RPF_VLAN_MAX_FILTERS			16
#define RPF_VLAN_FILTER_REG(i)			(0x5290 + (i) * 4)
#define  RPF_VLAN_FILTER_EN			__BIT(31)
#define  RPF_VLAN_FILTER_RXQ_EN			__BIT(28)
#define  RPF_VLAN_FILTER_RXQ			__BITS(24,20)
#define  RPF_VLAN_FILTER_ACTION			__BITS(18,16)
#define  RPF_VLAN_FILTER_ID			__BITS(11,0)

/* RPF_ETHERTYPE_FILTER_REG[AQ_RINGS_NUM] 0x5300-0x5380 */
#define RPF_ETHERTYPE_FILTER_REG(i)		(0x5300 + (i) * 4)
#define  RPF_ETHERTYPE_FILTER_EN		__BIT(31)
#define  RPF_ETHERTYPE_FILTER_PRIO_EN		__BIT(30)
#define  RPF_ETHERTYPE_FILTER_RXQF_EN		__BIT(29)
#define  RPF_ETHERTYPE_FILTER_PRIO		__BITS(28,26)
#define  RPF_ETHERTYPE_FILTER_RXQF		__BITS(24,20)
#define  RPF_ETHERTYPE_FILTER_MNG_RXQF		__BIT(19)
#define  RPF_ETHERTYPE_FILTER_ACTION		__BITS(18,16)
#define  RPF_ETHERTYPE_FILTER_VAL		__BITS(15,0)

/* RPF_L3_FILTER_REG[8] 0x5380-0x53a0 */
#define RPF_L3_FILTER_REG(i)			(0x5380 + (i) * 4)
#define  RPF_L3_FILTER_L4_EN			__BIT(31)
#define  RPF_L3_FILTER_IPV6_EN			__BIT(30)
#define  RPF_L3_FILTER_SRCADDR_EN		__BIT(29)
#define  RPF_L3_FILTER_DSTADDR_EN		__BIT(28)
#define  RPF_L3_FILTER_L4_SRCPORT_EN		__BIT(27)
#define  RPF_L3_FILTER_L4_DSTPORT_EN		__BIT(26)
#define  RPF_L3_FILTER_L4_PROTO_EN		__BIT(25)
#define  RPF_L3_FILTER_ARP_EN			__BIT(24)
#define  RPF_L3_FILTER_L4_RXQUEUE_EN		__BIT(23)
#define  RPF_L3_FILTER_L4_RXQUEUE_MANAGEMENT_EN	__BIT(22)
#define  RPF_L3_FILTER_L4_ACTION		__BITS(16,18)
#define  RPF_L3_FILTER_L4_RXQUEUE		__BITS(12,8)
#define  RPF_L3_FILTER_L4_PROTO			__BITS(2,0)
#define   RPF_L3_FILTER_L4_PROTO_TCP		0
#define   RPF_L3_FILTER_L4_PROTO_UDP		1
#define   RPF_L3_FILTER_L4_PROTO_SCTP		2
#define   RPF_L3_FILTER_L4_PROTO_ICMP		3
/* parameters of RPF_L3_FILTER_REG[8] */
#define RPF_L3_FILTER_SRCADDR_REG(i)		(0x53b0 + (i) * 4)
#define RPF_L3_FILTER_DSTADDR_REG(i)		(0x53d0 + (i) * 4)
#define RPF_L3_FILTER_L4_SRCPORT_REG(i)		(0x5400 + (i) * 4)
#define RPF_L3_FILTER_L4_DSTPORT_REG(i)		(0x5420 + (i) * 4)

#define RX_FLR_RSS_CONTROL1_REG			0x54c0
#define  RX_FLR_RSS_CONTROL1_EN			__BIT(31)

#define RPF_RPB_RX_TC_UPT_REG			0x54c4
#define  RPF_RPB_RX_TC_UPT_MASK(i)		(0x00000007 << ((i) * 4))

#define RPF_RSS_KEY_ADDR_REG			0x54d0
#define  RPF_RSS_KEY_ADDR			__BITS(4,0)
#define  RPF_RSS_KEY_WR_EN			__BIT(5)
#define RPF_RSS_KEY_WR_DATA_REG			0x54d4
#define RPF_RSS_KEY_RD_DATA_REG			0x54d8

#define RPF_RSS_REDIR_ADDR_REG			0x54e0
#define  RPF_RSS_REDIR_ADDR			__BITS(3,0)
#define  RPF_RSS_REDIR_WR_EN			__BIT(4)

#define RPF_RSS_REDIR_WR_DATA_REG		0x54e4
#define  RPF_RSS_REDIR_WR_DATA			__BITS(15,0)

#define RPO_HWCSUM_REG				0x5580
#define  RPO_HWCSUM_IP4CSUM_EN			__BIT(1)
#define  RPO_HWCSUM_L4CSUM_EN			__BIT(0) /* TCP/UDP/SCTP */

#define RPO_LRO_ENABLE_REG			0x5590

#define RPO_LRO_CONF_REG			0x5594
#define  RPO_LRO_CONF_QSESSION_LIMIT		__BITS(13,12)
#define  RPO_LRO_CONF_TOTAL_DESC_LIMIT		__BITS(6,5)
#define  RPO_LRO_CONF_PATCHOPTIMIZATION_EN	__BIT(15)
#define  RPO_LRO_CONF_MIN_PAYLOAD_OF_FIRST_PKT	__BITS(4,0)
#define RPO_LRO_RSC_MAX_REG			0x5598

/* RPO_LRO_LDES_MAX_REG[32/8] 0x55a0-0x55b0 */
#define RPO_LRO_LDES_MAX_REG(i)			(0x55a0 + (i / 8) * 4)
#define  RPO_LRO_LDES_MAX_MASK(i)		(0x00000003 << ((i & 7) * 4))
#define RPO_LRO_TB_DIV_REG			0x5620
#define  RPO_LRO_TB_DIV				__BITS(20,31)
#define RPO_LRO_INACTIVE_IVAL_REG		0x5620
#define  RPO_LRO_INACTIVE_IVAL			__BITS(10,19)
#define RPO_LRO_MAX_COALESCING_IVAL_REG		0x5620
#define  RPO_LRO_MAX_COALESCING_IVAL		__BITS(9,0)

#define RPB_RPF_RX_REG				0x5700
#define  RPB_RPF_RX_TC_MODE			__BIT(8)
#define  RPB_RPF_RX_FC_MODE			__BITS(5,4)
#define  RPB_RPF_RX_BUF_EN			__BIT(0)

/* RPB_RXB_BUFSIZE_REG[AQ_TRAFFICCLASS_NUM] 0x5710-0x5790 */
#define RPB_RXB_BUFSIZE_REG(i)			(0x5710 + (i) * 0x10)
#define  RPB_RXB_BUFSIZE			__BITS(8,0)
#define RPB_RXB_XOFF_REG(i)			(0x5714 + (i) * 0x10)
#define  RPB_RXB_XOFF_EN			__BIT(31)
#define  RPB_RXB_XOFF_THRESH_HI			__BITS(29,16)
#define  RPB_RXB_XOFF_THRESH_LO			__BITS(13,0)

#define RX_DMA_DESC_CACHE_INIT_REG		0x5a00
#define  RX_DMA_DESC_CACHE_INIT			__BIT(0)

#define RX_DMA_INT_DESC_WRWB_EN_REG		0x05a30
#define  RX_DMA_INT_DESC_WRWB_EN		__BIT(2)
#define  RX_DMA_INT_DESC_MODERATE_EN		__BIT(3)

/* RX_INTR_MODERATION_CTL_REG[AQ_RINGS_NUM] 0x5a40-0x5ac0 */
#define RX_INTR_MODERATION_CTL_REG(i)		(0x5a40 + (i) * 4)
#define  RX_INTR_MODERATION_CTL_EN		__BIT(1)
#define  RX_INTR_MODERATION_CTL_MIN		__BITS(15,8)
#define  RX_INTR_MODERATION_CTL_MAX		__BITS(24,16)

/* RX_DMA_DESC_*[AQ_RINGS_NUM] 0x5b00-0x5f00 */
#define RX_DMA_DESC_BASE_ADDRLSW_REG(i)		(0x5b00 + (i) * 0x20)
#define RX_DMA_DESC_BASE_ADDRMSW_REG(i)		(0x5b04 + (i) * 0x20)
#define RX_DMA_DESC_REG(i)			(0x5b08 + (i) * 0x20)
#define  RX_DMA_DESC_LEN			__BITS(12,3)	/* RXD_NUM/8 */
#define  RX_DMA_DESC_RESET			__BIT(25)
#define  RX_DMA_DESC_HEADER_SPLIT		__BIT(28)
#define  RX_DMA_DESC_VLAN_STRIP			__BIT(29)
#define  RX_DMA_DESC_EN				__BIT(31)
#define RX_DMA_DESC_HEAD_PTR_REG(i)		(0x5b0c + (i) * 0x20)
#define  RX_DMA_DESC_HEAD_PTR			__BITS(12,0)
#define RX_DMA_DESC_TAIL_PTR_REG(i)		(0x5b10 + (i) * 0x20)
#define RX_DMA_DESC_BUFSIZE_REG(i)		(0x5b18 + (i) * 0x20)
#define  RX_DMA_DESC_BUFSIZE_DATA		__BITS(4,0)
#define  RX_DMA_DESC_BUFSIZE_HDR		__BITS(12,8)

/* RX_DMA_DCAD_REG[AQ_RINGS_NUM] 0x6100-0x6180 */
#define RX_DMA_DCAD_REG(i)			(0x6100 + (i) * 4)
#define  RX_DMA_DCAD_CPUID			__BITS(7,0)
#define  RX_DMA_DCAD_PAYLOAD_EN			__BIT(29)
#define  RX_DMA_DCAD_HEADER_EN			__BIT(30)
#define  RX_DMA_DCAD_DESC_EN			__BIT(31)

#define RX_DMA_DCA_REG				0x6180
#define  RX_DMA_DCA_EN				__BIT(31)
#define  RX_DMA_DCA_MODE			__BITS(3,0)

/* counters */
#define RX_DMA_GOOD_PKT_COUNTERLSW		0x6800
#define RX_DMA_GOOD_OCTET_COUNTERLSW		0x6808
#define RX_DMA_DROP_PKT_CNT_REG			0x6818
#define RX_DMA_COALESCED_PKT_CNT_REG		0x6820

#define TX_SYSCONTROL_REG			0x7000
#define  TX_SYSCONTROL_TPB_DMA_LOOPBACK		__BIT(6)
#define  TX_SYSCONTROL_TPO_PKT_LOOPBACK		__BIT(7)
#define  TX_SYSCONTROL_RESET_DIS		__BIT(29)

#define TX_TPO2_REG				0x7040
#define  TX_TPO2_EN				__BIT(16)

#define TPS_DESC_VM_ARB_MODE_REG		0x7300
#define  TPS_DESC_VM_ARB_MODE			__BIT(0)
#define TPS_DESC_RATE_REG			0x7310
#define  TPS_DESC_RATE_TA_RST			__BIT(31)
#define  TPS_DESC_RATE_LIM			__BITS(10,0)
#define TPS_DESC_TC_ARB_MODE_REG		0x7200
#define  TPS_DESC_TC_ARB_MODE			__BITS(1,0)
#define TPS_DATA_TC_ARB_MODE_REG		0x7100
#define  TPS_DATA_TC_ARB_MODE			__BIT(0)

/* TPS_DATA_TCT_REG[AQ_TRAFFICCLASS_NUM] 0x7110-0x7130 */
#define TPS_DATA_TCT_REG(i)			(0x7110 + (i) * 4)
#define  TPS_DATA_TCT_CREDIT_MAX		__BITS(16,27)
#define  TPS_DATA_TCT_WEIGHT			__BITS(8,0)
/* TPS_DATA_TCT_REG[AQ_TRAFFICCLASS_NUM] 0x7210-0x7230 */
#define TPS_DESC_TCT_REG(i)			(0x7210 + (i) * 4)
#define  TPS_DESC_TCT_CREDIT_MAX		__BITS(16,27)
#define  TPS_DESC_TCT_WEIGHT			__BITS(8,0)

#define AQ_HW_TXBUF_MAX		160
#define AQ_HW_RXBUF_MAX		320

#define TPO_HWCSUM_REG				0x7800
#define  TPO_HWCSUM_IP4CSUM_EN			__BIT(1)
#define  TPO_HWCSUM_L4CSUM_EN			__BIT(0) /* TCP/UDP/SCTP */

#define TDM_LSO_EN_REG				0x7810

#define THM_LSO_TCP_FLAG1_REG			0x7820
#define  THM_LSO_TCP_FLAG1_FIRST		__BITS(11,0)
#define  THM_LSO_TCP_FLAG1_MID			__BITS(27,16)
#define THM_LSO_TCP_FLAG2_REG			0x7824
#define  THM_LSO_TCP_FLAG2_LAST			__BITS(11,0)

#define TPB_TX_BUF_REG				0x7900
#define  TPB_TX_BUF_EN				__BIT(0)
#define  TPB_TX_BUF_SCP_INS_EN			__BIT(2)
#define  TPB_TX_BUF_TC_MODE_EN			__BIT(8)

/* TPB_TXB_BUFSIZE_REG[AQ_TRAFFICCLASS_NUM] 0x7910-7990 */
#define TPB_TXB_BUFSIZE_REG(i)			(0x7910 + (i) * 0x10)
#define  TPB_TXB_BUFSIZE			__BITS(7,0)
#define TPB_TXB_THRESH_REG(i)			(0x7914 + (i) * 0x10)
#define  TPB_TXB_THRESH_HI			__BITS(16,28)
#define  TPB_TXB_THRESH_LO			__BITS(12,0)

#define AQ_HW_TX_DMA_TOTAL_REQ_LIMIT_REG	0x7b20
#define TX_DMA_INT_DESC_WRWB_EN_REG		0x7b40
#define  TX_DMA_INT_DESC_WRWB_EN		__BIT(1)
#define  TX_DMA_INT_DESC_MODERATE_EN		__BIT(4)

/* TX_DMA_DESC_*[AQ_RINGS_NUM] 0x7c00-0x8400 */
#define TX_DMA_DESC_BASE_ADDRLSW_REG(i)		(0x7c00 + (i) * 0x40)
#define TX_DMA_DESC_BASE_ADDRMSW_REG(i)		(0x7c04 + (i) * 0x40)
#define TX_DMA_DESC_REG(i)			(0x7c08 + (i) * 0x40)
#define  TX_DMA_DESC_LEN			__BITS(12, 3)	/* TXD_NUM/8 */
#define  TX_DMA_DESC_EN				__BIT(31)
#define TX_DMA_DESC_HEAD_PTR_REG(i)		(0x7c0c + (i) * 0x40)
#define  TX_DMA_DESC_HEAD_PTR			__BITS(12,0)
#define TX_DMA_DESC_TAIL_PTR_REG(i)		(0x7c10 + (i) * 0x40)
#define TX_DMA_DESC_WRWB_THRESH_REG(i)		(0x7c18 + (i) * 0x40)
#define  TX_DMA_DESC_WRWB_THRESH		__BITS(14,8)

/* TDM_DCAD_REG[AQ_RINGS_NUM] 0x8400-0x8480 */
#define TDM_DCAD_REG(i)				(0x8400 + (i) * 4)
#define  TDM_DCAD_CPUID				__BITS(7,0)
#define  TDM_DCAD_CPUID_EN			__BIT(31)

#define TDM_DCA_REG				0x8480
#define  TDM_DCA_EN				__BIT(31)
#define  TDM_DCA_MODE				__BITS(3,0)

/* TX_INTR_MODERATION_CTL_REG[AQ_RINGS_NUM] 0x8980-0x8a00 */
#define TX_INTR_MODERATION_CTL_REG(i)		(0x8980 + (i) * 4)
#define  TX_INTR_MODERATION_CTL_EN		__BIT(1)
#define  TX_INTR_MODERATION_CTL_MIN		__BITS(15,8)
#define  TX_INTR_MODERATION_CTL_MAX		__BITS(24,16)

#define FW1X_CTRL_10G				__BIT(0)
#define FW1X_CTRL_5G				__BIT(1)
#define FW1X_CTRL_5GSR				__BIT(2)
#define FW1X_CTRL_2G5				__BIT(3)
#define FW1X_CTRL_1G				__BIT(4)
#define FW1X_CTRL_100M				__BIT(5)

#define FW2X_CTRL_10BASET_HD			__BIT(0)
#define FW2X_CTRL_10BASET_FD			__BIT(1)
#define FW2X_CTRL_100BASETX_HD			__BIT(2)
#define FW2X_CTRL_100BASET4_HD			__BIT(3)
#define FW2X_CTRL_100BASET2_HD			__BIT(4)
#define FW2X_CTRL_100BASETX_FD			__BIT(5)
#define FW2X_CTRL_100BASET2_FD			__BIT(6)
#define FW2X_CTRL_1000BASET_HD			__BIT(7)
#define FW2X_CTRL_1000BASET_FD			__BIT(8)
#define FW2X_CTRL_2P5GBASET_FD			__BIT(9)
#define FW2X_CTRL_5GBASET_FD			__BIT(10)
#define FW2X_CTRL_10GBASET_FD			__BIT(11)
#define FW2X_CTRL_RESERVED1			__BIT(32)
#define FW2X_CTRL_10BASET_EEE			__BIT(33)
#define FW2X_CTRL_RESERVED2			__BIT(34)
#define FW2X_CTRL_PAUSE				__BIT(35)
#define FW2X_CTRL_ASYMMETRIC_PAUSE		__BIT(36)
#define FW2X_CTRL_100BASETX_EEE			__BIT(37)
#define FW2X_CTRL_RESERVED3			__BIT(38)
#define FW2X_CTRL_RESERVED4			__BIT(39)
#define FW2X_CTRL_1000BASET_FD_EEE		__BIT(40)
#define FW2X_CTRL_2P5GBASET_FD_EEE		__BIT(41)
#define FW2X_CTRL_5GBASET_FD_EEE		__BIT(42)
#define FW2X_CTRL_10GBASET_FD_EEE		__BIT(43)
#define FW2X_CTRL_RESERVED5			__BIT(44)
#define FW2X_CTRL_RESERVED6			__BIT(45)
#define FW2X_CTRL_RESERVED7			__BIT(46)
#define FW2X_CTRL_RESERVED8			__BIT(47)
#define FW2X_CTRL_RESERVED9			__BIT(48)
#define FW2X_CTRL_CABLE_DIAG			__BIT(49)
#define FW2X_CTRL_TEMPERATURE			__BIT(50)
#define FW2X_CTRL_DOWNSHIFT			__BIT(51)
#define FW2X_CTRL_PTP_AVB_EN			__BIT(52)
#define FW2X_CTRL_MEDIA_DETECT			__BIT(53)
#define FW2X_CTRL_LINK_DROP			__BIT(54)
#define FW2X_CTRL_SLEEP_PROXY			__BIT(55)
#define FW2X_CTRL_WOL				__BIT(56)
#define FW2X_CTRL_MAC_STOP			__BIT(57)
#define FW2X_CTRL_EXT_LOOPBACK			__BIT(58)
#define FW2X_CTRL_INT_LOOPBACK			__BIT(59)
#define FW2X_CTRL_EFUSE_AGENT			__BIT(60)
#define FW2X_CTRL_WOL_TIMER			__BIT(61)
#define FW2X_CTRL_STATISTICS			__BIT(62)
#define FW2X_CTRL_TRANSACTION_ID		__BIT(63)

#define FW2X_SNPRINTB			\
	"\177\020"			\
	"b\x23" "PAUSE\0"		\
	"b\x24" "ASYMMETRIC-PAUSE\0"	\
	"b\x31" "CABLE-DIAG\0"		\
	"b\x32" "TEMPERATURE\0"		\
	"b\x33" "DOWNSHIFT\0"		\
	"b\x34" "PTP-AVB\0"		\
	"b\x35" "MEDIA-DETECT\0"	\
	"b\x36" "LINK-DROP\0"		\
	"b\x37" "SLEEP-PROXY\0"		\
	"b\x38" "WOL\0"			\
	"b\x39" "MAC-STOP\0"		\
	"b\x3a" "EXT-LOOPBACK\0"	\
	"b\x3b" "INT-LOOPBACK\0"	\
	"b\x3c" "EFUSE-AGENT\0"		\
	"b\x3d" "WOL-TIMER\0"		\
	"b\x3e" "STATISTICS\0"		\
	"b\x3f" "TRANSACTION-ID\0"	\
	"\0"

#define FW2X_CTRL_RATE_100M			FW2X_CTRL_100BASETX_FD
#define FW2X_CTRL_RATE_1G			FW2X_CTRL_1000BASET_FD
#define FW2X_CTRL_RATE_2G5			FW2X_CTRL_2P5GBASET_FD
#define FW2X_CTRL_RATE_5G			FW2X_CTRL_5GBASET_FD
#define FW2X_CTRL_RATE_10G			FW2X_CTRL_10GBASET_FD
#define FW2X_CTRL_RATE_MASK		\
	(FW2X_CTRL_RATE_100M |		\
	 FW2X_CTRL_RATE_1G |		\
	 FW2X_CTRL_RATE_2G5 |		\
	 FW2X_CTRL_RATE_5G |		\
	 FW2X_CTRL_RATE_10G)
#define FW2X_CTRL_EEE_MASK		\
	(FW2X_CTRL_10BASET_EEE |	\
	 FW2X_CTRL_100BASETX_EEE |	\
	 FW2X_CTRL_1000BASET_FD_EEE |	\
	 FW2X_CTRL_2P5GBASET_FD_EEE |	\
	 FW2X_CTRL_5GBASET_FD_EEE |	\
	 FW2X_CTRL_10GBASET_FD_EEE)

typedef enum aq_fw_bootloader_mode {
	FW_BOOT_MODE_UNKNOWN = 0,
	FW_BOOT_MODE_FLB,
	FW_BOOT_MODE_RBL_FLASH,
	FW_BOOT_MODE_RBL_HOST_BOOTLOAD
} aq_fw_bootloader_mode_t;

#define AQ_WRITE_REG(sc, reg, val)				\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

#define AQ_READ_REG(sc, reg)					\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))

#define AQ_READ64_REG(sc, reg)					\
	((uint64_t)AQ_READ_REG(sc, reg) |			\
	(((uint64_t)AQ_READ_REG(sc, (reg) + 4)) << 32))

#define AQ_WRITE64_REG(sc, reg, val)				\
	do {							\
		AQ_WRITE_REG(sc, reg, (uint32_t)val);		\
		AQ_WRITE_REG(sc, reg + 4, (uint32_t)(val >> 32)); \
	} while (/* CONSTCOND */0)

#define AQ_READ_REG_BIT(sc, reg, mask)				\
	__SHIFTOUT(AQ_READ_REG(sc, reg), mask)

#define AQ_WRITE_REG_BIT(sc, reg, mask, val)			\
	do {							\
		uint32_t _v;					\
		_v = AQ_READ_REG((sc), (reg));			\
		_v &= ~(mask);					\
		if ((val) != 0)					\
			_v |= __SHIFTIN((val), (mask));		\
		AQ_WRITE_REG((sc), (reg), _v);			\
	} while (/* CONSTCOND */ 0)

#define WAIT_FOR(expr, us, n, errp)				\
	do {							\
		unsigned int _n;				\
		for (_n = n; (!(expr)) && _n != 0; --_n) {	\
			delay((us));				\
		}						\
		if ((errp != NULL)) {				\
			if (_n == 0)				\
				*(errp) = ETIMEDOUT;		\
			else					\
				*(errp) = 0;			\
		}						\
	} while (/* CONSTCOND */ 0)

#define msec_delay(x)	DELAY(1000 * (x))

typedef struct aq_mailbox_header {
	uint32_t version;
	uint32_t transaction_id;
	int32_t error;
} __packed __aligned(4) aq_mailbox_header_t;

typedef struct aq_hw_stats_s {
	uint32_t uprc;
	uint32_t mprc;
	uint32_t bprc;
	uint32_t erpt;
	uint32_t uptc;
	uint32_t mptc;
	uint32_t bptc;
	uint32_t erpr;
	uint32_t mbtc;
	uint32_t bbtc;
	uint32_t mbrc;
	uint32_t bbrc;
	uint32_t ubrc;
	uint32_t ubtc;
	uint32_t ptc;
	uint32_t prc;
	uint32_t dpc;	/* not exists in fw2x_msm_statistics */
	uint32_t cprc;	/* not exists in fw2x_msm_statistics */
} __packed __aligned(4) aq_hw_stats_s_t;

typedef struct fw1x_mailbox {
	aq_mailbox_header_t header;
	aq_hw_stats_s_t msm;
} __packed __aligned(4) fw1x_mailbox_t;

typedef struct fw2x_msm_statistics {
	uint32_t uprc;
	uint32_t mprc;
	uint32_t bprc;
	uint32_t erpt;
	uint32_t uptc;
	uint32_t mptc;
	uint32_t bptc;
	uint32_t erpr;
	uint32_t mbtc;
	uint32_t bbtc;
	uint32_t mbrc;
	uint32_t bbrc;
	uint32_t ubrc;
	uint32_t ubtc;
	uint32_t ptc;
	uint32_t prc;
} __packed __aligned(4) fw2x_msm_statistics_t;

typedef struct fw2x_phy_cable_diag_data {
	uint32_t lane_data[4];
} __packed __aligned(4) fw2x_phy_cable_diag_data_t;

typedef struct fw2x_capabilities {
	uint32_t caps_lo;
	uint32_t caps_hi;
} __packed __aligned(4) fw2x_capabilities_t;

typedef struct fw2x_mailbox {		/* struct fwHostInterface */
	aq_mailbox_header_t header;
	fw2x_msm_statistics_t msm;	/* msmStatistics_t msm; */

	uint32_t phy_info1;
#define PHYINFO1_FAULT_CODE	__BITS(31,16)
#define PHYINFO1_PHY_H_BIT	__BITS(0,15)
	uint32_t phy_info2;
#define PHYINFO2_TEMPERATURE	__BITS(15,0)
#define PHYINFO2_CABLE_LEN	__BITS(23,16)

	fw2x_phy_cable_diag_data_t diag_data;
	uint32_t reserved[8];

	fw2x_capabilities_t caps;

	/* ... */
} __packed __aligned(4) fw2x_mailbox_t;

typedef enum aq_link_speed {
	AQ_LINK_NONE	= 0,
	AQ_LINK_100M	= (1 << 0),
	AQ_LINK_1G	= (1 << 1),
	AQ_LINK_2G5	= (1 << 2),
	AQ_LINK_5G	= (1 << 3),
	AQ_LINK_10G	= (1 << 4)
} aq_link_speed_t;
#define AQ_LINK_ALL	(AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5 | \
			 AQ_LINK_5G | AQ_LINK_10G )
#define AQ_LINK_AUTO	AQ_LINK_ALL

typedef enum aq_link_fc {
	AQ_FC_NONE = 0,
	AQ_FC_RX = __BIT(0),
	AQ_FC_TX = __BIT(1),
	AQ_FC_ALL = (AQ_FC_RX | AQ_FC_TX)
} aq_link_fc_t;

typedef enum aq_link_eee {
	AQ_EEE_DISABLE = 0,
	AQ_EEE_ENABLE = 1
} aq_link_eee_t;

typedef enum aq_hw_fw_mpi_state {
	MPI_DEINIT	= 0,
	MPI_RESET	= 1,
	MPI_INIT	= 2,
	MPI_POWER	= 4
} aq_hw_fw_mpi_state_t;

enum aq_media_type {
	AQ_MEDIA_TYPE_UNKNOWN = 0,
	AQ_MEDIA_TYPE_FIBRE,
	AQ_MEDIA_TYPE_TP
};

struct aq_rx_desc_read {
	uint64_t buf_addr;
	uint64_t hdr_addr;
} __packed __aligned(8);

struct aq_rx_desc_wb {
	uint32_t type;
#define RXDESC_TYPE_RSSTYPE		__BITS(3,0)
#define  RXDESC_TYPE_RSSTYPE_NONE		0
#define  RXDESC_TYPE_RSSTYPE_IPV4		2
#define  RXDESC_TYPE_RSSTYPE_IPV6		3
#define  RXDESC_TYPE_RSSTYPE_IPV4_TCP		4
#define  RXDESC_TYPE_RSSTYPE_IPV6_TCP		5
#define  RXDESC_TYPE_RSSTYPE_IPV4_UDP		6
#define  RXDESC_TYPE_RSSTYPE_IPV6_UDP		7
#define RXDESC_TYPE_PKTTYPE_ETHER	__BITS(5,4)
#define  RXDESC_TYPE_PKTTYPE_ETHER_IPV4		0
#define  RXDESC_TYPE_PKTTYPE_ETHER_IPV6		1
#define  RXDESC_TYPE_PKTTYPE_ETHER_OTHERS	2
#define  RXDESC_TYPE_PKTTYPE_ETHER_ARP		3
#define RXDESC_TYPE_PKTTYPE_PROTO	__BITS(8,6)
#define  RXDESC_TYPE_PKTTYPE_PROTO_TCP		0
#define  RXDESC_TYPE_PKTTYPE_PROTO_UDP		1
#define  RXDESC_TYPE_PKTTYPE_PROTO_SCTP		2
#define  RXDESC_TYPE_PKTTYPE_PROTO_ICMP		3
#define  RXDESC_TYPE_PKTTYPE_PROTO_OTHERS	4
#define RXDESC_TYPE_PKTTYPE_VLAN	__BIT(9)
#define RXDESC_TYPE_PKTTYPE_VLAN_DOUBLE	__BIT(10)
#define RXDESC_TYPE_MAC_DMA_ERR		__BIT(12)
#define RXDESC_TYPE_RESERVED		__BITS(18,13)
#define RXDESC_TYPE_IPV4_CSUM_CHECKED	__BIT(19)	/* PKTTYPE_ETHER_IPV4 */
#define RXDESC_TYPE_TCPUDP_CSUM_CHECKED	__BIT(20)
#define RXDESC_TYPE_SPH			__BIT(21)
#define RXDESC_TYPE_HDR_LEN		__BITS(31,22)
	uint32_t rss_hash;
	uint16_t status;
#define RXDESC_STATUS_DD		__BIT(0)
#define RXDESC_STATUS_EOP		__BIT(1)
#define RXDESC_STATUS_MACERR		__BIT(2)
#define RXDESC_STATUS_IPV4_CSUM_NG	__BIT(3)
#define RXDESC_STATUS_TCPUDP_CSUM_ERROR	__BIT(4)
#define RXDESC_STATUS_TCPUDP_CSUM_OK	__BIT(5)

#define RXDESC_STATUS_STAT		__BITS(2,5)
#define RXDESC_STATUS_ESTAT		__BITS(6,11)
#define RXDESC_STATUS_RSC_CNT		__BITS(12,15)
	uint16_t pkt_len;
	uint16_t next_desc_ptr;
	uint16_t vlan;
} __packed __aligned(4);

typedef union aq_rx_desc {
	struct aq_rx_desc_read read;
	struct aq_rx_desc_wb wb;
} __packed __aligned(8) aq_rx_desc_t;

typedef struct aq_tx_desc {
	uint64_t buf_addr;
	uint32_t ctl1;
#define AQ_TXDESC_CTL1_TYPE_MASK	0x00000003
#define AQ_TXDESC_CTL1_TYPE_TXD		0x00000001
#define AQ_TXDESC_CTL1_TYPE_TXC		0x00000002
#define AQ_TXDESC_CTL1_BLEN		__BITS(19,4)	/* TXD */
#define AQ_TXDESC_CTL1_DD		__BIT(20)	/* TXD */
#define AQ_TXDESC_CTL1_EOP		__BIT(21)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_VLAN		__BIT(22)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_FCS		__BIT(23)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_IP4CSUM	__BIT(24)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_L4CSUM	__BIT(25)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_LSO		__BIT(26)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_WB		__BIT(27)	/* TXD */
#define AQ_TXDESC_CTL1_CMD_VXLAN	__BIT(28)	/* TXD */
#define AQ_TXDESC_CTL1_VID		__BITS(15,4)	/* TXC */
#define AQ_TXDESC_CTL1_LSO_IPV6		__BIT(21)	/* TXC */
#define AQ_TXDESC_CTL1_LSO_TCP		__BIT(22)	/* TXC */
	uint32_t ctl2;
#define AQ_TXDESC_CTL2_LEN		__BITS(31,14)
#define AQ_TXDESC_CTL2_CTX_EN		__BIT(13)
#define AQ_TXDESC_CTL2_CTX_IDX		__BIT(12)
} __packed __aligned(8) aq_tx_desc_t;

struct aq_txring {
	struct aq_softc *txr_sc;
	int txr_index;
	kmutex_t txr_mutex;
	bool txr_active;

	pcq_t *txr_pcq;
	void *txr_softint;

	aq_tx_desc_t *txr_txdesc;	/* aq_tx_desc_t[AQ_TXD_NUM] */
	bus_dmamap_t txr_txdesc_dmamap;
	bus_dma_segment_t txr_txdesc_seg[1];
	bus_size_t txr_txdesc_size;

	struct {
		struct mbuf *m;
		bus_dmamap_t dmamap;
	} txr_mbufs[AQ_TXD_NUM];
	unsigned int txr_prodidx;
	unsigned int txr_considx;
	int txr_nfree;
};

struct aq_rxring {
	struct aq_softc *rxr_sc;
	int rxr_index;
	kmutex_t rxr_mutex;
	bool rxr_active;

	aq_rx_desc_t *rxr_rxdesc;	/* aq_rx_desc_t[AQ_RXD_NUM] */
	bus_dmamap_t rxr_rxdesc_dmamap;
	bus_dma_segment_t rxr_rxdesc_seg[1];
	bus_size_t rxr_rxdesc_size;
	struct {
		struct mbuf *m;
		bus_dmamap_t dmamap;
	} rxr_mbufs[AQ_RXD_NUM];
	unsigned int rxr_readidx;
};

struct aq_queue {
	struct aq_softc *sc;
	struct aq_txring txring;
	struct aq_rxring rxring;
};

struct aq_softc;
struct aq_firmware_ops {
	int (*reset)(struct aq_softc *);
	int (*set_mode)(struct aq_softc *, aq_hw_fw_mpi_state_t,
	    aq_link_speed_t, aq_link_fc_t, aq_link_eee_t);
	int (*get_mode)(struct aq_softc *, aq_hw_fw_mpi_state_t *,
	    aq_link_speed_t *, aq_link_fc_t *, aq_link_eee_t *);
	int (*get_stats)(struct aq_softc *, aq_hw_stats_s_t *);
#if NSYSMON_ENVSYS > 0
	int (*get_temperature)(struct aq_softc *, uint32_t *);
#endif
};

#ifdef AQ_EVENT_COUNTERS
#define AQ_EVCNT_DECL(name)						\
	char sc_evcount_##name##_name[32];				\
	struct evcnt sc_evcount_##name##_ev;
#define AQ_EVCNT_ATTACH(sc, name, desc, evtype)				\
	do {								\
		snprintf((sc)->sc_evcount_##name##_name,		\
		    sizeof((sc)->sc_evcount_##name##_name),		\
		    "%s", desc);					\
		evcnt_attach_dynamic(&(sc)->sc_evcount_##name##_ev,	\
		    (evtype), NULL, device_xname((sc)->sc_dev),		\
		    (sc)->sc_evcount_##name##_name);			\
	} while (/*CONSTCOND*/0)
#define AQ_EVCNT_ATTACH_MISC(sc, name, desc)				\
	AQ_EVCNT_ATTACH(sc, name, desc, EVCNT_TYPE_MISC)
#define AQ_EVCNT_DETACH(sc, name)					\
	evcnt_detach(&(sc)->sc_evcount_##name##_ev)
#define AQ_EVCNT_ADD(sc, name, val)					\
	((sc)->sc_evcount_##name##_ev.ev_count += (val))
#endif /* AQ_EVENT_COUNTERS */

#define AQ_LOCK(sc)		mutex_enter(&(sc)->sc_mutex);
#define AQ_UNLOCK(sc)		mutex_exit(&(sc)->sc_mutex);

/* lock for FW2X_MPI_{CONTROL,STATE]_REG read-modify-write */
#define AQ_MPI_LOCK(sc)		mutex_enter(&(sc)->sc_mpi_mutex);
#define AQ_MPI_UNLOCK(sc)	mutex_exit(&(sc)->sc_mpi_mutex);


struct aq_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_iosize;
	bus_dma_tag_t sc_dmat;

	void *sc_ihs[AQ_NINTR_MAX];
	pci_intr_handle_t *sc_intrs;

	int sc_tx_irq[AQ_RSSQUEUE_MAX];
	int sc_rx_irq[AQ_RSSQUEUE_MAX];
	int sc_linkstat_irq;
	bool sc_use_txrx_independent_intr;
	bool sc_poll_linkstat;
	bool sc_detect_linkstat;

#if NSYSMON_ENVSYS > 0
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor_temp;
#endif

	callout_t sc_tick_ch;

	int sc_nintrs;
	bool sc_msix;

	struct aq_queue sc_queue[AQ_RSSQUEUE_MAX];
	int sc_nqueues;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	uint16_t sc_product;
	uint16_t sc_revision;

	kmutex_t sc_mutex;
	kmutex_t sc_mpi_mutex;

	const struct aq_firmware_ops *sc_fw_ops;
	uint64_t sc_fw_caps;
	enum aq_media_type sc_media_type;
	aq_link_speed_t sc_available_rates;

	aq_link_speed_t sc_link_rate;
	aq_link_fc_t sc_link_fc;
	aq_link_eee_t sc_link_eee;

	uint32_t sc_fw_version;
#define FW_VERSION_MAJOR(sc)	(((sc)->sc_fw_version >> 24) & 0xff)
#define FW_VERSION_MINOR(sc)	(((sc)->sc_fw_version >> 16) & 0xff)
#define FW_VERSION_BUILD(sc)	((sc)->sc_fw_version & 0xffff)
	uint32_t sc_features;
#define FEATURES_MIPS		0x00000001
#define FEATURES_TPO2		0x00000002
#define FEATURES_RPF2		0x00000004
#define FEATURES_MPI_AQ		0x00000008
#define FEATURES_REV_A0		0x10000000
#define FEATURES_REV_A		(FEATURES_REV_A0)
#define FEATURES_REV_B0		0x20000000
#define FEATURES_REV_B1		0x40000000
#define FEATURES_REV_B		(FEATURES_REV_B0|FEATURES_REV_B1)
	uint32_t sc_max_mtu;
	uint32_t sc_mbox_addr;

	bool sc_rbl_enabled;
	bool sc_fast_start_enabled;
	bool sc_flash_present;

	bool sc_intr_moderation_enable;
	bool sc_rss_enable;

	struct ethercom sc_ethercom;
	struct ether_addr sc_enaddr;
	struct ifmedia sc_media;
	int sc_ec_capenable;		/* last ec_capenable */
	unsigned short sc_if_flags;	/* last if_flags */

#ifdef AQ_EVENT_COUNTERS
	aq_hw_stats_s_t sc_statistics[2];
	int sc_statistics_idx;
	bool sc_poll_statistics;

	AQ_EVCNT_DECL(uprc);
	AQ_EVCNT_DECL(mprc);
	AQ_EVCNT_DECL(bprc);
	AQ_EVCNT_DECL(erpt);
	AQ_EVCNT_DECL(uptc);
	AQ_EVCNT_DECL(mptc);
	AQ_EVCNT_DECL(bptc);
	AQ_EVCNT_DECL(erpr);
	AQ_EVCNT_DECL(mbtc);
	AQ_EVCNT_DECL(bbtc);
	AQ_EVCNT_DECL(mbrc);
	AQ_EVCNT_DECL(bbrc);
	AQ_EVCNT_DECL(ubrc);
	AQ_EVCNT_DECL(ubtc);
	AQ_EVCNT_DECL(ptc);
	AQ_EVCNT_DECL(prc);
	AQ_EVCNT_DECL(dpc);
	AQ_EVCNT_DECL(cprc);
#endif
};

static int aq_match(device_t, cfdata_t, void *);
static void aq_attach(device_t, device_t, void *);
static int aq_detach(device_t, int);

static int aq_setup_msix(struct aq_softc *, struct pci_attach_args *, int,
    bool, bool);
static int aq_setup_legacy(struct aq_softc *, struct pci_attach_args *,
    pci_intr_type_t);
static int aq_establish_msix_intr(struct aq_softc *, bool, bool);

static int aq_ifmedia_change(struct ifnet * const);
static void aq_ifmedia_status(struct ifnet * const, struct ifmediareq *);
static int aq_vlan_cb(struct ethercom *ec, uint16_t vid, bool set);
static int aq_ifflags_cb(struct ethercom *);
static int aq_init(struct ifnet *);
static void aq_send_common_locked(struct ifnet *, struct aq_softc *,
    struct aq_txring *, bool);
static int aq_transmit(struct ifnet *, struct mbuf *);
static void aq_deferred_transmit(void *);
static void aq_start(struct ifnet *);
static void aq_stop(struct ifnet *, int);
static void aq_watchdog(struct ifnet *);
static int aq_ioctl(struct ifnet *, unsigned long, void *);

static int aq_txrx_rings_alloc(struct aq_softc *);
static void aq_txrx_rings_free(struct aq_softc *);
static int aq_tx_pcq_alloc(struct aq_softc *, struct aq_txring *);
static void aq_tx_pcq_free(struct aq_softc *, struct aq_txring *);

static void aq_initmedia(struct aq_softc *);
static void aq_enable_intr(struct aq_softc *, bool, bool);

#if NSYSMON_ENVSYS > 0
static void aq_temp_refresh(struct sysmon_envsys *, envsys_data_t *);
#endif
static void aq_tick(void *);
static int aq_legacy_intr(void *);
static int aq_link_intr(void *);
static int aq_txrx_intr(void *);
static int aq_tx_intr(void *);
static int aq_rx_intr(void *);

static int aq_set_linkmode(struct aq_softc *, aq_link_speed_t, aq_link_fc_t,
    aq_link_eee_t);
static int aq_get_linkmode(struct aq_softc *, aq_link_speed_t *, aq_link_fc_t *,
    aq_link_eee_t *);

static int aq_fw_reset(struct aq_softc *);
static int aq_fw_version_init(struct aq_softc *);
static int aq_hw_init(struct aq_softc *);
static int aq_hw_init_ucp(struct aq_softc *);
static int aq_hw_reset(struct aq_softc *);
static int aq_fw_downld_dwords(struct aq_softc *, uint32_t, uint32_t *,
    uint32_t);
static int aq_get_mac_addr(struct aq_softc *);
static int aq_init_rss(struct aq_softc *);
static int aq_set_capability(struct aq_softc *);

static int fw1x_reset(struct aq_softc *);
static int fw1x_set_mode(struct aq_softc *, aq_hw_fw_mpi_state_t,
    aq_link_speed_t, aq_link_fc_t, aq_link_eee_t);
static int fw1x_get_mode(struct aq_softc *, aq_hw_fw_mpi_state_t *,
    aq_link_speed_t *, aq_link_fc_t *, aq_link_eee_t *);
static int fw1x_get_stats(struct aq_softc *, aq_hw_stats_s_t *);

static int fw2x_reset(struct aq_softc *);
static int fw2x_set_mode(struct aq_softc *, aq_hw_fw_mpi_state_t,
    aq_link_speed_t, aq_link_fc_t, aq_link_eee_t);
static int fw2x_get_mode(struct aq_softc *, aq_hw_fw_mpi_state_t *,
    aq_link_speed_t *, aq_link_fc_t *, aq_link_eee_t *);
static int fw2x_get_stats(struct aq_softc *, aq_hw_stats_s_t *);
#if NSYSMON_ENVSYS > 0
static int fw2x_get_temperature(struct aq_softc *, uint32_t *);
#endif

static const struct aq_firmware_ops aq_fw1x_ops = {
	.reset = fw1x_reset,
	.set_mode = fw1x_set_mode,
	.get_mode = fw1x_get_mode,
	.get_stats = fw1x_get_stats,
#if NSYSMON_ENVSYS > 0
	.get_temperature = NULL
#endif
};

static const struct aq_firmware_ops aq_fw2x_ops = {
	.reset = fw2x_reset,
	.set_mode = fw2x_set_mode,
	.get_mode = fw2x_get_mode,
	.get_stats = fw2x_get_stats,
#if NSYSMON_ENVSYS > 0
	.get_temperature = fw2x_get_temperature
#endif
};

CFATTACH_DECL3_NEW(aq, sizeof(struct aq_softc),
    aq_match, aq_attach, aq_detach, NULL, NULL, NULL, DVF_DETACH_SHUTDOWN);

static const struct aq_product {
	pci_vendor_id_t aq_vendor;
	pci_product_id_t aq_product;
	const char *aq_name;
	enum aq_media_type aq_media_type;
	aq_link_speed_t aq_available_rates;
} aq_products[] = {
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC100,
	  "Aquantia AQC100 10 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_FIBRE, AQ_LINK_ALL
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC107,
	  "Aquantia AQC107 10 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_ALL
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC108,
	  "Aquantia AQC108 5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5 | AQ_LINK_5G
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC109,
	  "Aquantia AQC109 2.5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC111,
	  "Aquantia AQC111 5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5 | AQ_LINK_5G
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC112,
	  "Aquantia AQC112 2.5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC100S,
	  "Aquantia AQC100S 10 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_FIBRE, AQ_LINK_ALL
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC107S,
	  "Aquantia AQC107S 10 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_ALL
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC108S,
	  "Aquantia AQC108S 5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5 | AQ_LINK_5G
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC109S,
	  "Aquantia AQC109S 2.5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC111S,
	  "Aquantia AQC111S 5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5 | AQ_LINK_5G
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_AQC112S,
	  "Aquantia AQC112S 2.5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_D100,
	  "Aquantia D100 10 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_FIBRE, AQ_LINK_ALL
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_D107,
	  "Aquantia D107 10 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_ALL
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_D108,
	  "Aquantia D108 5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5 | AQ_LINK_5G
	},
	{ PCI_VENDOR_AQUANTIA, PCI_PRODUCT_AQUANTIA_D109,
	  "Aquantia D109 2.5 Gigabit Network Adapter",
	  AQ_MEDIA_TYPE_TP, AQ_LINK_100M | AQ_LINK_1G | AQ_LINK_2G5
	}
};

static const struct aq_product *
aq_lookup(const struct pci_attach_args *pa)
{
	unsigned int i;

	for (i = 0; i < __arraycount(aq_products); i++) {
		if (PCI_VENDOR(pa->pa_id)  == aq_products[i].aq_vendor &&
		    PCI_PRODUCT(pa->pa_id) == aq_products[i].aq_product)
			return &aq_products[i];
	}
	return NULL;
}

static int
aq_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (aq_lookup(pa) != NULL)
		return 1;

	return 0;
}

static void
aq_attach(device_t parent, device_t self, void *aux)
{
	struct aq_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	pci_chipset_tag_t pc;
	pcitag_t tag;
	pcireg_t command, memtype, bar;
	const struct aq_product *aqp;
	int error;

	sc->sc_dev = self;
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_mpi_mutex, MUTEX_DEFAULT, IPL_NET);

	sc->sc_pc = pc = pa->pa_pc;
	sc->sc_pcitag = tag = pa->pa_tag;
	sc->sc_dmat = pci_dma64_available(pa) ? pa->pa_dmat64 : pa->pa_dmat;

	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

	sc->sc_product = PCI_PRODUCT(pa->pa_id);
	sc->sc_revision = PCI_REVISION(pa->pa_class);

	aqp = aq_lookup(pa);
	KASSERT(aqp != NULL);

	pci_aprint_devinfo_fancy(pa, "Ethernet controller", aqp->aq_name, 1);

	bar = pci_conf_read(pc, tag, PCI_BAR(0));
	if ((PCI_MAPREG_MEM_ADDR(bar) == 0) ||
	    (PCI_MAPREG_TYPE(bar) != PCI_MAPREG_TYPE_MEM)) {
		aprint_error_dev(sc->sc_dev, "wrong BAR type\n");
		return;
	}
	memtype = pci_mapreg_type(pc, tag, PCI_BAR(0));
	if (pci_mapreg_map(pa, PCI_BAR(0), memtype, 0, &sc->sc_iot, &sc->sc_ioh,
	    NULL, &sc->sc_iosize) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map register\n");
		return;
	}

	sc->sc_nqueues = MIN(ncpu, AQ_RSSQUEUE_MAX);

	/* max queue num is 8, and must be 2^n */
	if (ncpu >= 8)
		sc->sc_nqueues = 8;
	else if (ncpu >= 4)
		sc->sc_nqueues = 4;
	else if (ncpu >= 2)
		sc->sc_nqueues = 2;
	else
		sc->sc_nqueues = 1;

	int msixcount = pci_msix_count(pa->pa_pc, pa->pa_tag);
#ifndef CONFIG_NO_TXRX_INDEPENDENT
	if (msixcount >= (sc->sc_nqueues * 2 + 1)) {
		/* TX intrs + RX intrs + LINKSTAT intrs */
		sc->sc_use_txrx_independent_intr = true;
		sc->sc_poll_linkstat = false;
		sc->sc_msix = true;
	} else if (msixcount >= (sc->sc_nqueues * 2)) {
		/* TX intrs + RX intrs */
		sc->sc_use_txrx_independent_intr = true;
		sc->sc_poll_linkstat = true;
		sc->sc_msix = true;
	} else
#endif
	if (msixcount >= (sc->sc_nqueues + 1)) {
		/* TX/RX intrs LINKSTAT intrs */
		sc->sc_use_txrx_independent_intr = false;
		sc->sc_poll_linkstat = false;
		sc->sc_msix = true;
	} else if (msixcount >= sc->sc_nqueues) {
		/* TX/RX intrs */
		sc->sc_use_txrx_independent_intr = false;
		sc->sc_poll_linkstat = true;
		sc->sc_msix = true;
	} else {
		/* giving up using MSI-X */
		sc->sc_msix = false;
	}

	/* XXX: on FIBRE, linkstat interrupt does not occur on boot? */
	if (aqp->aq_media_type == AQ_MEDIA_TYPE_FIBRE)
		sc->sc_poll_linkstat = true;

#ifdef AQ_FORCE_POLL_LINKSTAT
	sc->sc_poll_linkstat = true;
#endif

	aprint_debug_dev(sc->sc_dev,
	    "ncpu=%d, pci_msix_count=%d."
	    " allocate %d interrupts for %d%s queues%s\n",
	    ncpu, msixcount,
	    (sc->sc_use_txrx_independent_intr ?
	    (sc->sc_nqueues * 2) : sc->sc_nqueues) +
	    (sc->sc_poll_linkstat ? 0 : 1),
	    sc->sc_nqueues,
	    sc->sc_use_txrx_independent_intr ? "*2" : "",
	    sc->sc_poll_linkstat ? "" : ", and link status");

	if (sc->sc_msix)
		error = aq_setup_msix(sc, pa, sc->sc_nqueues,
		    sc->sc_use_txrx_independent_intr, !sc->sc_poll_linkstat);
	else
		error = ENODEV;

	if (error != 0) {
		/* if MSI-X failed, fallback to MSI with single queue */
		sc->sc_use_txrx_independent_intr = false;
		sc->sc_poll_linkstat = false;
		sc->sc_msix = false;
		sc->sc_nqueues = 1;
		error = aq_setup_legacy(sc, pa, PCI_INTR_TYPE_MSI);
	}
	if (error != 0) {
		/* if MSI failed, fallback to INTx */
		error = aq_setup_legacy(sc, pa, PCI_INTR_TYPE_INTX);
	}
	if (error != 0)
		return;

	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, aq_tick, sc);

	sc->sc_intr_moderation_enable = CONFIG_INTR_MODERATION_ENABLE;

	if (sc->sc_msix && (sc->sc_nqueues > 1))
		sc->sc_rss_enable = true;
	else
		sc->sc_rss_enable = false;

	error = aq_txrx_rings_alloc(sc);
	if (error != 0)
		goto attach_failure;

	error = aq_fw_reset(sc);
	if (error != 0)
		goto attach_failure;

	error = aq_fw_version_init(sc);
	if (error != 0)
		goto attach_failure;

	error = aq_hw_init_ucp(sc);
	if (error < 0)
		goto attach_failure;

	KASSERT(sc->sc_mbox_addr != 0);
	error = aq_hw_reset(sc);
	if (error != 0)
		goto attach_failure;

	aq_get_mac_addr(sc);
	aq_init_rss(sc);

	error = aq_hw_init(sc);	/* initialize and interrupts */
	if (error != 0)
		goto attach_failure;

	sc->sc_media_type = aqp->aq_media_type;
	sc->sc_available_rates = aqp->aq_available_rates;

	sc->sc_ethercom.ec_ifmedia = &sc->sc_media;
	ifmedia_init(&sc->sc_media, IFM_IMASK,
	    aq_ifmedia_change, aq_ifmedia_status);
	aq_initmedia(sc);

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_baudrate = IF_Gbps(10);
	ifp->if_init = aq_init;
	ifp->if_ioctl = aq_ioctl;
	if (sc->sc_msix && (sc->sc_nqueues > 1))
		ifp->if_transmit = aq_transmit;
	ifp->if_start = aq_start;
	ifp->if_stop = aq_stop;
	ifp->if_watchdog = aq_watchdog;
	IFQ_SET_READY(&ifp->if_snd);

	/* initialize capabilities */
	sc->sc_ethercom.ec_capabilities = 0;
	sc->sc_ethercom.ec_capenable = 0;
#if notyet
	/* TODO */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_EEE;
#endif
	sc->sc_ethercom.ec_capabilities |=
	    ETHERCAP_JUMBO_MTU |
	    ETHERCAP_VLAN_MTU |
	    ETHERCAP_VLAN_HWTAGGING |
	    ETHERCAP_VLAN_HWFILTER;
	sc->sc_ethercom.ec_capenable |=
	    ETHERCAP_VLAN_HWTAGGING |
	    ETHERCAP_VLAN_HWFILTER;

	ifp->if_capabilities = 0;
	ifp->if_capenable = 0;
#ifdef CONFIG_LRO_SUPPORT
	ifp->if_capabilities |= IFCAP_LRO;
	ifp->if_capenable |= IFCAP_LRO;
#endif
#if notyet
	/* TSO */
	ifp->if_capabilities |= IFCAP_TSOv4 | IFCAP_TSOv6;
#endif

	/* TX hardware checksum offloading */
	ifp->if_capabilities |= IFCAP_CSUM_IPv4_Tx;
	ifp->if_capabilities |= IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv6_Tx;
	ifp->if_capabilities |= IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv6_Tx;
	/* RX hardware checksum offloading */
	ifp->if_capabilities |= IFCAP_CSUM_IPv4_Rx;
	ifp->if_capabilities |= IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv6_Rx;
	ifp->if_capabilities |= IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv6_Rx;

	if_initialize(ifp);
	ifp->if_percpuq = if_percpuq_create(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_enaddr.ether_addr_octet);
	ether_set_vlan_cb(&sc->sc_ethercom, aq_vlan_cb);
	ether_set_ifflags_cb(&sc->sc_ethercom, aq_ifflags_cb);
	if_register(ifp);

	aq_enable_intr(sc, true, false);	/* only intr about link */

	/* update media */
	aq_ifmedia_change(ifp);

#if NSYSMON_ENVSYS > 0
	/* temperature monitoring */
	if (sc->sc_fw_ops != NULL && sc->sc_fw_ops->get_temperature != NULL &&
	    (sc->sc_fw_caps & FW2X_CTRL_TEMPERATURE) != 0) {

		sc->sc_sme = sysmon_envsys_create();
		sc->sc_sme->sme_name = device_xname(self);
		sc->sc_sme->sme_cookie = sc;
		sc->sc_sme->sme_flags = 0;
		sc->sc_sme->sme_refresh = aq_temp_refresh;
		sc->sc_sensor_temp.units = ENVSYS_STEMP;
		sc->sc_sensor_temp.state = ENVSYS_SINVALID;
		snprintf(sc->sc_sensor_temp.desc, ENVSYS_DESCLEN, "PHY");

		sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor_temp);
		if (sysmon_envsys_register(sc->sc_sme)) {
			sysmon_envsys_destroy(sc->sc_sme);
			sc->sc_sme = NULL;
			goto attach_failure;
		}

		/*
		 * for unknown reasons, the first call of fw2x_get_temperature()
		 * will always fail (firmware matter?), so run once now.
		 */
		aq_temp_refresh(sc->sc_sme, &sc->sc_sensor_temp);
	}
#endif

#ifdef AQ_EVENT_COUNTERS
	/* get starting statistics values */
	if (sc->sc_fw_ops != NULL && sc->sc_fw_ops->get_stats != NULL &&
	    sc->sc_fw_ops->get_stats(sc, &sc->sc_statistics[0]) == 0) {
		sc->sc_poll_statistics = true;
	}

	AQ_EVCNT_ATTACH_MISC(sc, uprc, "RX unicast packet");
	AQ_EVCNT_ATTACH_MISC(sc, bprc, "RX broadcast packet");
	AQ_EVCNT_ATTACH_MISC(sc, mprc, "RX multicast packet");
	AQ_EVCNT_ATTACH_MISC(sc, erpr, "RX error packet");
	AQ_EVCNT_ATTACH_MISC(sc, ubrc, "RX unicast bytes");
	AQ_EVCNT_ATTACH_MISC(sc, bbrc, "RX broadcast bytes");
	AQ_EVCNT_ATTACH_MISC(sc, mbrc, "RX multicast bytes");
	AQ_EVCNT_ATTACH_MISC(sc, prc, "RX good packet");
	AQ_EVCNT_ATTACH_MISC(sc, uptc, "TX unicast packet");
	AQ_EVCNT_ATTACH_MISC(sc, bptc, "TX broadcast packet");
	AQ_EVCNT_ATTACH_MISC(sc, mptc, "TX multicast packet");
	AQ_EVCNT_ATTACH_MISC(sc, erpt, "TX error packet");
	AQ_EVCNT_ATTACH_MISC(sc, ubtc, "TX unicast bytes");
	AQ_EVCNT_ATTACH_MISC(sc, bbtc, "TX broadcast bytes");
	AQ_EVCNT_ATTACH_MISC(sc, mbtc, "TX multicast bytes");
	AQ_EVCNT_ATTACH_MISC(sc, ptc, "TX good packet");
	AQ_EVCNT_ATTACH_MISC(sc, dpc, "DMA drop packet");
	AQ_EVCNT_ATTACH_MISC(sc, cprc, "RX coalesced packet");
#endif

	return;

 attach_failure:
	aq_detach(self, 0);
}

static int
aq_detach(device_t self, int flags __unused)
{
	struct aq_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i, s;

	if (sc->sc_iosize != 0) {
		if (ifp->if_softc != NULL) {
			s = splnet();
			aq_stop(ifp, 0);
			splx(s);
		}

		for (i = 0; i < AQ_NINTR_MAX; i++) {
			if (sc->sc_ihs[i] != NULL) {
				pci_intr_disestablish(sc->sc_pc, sc->sc_ihs[i]);
				sc->sc_ihs[i] = NULL;
			}
		}
		if (sc->sc_nintrs > 0) {
			pci_intr_release(sc->sc_pc, sc->sc_intrs,
			    sc->sc_nintrs);
			sc->sc_intrs = NULL;
			sc->sc_nintrs = 0;
		}

		aq_txrx_rings_free(sc);

		if (ifp->if_softc != NULL) {
			ether_ifdetach(ifp);
			if_detach(ifp);
		}

		aprint_debug_dev(sc->sc_dev, "%s: bus_space_unmap\n", __func__);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_iosize);
		sc->sc_iosize = 0;
	}

	callout_stop(&sc->sc_tick_ch);

#if NSYSMON_ENVSYS > 0
	if (sc->sc_sme != NULL) {
		/* all sensors associated with this will also be detached */
		sysmon_envsys_unregister(sc->sc_sme);
	}
#endif

#ifdef AQ_EVENT_COUNTERS
	AQ_EVCNT_DETACH(sc, uprc);
	AQ_EVCNT_DETACH(sc, mprc);
	AQ_EVCNT_DETACH(sc, bprc);
	AQ_EVCNT_DETACH(sc, erpt);
	AQ_EVCNT_DETACH(sc, uptc);
	AQ_EVCNT_DETACH(sc, mptc);
	AQ_EVCNT_DETACH(sc, bptc);
	AQ_EVCNT_DETACH(sc, erpr);
	AQ_EVCNT_DETACH(sc, mbtc);
	AQ_EVCNT_DETACH(sc, bbtc);
	AQ_EVCNT_DETACH(sc, mbrc);
	AQ_EVCNT_DETACH(sc, bbrc);
	AQ_EVCNT_DETACH(sc, ubrc);
	AQ_EVCNT_DETACH(sc, ubtc);
	AQ_EVCNT_DETACH(sc, ptc);
	AQ_EVCNT_DETACH(sc, prc);
	AQ_EVCNT_DETACH(sc, dpc);
	AQ_EVCNT_DETACH(sc, cprc);
#endif

	ifmedia_fini(&sc->sc_media);

	mutex_destroy(&sc->sc_mpi_mutex);
	mutex_destroy(&sc->sc_mutex);

	return 0;
}

static int
aq_establish_intr(struct aq_softc *sc, int intno, kcpuset_t *affinity,
    int (*func)(void *), void *arg, const char *xname)
{
	char intrbuf[PCI_INTRSTR_LEN];
	pci_chipset_tag_t pc = sc->sc_pc;
	void *vih;
	const char *intrstr = NULL;

	intrstr = pci_intr_string(pc, sc->sc_intrs[intno], intrbuf,
	    sizeof(intrbuf));

	pci_intr_setattr(pc, &sc->sc_intrs[intno], PCI_INTR_MPSAFE, true);

	vih = pci_intr_establish_xname(pc, sc->sc_intrs[intno],
	    IPL_NET, func, arg, xname);
	if (vih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish MSI-X%s%s for %s\n",
		    intrstr ? " at " : "",
		    intrstr ? intrstr : "", xname);
		return EIO;
	}
	sc->sc_ihs[intno] = vih;

	if (affinity != NULL) {
		/* Round-robin affinity */
		kcpuset_zero(affinity);
		kcpuset_set(affinity, intno % ncpu);
		interrupt_distribute(vih, affinity, NULL);
	}

	return 0;
}

static int
aq_establish_msix_intr(struct aq_softc *sc, bool txrx_independent,
    bool linkintr)
{
	kcpuset_t *affinity;
	int error, intno, i;
	char intr_xname[INTRDEVNAMEBUF];

	kcpuset_create(&affinity, false);

	intno = 0;

	if (txrx_independent) {
		for (i = 0; i < sc->sc_nqueues; i++) {
			snprintf(intr_xname, sizeof(intr_xname), "%s RX%d",
			    device_xname(sc->sc_dev), i);
			sc->sc_rx_irq[i] = intno;
			error = aq_establish_intr(sc, intno++, affinity,
			   aq_rx_intr, &sc->sc_queue[i].rxring, intr_xname);
			if (error != 0)
				goto fail;
		}
		for (i = 0; i < sc->sc_nqueues; i++) {
			snprintf(intr_xname, sizeof(intr_xname), "%s TX%d",
			    device_xname(sc->sc_dev), i);
			sc->sc_tx_irq[i] = intno;
			error = aq_establish_intr(sc, intno++, affinity,
			    aq_tx_intr, &sc->sc_queue[i].txring, intr_xname);
			if (error != 0)
				goto fail;
		}
	} else {
		for (i = 0; i < sc->sc_nqueues; i++) {
			snprintf(intr_xname, sizeof(intr_xname), "%s TXRX%d",
			    device_xname(sc->sc_dev), i);
			sc->sc_rx_irq[i] = intno;
			sc->sc_tx_irq[i] = intno;
			error = aq_establish_intr(sc, intno++, affinity,
			    aq_txrx_intr, &sc->sc_queue[i], intr_xname);
			if (error != 0)
				goto fail;
		}
	}

	if (linkintr) {
		snprintf(intr_xname, sizeof(intr_xname), "%s LINK",
		    device_xname(sc->sc_dev));
		sc->sc_linkstat_irq = intno;
		error = aq_establish_intr(sc, intno++, affinity,
		    aq_link_intr, sc, intr_xname);
		if (error != 0)
			goto fail;
	}

	kcpuset_destroy(affinity);
	return 0;

 fail:
	for (i = 0; i < AQ_NINTR_MAX; i++) {
		if (sc->sc_ihs[i] != NULL) {
			pci_intr_disestablish(sc->sc_pc, sc->sc_ihs[i]);
			sc->sc_ihs[i] = NULL;
		}
	}

	kcpuset_destroy(affinity);
	return ENOMEM;
}

static int
aq_setup_msix(struct aq_softc *sc, struct pci_attach_args *pa, int nqueue,
    bool txrx_independent, bool linkintr)
{
	int error, nintr;

	if (txrx_independent)
		nintr = nqueue * 2;
	else
		nintr = nqueue;

	if (linkintr)
		nintr++;

	error = pci_msix_alloc_exact(pa, &sc->sc_intrs, nintr);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to allocate MSI-X interrupts\n");
		goto fail;
	}

	error = aq_establish_msix_intr(sc, txrx_independent, linkintr);
	if (error == 0) {
		sc->sc_nintrs = nintr;
	} else {
		pci_intr_release(sc->sc_pc, sc->sc_intrs, nintr);
		sc->sc_nintrs = 0;
	}
 fail:
	return error;

}

static int
aq_setup_legacy(struct aq_softc *sc, struct pci_attach_args *pa,
    pci_intr_type_t inttype)
{
	int counts[PCI_INTR_TYPE_SIZE];
	int error, nintr;

	nintr = 1;

	memset(counts, 0, sizeof(counts));
	counts[inttype] = nintr;

	error = pci_intr_alloc(pa, &sc->sc_intrs, counts, inttype);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to allocate%s interrupts\n",
		    (inttype == PCI_INTR_TYPE_MSI) ? " MSI" : "");
		return error;
	}
	error = aq_establish_intr(sc, 0, NULL, aq_legacy_intr, sc,
	    device_xname(sc->sc_dev));
	if (error == 0) {
		sc->sc_nintrs = nintr;
	} else {
		pci_intr_release(sc->sc_pc, sc->sc_intrs, nintr);
		sc->sc_nintrs = 0;
	}
	return error;
}

static void
global_software_reset(struct aq_softc *sc)
{
	uint32_t v;

	AQ_WRITE_REG_BIT(sc, RX_SYSCONTROL_REG, RX_SYSCONTROL_RESET_DIS, 0);
	AQ_WRITE_REG_BIT(sc, TX_SYSCONTROL_REG, TX_SYSCONTROL_RESET_DIS, 0);
	AQ_WRITE_REG_BIT(sc, FW_MPI_RESETCTRL_REG,
	    FW_MPI_RESETCTRL_RESET_DIS, 0);

	v = AQ_READ_REG(sc, AQ_FW_SOFTRESET_REG);
	v &= ~AQ_FW_SOFTRESET_DIS;
	v |= AQ_FW_SOFTRESET_RESET;
	AQ_WRITE_REG(sc, AQ_FW_SOFTRESET_REG, v);
}

static int
mac_soft_reset_rbl(struct aq_softc *sc, aq_fw_bootloader_mode_t *mode)
{
	int timo;

	aprint_debug_dev(sc->sc_dev, "RBL> MAC reset STARTED!\n");

	AQ_WRITE_REG(sc, AQ_FW_GLB_CTL2_REG, 0x40e1);
	AQ_WRITE_REG(sc, AQ_FW_GLB_CPU_SEM_REG(0), 1);
	AQ_WRITE_REG(sc, AQ_MBOXIF_POWER_GATING_CONTROL_REG, 0);

	/* MAC FW will reload PHY FW if 1E.1000.3 was cleaned - #undone */
	AQ_WRITE_REG(sc, FW_BOOT_EXIT_CODE_REG, RBL_STATUS_DEAD);

	global_software_reset(sc);

	AQ_WRITE_REG(sc, AQ_FW_GLB_CTL2_REG, 0x40e0);

	/* Wait for RBL to finish boot process. */
#define RBL_TIMEOUT_MS	10000
	uint16_t rbl_status;
	for (timo = RBL_TIMEOUT_MS; timo > 0; timo--) {
		rbl_status = AQ_READ_REG(sc, FW_BOOT_EXIT_CODE_REG) & 0xffff;
		if (rbl_status != 0 && rbl_status != RBL_STATUS_DEAD)
			break;
		msec_delay(1);
	}
	if (timo <= 0) {
		aprint_error_dev(sc->sc_dev,
		    "RBL> RBL restart failed: timeout\n");
		return EBUSY;
	}
	switch (rbl_status) {
	case RBL_STATUS_SUCCESS:
		if (mode != NULL)
			*mode = FW_BOOT_MODE_RBL_FLASH;
		aprint_debug_dev(sc->sc_dev, "RBL> reset complete! [Flash]\n");
		break;
	case RBL_STATUS_HOST_BOOT:
		if (mode != NULL)
			*mode = FW_BOOT_MODE_RBL_HOST_BOOTLOAD;
		aprint_debug_dev(sc->sc_dev,
		    "RBL> reset complete! [Host Bootload]\n");
		break;
	case RBL_STATUS_FAILURE:
	default:
		aprint_error_dev(sc->sc_dev,
		    "unknown RBL status 0x%x\n", rbl_status);
		return EBUSY;
	}

	return 0;
}

static int
mac_soft_reset_flb(struct aq_softc *sc)
{
	uint32_t v;
	int timo;

	AQ_WRITE_REG(sc, AQ_FW_GLB_CTL2_REG, 0x40e1);
	/*
	 * Let Felicity hardware to complete SMBUS transaction before
	 * Global software reset.
	 */
	msec_delay(50);

	/*
	 * If SPI burst transaction was interrupted(before running the script),
	 * global software reset may not clear SPI interface.
	 * Clean it up manually before global reset.
	 */
	AQ_WRITE_REG(sc, AQ_GLB_NVR_PROVISIONING2_REG, 0x00a0);
	AQ_WRITE_REG(sc, AQ_GLB_NVR_INTERFACE1_REG, 0x009f);
	AQ_WRITE_REG(sc, AQ_GLB_NVR_INTERFACE1_REG, 0x809f);
	msec_delay(50);

	v = AQ_READ_REG(sc, AQ_FW_SOFTRESET_REG);
	v &= ~AQ_FW_SOFTRESET_DIS;
	v |= AQ_FW_SOFTRESET_RESET;
	AQ_WRITE_REG(sc, AQ_FW_SOFTRESET_REG, v);

	/* Kickstart. */
	AQ_WRITE_REG(sc, AQ_FW_GLB_CTL2_REG, 0x80e0);
	AQ_WRITE_REG(sc, AQ_MBOXIF_POWER_GATING_CONTROL_REG, 0);
	if (!sc->sc_fast_start_enabled)
		AQ_WRITE_REG(sc, AQ_GLB_GENERAL_PROVISIONING9_REG, 1);

	/*
	 * For the case SPI burst transaction was interrupted (by MCP reset
	 * above), wait until it is completed by hardware.
	 */
	msec_delay(50);

	/* MAC Kickstart */
	if (!sc->sc_fast_start_enabled) {
		AQ_WRITE_REG(sc, AQ_FW_GLB_CTL2_REG, 0x180e0);

		uint32_t flb_status;
		for (timo = 0; timo < 1000; timo++) {
			flb_status = AQ_READ_REG(sc,
			    FW_MPI_DAISY_CHAIN_STATUS_REG) & 0x10;
			if (flb_status != 0)
				break;
			msec_delay(1);
		}
		if (flb_status == 0) {
			aprint_error_dev(sc->sc_dev,
			    "FLB> MAC kickstart failed: timed out\n");
			return ETIMEDOUT;
		}
		aprint_debug_dev(sc->sc_dev,
		    "FLB> MAC kickstart done, %d ms\n", timo);
		/* FW reset */
		AQ_WRITE_REG(sc, AQ_FW_GLB_CTL2_REG, 0x80e0);
		/*
		 * Let Felicity hardware complete SMBUS transaction before
		 * Global software reset.
		 */
		msec_delay(50);
		sc->sc_fast_start_enabled = true;
	}
	AQ_WRITE_REG(sc, AQ_FW_GLB_CPU_SEM_REG(0), 1);

	/* PHY Kickstart: #undone */
	global_software_reset(sc);

	for (timo = 0; timo < 1000; timo++) {
		if (AQ_READ_REG(sc, AQ_FW_VERSION_REG) != 0)
			break;
		msec_delay(10);
	}
	if (timo >= 1000) {
		aprint_error_dev(sc->sc_dev, "FLB> Global Soft Reset failed\n");
		return ETIMEDOUT;
	}
	aprint_debug_dev(sc->sc_dev, "FLB> F/W restart: %d ms\n", timo * 10);
	return 0;

}

static int
mac_soft_reset(struct aq_softc *sc, aq_fw_bootloader_mode_t *mode)
{
	if (sc->sc_rbl_enabled)
		return mac_soft_reset_rbl(sc, mode);

	if (mode != NULL)
		*mode = FW_BOOT_MODE_FLB;
	return mac_soft_reset_flb(sc);
}

static int
aq_fw_read_version(struct aq_softc *sc)
{
	int i, error = EBUSY;
#define MAC_FW_START_TIMEOUT_MS	10000
	for (i = 0; i < MAC_FW_START_TIMEOUT_MS; i++) {
		sc->sc_fw_version = AQ_READ_REG(sc, AQ_FW_VERSION_REG);
		if (sc->sc_fw_version != 0) {
			error = 0;
			break;
		}
		delay(1000);
	}
	return error;
}

static int
aq_fw_reset(struct aq_softc *sc)
{
	uint32_t ver, v, bootExitCode;
	int i, error;

	ver = AQ_READ_REG(sc, AQ_FW_VERSION_REG);

	for (i = 1000; i > 0; i--) {
		v = AQ_READ_REG(sc, FW_MPI_DAISY_CHAIN_STATUS_REG);
		bootExitCode = AQ_READ_REG(sc, FW_BOOT_EXIT_CODE_REG);
		if (v != 0x06000000 || bootExitCode != 0)
			break;
	}
	if (i <= 0) {
		aprint_error_dev(sc->sc_dev,
		    "F/W reset failed. Neither RBL nor FLB started\n");
		return ETIMEDOUT;
	}
	sc->sc_rbl_enabled = (bootExitCode != 0);

	/*
	 * Having FW version 0 is an indicator that cold start
	 * is in progress. This means two things:
	 * 1) Driver have to wait for FW/HW to finish boot (500ms giveup)
	 * 2) Driver may skip reset sequence and save time.
	 */
	if (sc->sc_fast_start_enabled && (ver != 0)) {
		error = aq_fw_read_version(sc);
		/* Skip reset as it just completed */
		if (error == 0)
			return 0;
	}

	aq_fw_bootloader_mode_t mode = FW_BOOT_MODE_UNKNOWN;
	error = mac_soft_reset(sc, &mode);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "MAC reset failed: %d\n", error);
		return error;
	}

	switch (mode) {
	case FW_BOOT_MODE_FLB:
		aprint_debug_dev(sc->sc_dev,
		    "FLB> F/W successfully loaded from flash.\n");
		sc->sc_flash_present = true;
		return aq_fw_read_version(sc);
	case FW_BOOT_MODE_RBL_FLASH:
		aprint_debug_dev(sc->sc_dev,
		    "RBL> F/W loaded from flash. Host Bootload disabled.\n");
		sc->sc_flash_present = true;
		return aq_fw_read_version(sc);
	case FW_BOOT_MODE_UNKNOWN:
		aprint_error_dev(sc->sc_dev,
		    "F/W bootload error: unknown bootloader type\n");
		return ENOTSUP;
	case FW_BOOT_MODE_RBL_HOST_BOOTLOAD:
		aprint_debug_dev(sc->sc_dev, "RBL> Host Bootload mode\n");
		break;
	}

	/*
	 * XXX: TODO: add support Host Boot
	 */
	aprint_error_dev(sc->sc_dev,
	    "RBL> F/W Host Bootload not implemented\n");
	return ENOTSUP;
}

static int
aq_hw_reset(struct aq_softc *sc)
{
	int error;

	/* disable irq */
	AQ_WRITE_REG_BIT(sc, AQ_INTR_CTRL_REG, AQ_INTR_CTRL_RESET_DIS, 0);

	/* apply */
	AQ_WRITE_REG_BIT(sc, AQ_INTR_CTRL_REG, AQ_INTR_CTRL_RESET_IRQ, 1);

	/* wait ack 10 times by 1ms */
	WAIT_FOR(
	    (AQ_READ_REG(sc, AQ_INTR_CTRL_REG) & AQ_INTR_CTRL_RESET_IRQ) == 0,
	    1000, 10, &error);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "atlantic: IRQ reset failed: %d\n", error);
		return error;
	}

	return sc->sc_fw_ops->reset(sc);
}

static int
aq_hw_init_ucp(struct aq_softc *sc)
{
	int timo;

	if (FW_VERSION_MAJOR(sc) == 1) {
		if (AQ_READ_REG(sc, FW1X_MPI_INIT2_REG) == 0) {
			uint32_t data;
			cprng_fast(&data, sizeof(data));
			data &= 0xfefefefe;
			data |= 0x02020202;
			AQ_WRITE_REG(sc, FW1X_MPI_INIT2_REG, data);
		}
		AQ_WRITE_REG(sc, FW1X_MPI_INIT1_REG, 0);
	}

	for (timo = 100; timo > 0; timo--) {
		sc->sc_mbox_addr = AQ_READ_REG(sc, FW_MPI_MBOX_ADDR_REG);
		if (sc->sc_mbox_addr != 0)
			break;
		delay(1000);
	}

#define AQ_FW_MIN_VERSION	0x01050006
#define AQ_FW_MIN_VERSION_STR	"1.5.6"
	if (sc->sc_fw_version < AQ_FW_MIN_VERSION) {
		aprint_error_dev(sc->sc_dev,
		    "atlantic: wrong FW version: " AQ_FW_MIN_VERSION_STR
		    " or later required, this is %d.%d.%d\n",
		    FW_VERSION_MAJOR(sc),
		    FW_VERSION_MINOR(sc),
		    FW_VERSION_BUILD(sc));
		return ENOTSUP;
	}

	return 0;
}

static int
aq_fw_version_init(struct aq_softc *sc)
{
	int error = 0;
	char fw_vers[sizeof("F/W version xxxxx.xxxxx.xxxxx")];

	if (FW_VERSION_MAJOR(sc) == 1) {
		sc->sc_fw_ops = &aq_fw1x_ops;
	} else if ((FW_VERSION_MAJOR(sc) == 2) || (FW_VERSION_MAJOR(sc) == 3)) {
		sc->sc_fw_ops = &aq_fw2x_ops;
	} else {
		aprint_error_dev(sc->sc_dev,
		    "Unsupported F/W version %d.%d.%d\n",
		    FW_VERSION_MAJOR(sc), FW_VERSION_MINOR(sc),
		    FW_VERSION_BUILD(sc));
		return ENOTSUP;
	}
	snprintf(fw_vers, sizeof(fw_vers), "F/W version %d.%d.%d",
	    FW_VERSION_MAJOR(sc), FW_VERSION_MINOR(sc), FW_VERSION_BUILD(sc));

	/* detect revision */
	uint32_t hwrev = AQ_READ_REG(sc, AQ_HW_REVISION_REG);
	switch (hwrev & 0x0000000f) {
	case 0x01:
		aprint_normal_dev(sc->sc_dev, "Atlantic revision A0, %s\n",
		    fw_vers);
		sc->sc_features |= FEATURES_REV_A0 |
		    FEATURES_MPI_AQ | FEATURES_MIPS;
		sc->sc_max_mtu = AQ_JUMBO_MTU_REV_A;
		break;
	case 0x02:
		aprint_normal_dev(sc->sc_dev, "Atlantic revision B0, %s\n",
		    fw_vers);
		sc->sc_features |= FEATURES_REV_B0 |
		    FEATURES_MPI_AQ | FEATURES_MIPS |
		    FEATURES_TPO2 | FEATURES_RPF2;
		sc->sc_max_mtu = AQ_JUMBO_MTU_REV_B;
		break;
	case 0x0A:
		aprint_normal_dev(sc->sc_dev, "Atlantic revision B1, %s\n",
		    fw_vers);
		sc->sc_features |= FEATURES_REV_B1 |
		    FEATURES_MPI_AQ | FEATURES_MIPS |
		    FEATURES_TPO2 | FEATURES_RPF2;
		sc->sc_max_mtu = AQ_JUMBO_MTU_REV_B;
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "Unknown revision (0x%08x)\n", hwrev);
		sc->sc_features = 0;
		sc->sc_max_mtu = ETHERMTU;
		error = ENOTSUP;
		break;
	}
	return error;
}

static int
fw1x_reset(struct aq_softc *sc)
{
	struct aq_mailbox_header mbox;
	const int retryCount = 1000;
	uint32_t tid0;
	int i;

	tid0 = ~0;	/*< Initial value of MBOX transactionId. */

	for (i = 0; i < retryCount; ++i) {
		/*
		 * Read the beginning of Statistics structure to capture
		 * the Transaction ID.
		 */
		aq_fw_downld_dwords(sc, sc->sc_mbox_addr,
		    (uint32_t *)&mbox, sizeof(mbox) / sizeof(uint32_t));

		/* Successfully read the stats. */
		if (tid0 == ~0U) {
			/* We have read the initial value. */
			tid0 = mbox.transaction_id;
			continue;
		} else if (mbox.transaction_id != tid0) {
			/*
			 * Compare transaction ID to initial value.
			 * If it's different means f/w is alive.
			 * We're done.
			 */
			return 0;
		}

		/*
		 * Transaction ID value haven't changed since last time.
		 * Try reading the stats again.
		 */
		delay(10);
	}
	aprint_error_dev(sc->sc_dev, "F/W 1.x reset finalize timeout\n");
	return EBUSY;
}

static int
fw1x_set_mode(struct aq_softc *sc, aq_hw_fw_mpi_state_t mode,
    aq_link_speed_t speed, aq_link_fc_t fc, aq_link_eee_t eee)
{
	uint32_t mpictrl = 0;
	uint32_t mpispeed = 0;

	if (speed & AQ_LINK_10G)
		mpispeed |= FW1X_CTRL_10G;
	if (speed & AQ_LINK_5G)
		mpispeed |= (FW1X_CTRL_5G | FW1X_CTRL_5GSR);
	if (speed & AQ_LINK_2G5)
		mpispeed |= FW1X_CTRL_2G5;
	if (speed & AQ_LINK_1G)
		mpispeed |= FW1X_CTRL_1G;
	if (speed & AQ_LINK_100M)
		mpispeed |= FW1X_CTRL_100M;

	mpictrl |= __SHIFTIN(mode, FW1X_MPI_STATE_MODE);
	mpictrl |= __SHIFTIN(mpispeed, FW1X_MPI_STATE_SPEED);
	AQ_WRITE_REG(sc, FW1X_MPI_CONTROL_REG, mpictrl);
	return 0;
}

static int
fw1x_get_mode(struct aq_softc *sc, aq_hw_fw_mpi_state_t *modep,
    aq_link_speed_t *speedp, aq_link_fc_t *fcp, aq_link_eee_t *eeep)
{
	uint32_t mpistate, mpi_speed;
	aq_link_speed_t speed = AQ_LINK_NONE;

	mpistate = AQ_READ_REG(sc, FW1X_MPI_STATE_REG);

	if (modep != NULL)
		*modep = __SHIFTOUT(mpistate, FW1X_MPI_STATE_MODE);

	mpi_speed = __SHIFTOUT(mpistate, FW1X_MPI_STATE_SPEED);
	if (mpi_speed & FW1X_CTRL_10G)
		speed = AQ_LINK_10G;
	else if (mpi_speed & (FW1X_CTRL_5G|FW1X_CTRL_5GSR))
		speed = AQ_LINK_5G;
	else if (mpi_speed & FW1X_CTRL_2G5)
		speed = AQ_LINK_2G5;
	else if (mpi_speed & FW1X_CTRL_1G)
		speed = AQ_LINK_1G;
	else if (mpi_speed & FW1X_CTRL_100M)
		speed = AQ_LINK_100M;

	if (speedp != NULL)
		*speedp = speed;

	if (fcp != NULL)
		*fcp = AQ_FC_NONE;

	if (eeep != NULL)
		*eeep = AQ_EEE_DISABLE;

	return 0;
}

static int
fw1x_get_stats(struct aq_softc *sc, aq_hw_stats_s_t *stats)
{
	int error;

	error = aq_fw_downld_dwords(sc,
	    sc->sc_mbox_addr + offsetof(fw1x_mailbox_t, msm), (uint32_t *)stats,
	    sizeof(aq_hw_stats_s_t) / sizeof(uint32_t));
	if (error < 0) {
		device_printf(sc->sc_dev,
		    "fw1x> download statistics data FAILED, error %d", error);
		return error;
	}

	stats->dpc = AQ_READ_REG(sc, RX_DMA_DROP_PKT_CNT_REG);
	stats->cprc = AQ_READ_REG(sc, RX_DMA_COALESCED_PKT_CNT_REG);
	return 0;
}

static int
fw2x_reset(struct aq_softc *sc)
{
	fw2x_capabilities_t caps = { 0 };
	int error;

	error = aq_fw_downld_dwords(sc,
	    sc->sc_mbox_addr + offsetof(fw2x_mailbox_t, caps),
	    (uint32_t *)&caps, sizeof caps / sizeof(uint32_t));
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "fw2x> can't get F/W capabilities mask, error %d\n",
		    error);
		return error;
	}
	sc->sc_fw_caps = caps.caps_lo | ((uint64_t)caps.caps_hi << 32);

	char buf[256];
	snprintb(buf, sizeof(buf), FW2X_SNPRINTB, sc->sc_fw_caps);
	aprint_verbose_dev(sc->sc_dev, "fw2x> F/W capabilities=%s\n", buf);

	return 0;
}

static int
fw2x_set_mode(struct aq_softc *sc, aq_hw_fw_mpi_state_t mode,
    aq_link_speed_t speed, aq_link_fc_t fc, aq_link_eee_t eee)
{
	uint64_t mpi_ctrl;
	int error = 0;

	AQ_MPI_LOCK(sc);

	mpi_ctrl = AQ_READ64_REG(sc, FW2X_MPI_CONTROL_REG);

	switch (mode) {
	case MPI_INIT:
		mpi_ctrl &= ~FW2X_CTRL_RATE_MASK;
		if (speed & AQ_LINK_10G)
			mpi_ctrl |= FW2X_CTRL_RATE_10G;
		if (speed & AQ_LINK_5G)
			mpi_ctrl |= FW2X_CTRL_RATE_5G;
		if (speed & AQ_LINK_2G5)
			mpi_ctrl |= FW2X_CTRL_RATE_2G5;
		if (speed & AQ_LINK_1G)
			mpi_ctrl |= FW2X_CTRL_RATE_1G;
		if (speed & AQ_LINK_100M)
			mpi_ctrl |= FW2X_CTRL_RATE_100M;

		mpi_ctrl &= ~FW2X_CTRL_LINK_DROP;

		mpi_ctrl &= ~FW2X_CTRL_EEE_MASK;
		if (eee == AQ_EEE_ENABLE)
			mpi_ctrl |= FW2X_CTRL_EEE_MASK;

		mpi_ctrl &= ~(FW2X_CTRL_PAUSE | FW2X_CTRL_ASYMMETRIC_PAUSE);
		if (fc & AQ_FC_RX)
			mpi_ctrl |= FW2X_CTRL_PAUSE;
		if (fc & AQ_FC_TX)
			mpi_ctrl |= FW2X_CTRL_ASYMMETRIC_PAUSE;
		break;
	case MPI_DEINIT:
		mpi_ctrl &= ~(FW2X_CTRL_RATE_MASK | FW2X_CTRL_EEE_MASK);
		mpi_ctrl &= ~(FW2X_CTRL_PAUSE | FW2X_CTRL_ASYMMETRIC_PAUSE);
		break;
	default:
		device_printf(sc->sc_dev, "fw2x> unknown MPI state %d\n", mode);
		error =  EINVAL;
		goto failure;
	}
	AQ_WRITE64_REG(sc, FW2X_MPI_CONTROL_REG, mpi_ctrl);

 failure:
	AQ_MPI_UNLOCK(sc);
	return error;
}

static int
fw2x_get_mode(struct aq_softc *sc, aq_hw_fw_mpi_state_t *modep,
    aq_link_speed_t *speedp, aq_link_fc_t *fcp, aq_link_eee_t *eeep)
{
	uint64_t mpi_state = AQ_READ64_REG(sc, FW2X_MPI_STATE_REG);

	if (modep != NULL) {
		uint64_t mpi_ctrl = AQ_READ64_REG(sc, FW2X_MPI_CONTROL_REG);
		if (mpi_ctrl & FW2X_CTRL_RATE_MASK)
			*modep = MPI_INIT;
		else
			*modep = MPI_DEINIT;
	}

	aq_link_speed_t speed = AQ_LINK_NONE;
	if (mpi_state & FW2X_CTRL_RATE_10G)
		speed = AQ_LINK_10G;
	else if (mpi_state & FW2X_CTRL_RATE_5G)
		speed = AQ_LINK_5G;
	else if (mpi_state & FW2X_CTRL_RATE_2G5)
		speed = AQ_LINK_2G5;
	else if (mpi_state & FW2X_CTRL_RATE_1G)
		speed = AQ_LINK_1G;
	else if (mpi_state & FW2X_CTRL_RATE_100M)
		speed = AQ_LINK_100M;

	if (speedp != NULL)
		*speedp = speed;

	aq_link_fc_t fc = AQ_FC_NONE;
	if (mpi_state & FW2X_CTRL_PAUSE)
		fc |= AQ_FC_RX;
	if (mpi_state & FW2X_CTRL_ASYMMETRIC_PAUSE)
		fc |= AQ_FC_TX;
	if (fcp != NULL)
		*fcp = fc;

	/* XXX: TODO: EEE */
	if (eeep != NULL)
		*eeep = AQ_EEE_DISABLE;

	return 0;
}

static int
toggle_mpi_ctrl_and_wait(struct aq_softc *sc, uint64_t mask,
    uint32_t timeout_ms, uint32_t try_count)
{
	uint64_t mpi_ctrl = AQ_READ64_REG(sc, FW2X_MPI_CONTROL_REG);
	uint64_t mpi_state = AQ_READ64_REG(sc, FW2X_MPI_STATE_REG);
	int error;

	/* First, check that control and state values are consistent */
	if ((mpi_ctrl & mask) != (mpi_state & mask)) {
		device_printf(sc->sc_dev,
		    "fw2x> MPI control (%#llx) and state (%#llx)"
		    " are not consistent for mask %#llx!\n",
		    (unsigned long long)mpi_ctrl, (unsigned long long)mpi_state,
		    (unsigned long long)mask);
		return EINVAL;
	}

	/* Invert bits (toggle) in control register */
	mpi_ctrl ^= mask;
	AQ_WRITE64_REG(sc, FW2X_MPI_CONTROL_REG, mpi_ctrl);

	/* Clear all bits except masked */
	mpi_ctrl &= mask;

	/* Wait for FW reflecting change in state register */
	WAIT_FOR((AQ_READ64_REG(sc, FW2X_MPI_CONTROL_REG) & mask) == mpi_ctrl,
	    1000 * timeout_ms, try_count, &error);
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "f/w2x> timeout while waiting for response"
		    " in state register for bit %#llx!",
		    (unsigned long long)mask);
		return error;
	}
	return 0;
}

static int
fw2x_get_stats(struct aq_softc *sc, aq_hw_stats_s_t *stats)
{
	int error;

	AQ_MPI_LOCK(sc);
	/* Say to F/W to update the statistics */
	error = toggle_mpi_ctrl_and_wait(sc, FW2X_CTRL_STATISTICS, 1, 25);
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "fw2x> statistics update error %d\n", error);
		goto failure;
	}

	CTASSERT(sizeof(fw2x_msm_statistics_t) <= sizeof(struct aq_hw_stats_s));
	error = aq_fw_downld_dwords(sc,
	    sc->sc_mbox_addr + offsetof(fw2x_mailbox_t, msm), (uint32_t *)stats,
	    sizeof(fw2x_msm_statistics_t) / sizeof(uint32_t));
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "fw2x> download statistics data FAILED, error %d", error);
		goto failure;
	}
	stats->dpc = AQ_READ_REG(sc, RX_DMA_DROP_PKT_CNT_REG);
	stats->cprc = AQ_READ_REG(sc, RX_DMA_COALESCED_PKT_CNT_REG);

 failure:
	AQ_MPI_UNLOCK(sc);
	return error;
}

#if NSYSMON_ENVSYS > 0
static int
fw2x_get_temperature(struct aq_softc *sc, uint32_t *temp)
{
	int error;
	uint32_t value, celsius;

	AQ_MPI_LOCK(sc);

	/* Say to F/W to update the temperature */
	error = toggle_mpi_ctrl_and_wait(sc, FW2X_CTRL_TEMPERATURE, 1, 25);
	if (error != 0)
		goto failure;

	error = aq_fw_downld_dwords(sc,
	    sc->sc_mbox_addr + offsetof(fw2x_mailbox_t, phy_info2),
	    &value, sizeof(value) / sizeof(uint32_t));
	if (error != 0)
		goto failure;

	/* 1/256 decrees C to microkelvin */
	celsius = __SHIFTOUT(value, PHYINFO2_TEMPERATURE);
	if (celsius == 0) {
		error = EIO;
		goto failure;
	}
	*temp = celsius * (1000000 / 256) + 273150000;

 failure:
	AQ_MPI_UNLOCK(sc);
	return 0;
}
#endif

static int
aq_fw_downld_dwords(struct aq_softc *sc, uint32_t addr, uint32_t *p,
    uint32_t cnt)
{
	uint32_t v;
	int error = 0;

	WAIT_FOR(AQ_READ_REG(sc, AQ_FW_SEM_RAM_REG) == 1, 1, 10000, &error);
	if (error != 0) {
		AQ_WRITE_REG(sc, AQ_FW_SEM_RAM_REG, 1);
		v = AQ_READ_REG(sc, AQ_FW_SEM_RAM_REG);
		if (v == 0) {
			device_printf(sc->sc_dev,
			    "%s:%d: timeout\n", __func__, __LINE__);
			return ETIMEDOUT;
		}
	}

	AQ_WRITE_REG(sc, AQ_FW_MBOX_ADDR_REG, addr);

	error = 0;
	for (; cnt > 0 && error == 0; cnt--) {
		/* execute mailbox interface */
		AQ_WRITE_REG_BIT(sc, AQ_FW_MBOX_CMD_REG,
		    AQ_FW_MBOX_CMD_EXECUTE, 1);
		if (sc->sc_features & FEATURES_REV_B1) {
			WAIT_FOR(AQ_READ_REG(sc, AQ_FW_MBOX_ADDR_REG) != addr,
			    1, 1000, &error);
		} else {
			WAIT_FOR((AQ_READ_REG(sc, AQ_FW_MBOX_CMD_REG) &
			    AQ_FW_MBOX_CMD_BUSY) == 0,
			    1, 1000, &error);
		}
		*p++ = AQ_READ_REG(sc, AQ_FW_MBOX_VAL_REG);
		addr += sizeof(uint32_t);
	}
	AQ_WRITE_REG(sc, AQ_FW_SEM_RAM_REG, 1);

	if (error != 0)
		device_printf(sc->sc_dev,
		    "%s:%d: timeout\n", __func__, __LINE__);

	return error;
}

/* read my mac address */
static int
aq_get_mac_addr(struct aq_softc *sc)
{
	uint32_t mac_addr[2];
	uint32_t efuse_shadow_addr;
	int err;

	efuse_shadow_addr = 0;
	if (FW_VERSION_MAJOR(sc) >= 2)
		efuse_shadow_addr = AQ_READ_REG(sc, FW2X_MPI_EFUSEADDR_REG);
	else
		efuse_shadow_addr = AQ_READ_REG(sc, FW1X_MPI_EFUSEADDR_REG);

	if (efuse_shadow_addr == 0) {
		aprint_error_dev(sc->sc_dev, "cannot get efuse addr\n");
		return ENXIO;
	}

	memset(mac_addr, 0, sizeof(mac_addr));
	err = aq_fw_downld_dwords(sc, efuse_shadow_addr + (40 * 4),
	    mac_addr, __arraycount(mac_addr));
	if (err < 0)
		return err;

	if (mac_addr[0] == 0 && mac_addr[1] == 0) {
		aprint_error_dev(sc->sc_dev, "mac address not found\n");
		return ENXIO;
	}

	mac_addr[0] = htobe32(mac_addr[0]);
	mac_addr[1] = htobe32(mac_addr[1]);

	memcpy(sc->sc_enaddr.ether_addr_octet,
	    (uint8_t *)mac_addr, ETHER_ADDR_LEN);
	aprint_normal_dev(sc->sc_dev, "Etheraddr: %s\n",
	    ether_sprintf(sc->sc_enaddr.ether_addr_octet));

	return 0;
}

/* set multicast filter. index 0 for own address */
static int
aq_set_mac_addr(struct aq_softc *sc, int index, uint8_t *enaddr)
{
	uint32_t h, l;

	if (index >= AQ_HW_MAC_NUM)
		return EINVAL;

	if (enaddr == NULL) {
		/* disable */
		AQ_WRITE_REG_BIT(sc,
		    RPF_L2UC_MSW_REG(index), RPF_L2UC_MSW_EN, 0);
		return 0;
	}

	h = (enaddr[0] <<  8) | (enaddr[1]);
	l = ((uint32_t)enaddr[2] << 24) | (enaddr[3] << 16) |
	    (enaddr[4] <<  8) | (enaddr[5]);

	/* disable, set, and enable */
	AQ_WRITE_REG_BIT(sc, RPF_L2UC_MSW_REG(index), RPF_L2UC_MSW_EN, 0);
	AQ_WRITE_REG(sc, RPF_L2UC_LSW_REG(index), l);
	AQ_WRITE_REG_BIT(sc, RPF_L2UC_MSW_REG(index),
	    RPF_L2UC_MSW_MACADDR_HI, h);
	AQ_WRITE_REG_BIT(sc, RPF_L2UC_MSW_REG(index), RPF_L2UC_MSW_ACTION, 1);
	AQ_WRITE_REG_BIT(sc, RPF_L2UC_MSW_REG(index), RPF_L2UC_MSW_EN, 1);

	return 0;
}

static int
aq_set_capability(struct aq_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int ip4csum_tx =
	    ((ifp->if_capenable & IFCAP_CSUM_IPv4_Tx) == 0) ? 0 : 1;
	int ip4csum_rx =
	    ((ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) == 0) ? 0 : 1;
	int l4csum_tx = ((ifp->if_capenable &
	   (IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx |
	   IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_UDPv6_Tx)) == 0) ? 0 : 1;
	int l4csum_rx =
	   ((ifp->if_capenable & (IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
	   IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx)) == 0) ? 0 : 1;
	uint32_t lso =
	   ((ifp->if_capenable & (IFCAP_TSOv4 | IFCAP_TSOv6)) == 0) ?
	   0 : 0xffffffff;
	uint32_t lro = ((ifp->if_capenable & IFCAP_LRO) == 0) ?
	    0 : 0xffffffff;
	uint32_t i, v;

	/* TX checksums offloads*/
	AQ_WRITE_REG_BIT(sc, TPO_HWCSUM_REG, TPO_HWCSUM_IP4CSUM_EN, ip4csum_tx);
	AQ_WRITE_REG_BIT(sc, TPO_HWCSUM_REG, TPO_HWCSUM_L4CSUM_EN, l4csum_tx);

	/* RX checksums offloads*/
	AQ_WRITE_REG_BIT(sc, RPO_HWCSUM_REG, RPO_HWCSUM_IP4CSUM_EN, ip4csum_rx);
	AQ_WRITE_REG_BIT(sc, RPO_HWCSUM_REG, RPO_HWCSUM_L4CSUM_EN, l4csum_rx);

	/* LSO offloads*/
	AQ_WRITE_REG(sc, TDM_LSO_EN_REG, lso);

#define AQ_B0_LRO_RXD_MAX	16
	v = (8 < AQ_B0_LRO_RXD_MAX) ? 3 :
	    (4 < AQ_B0_LRO_RXD_MAX) ? 2 :
	    (2 < AQ_B0_LRO_RXD_MAX) ? 1 : 0;
	for (i = 0; i < AQ_RINGS_NUM; i++) {
		AQ_WRITE_REG_BIT(sc, RPO_LRO_LDES_MAX_REG(i),
		    RPO_LRO_LDES_MAX_MASK(i), v);
	}

	AQ_WRITE_REG_BIT(sc, RPO_LRO_TB_DIV_REG, RPO_LRO_TB_DIV, 0x61a);
	AQ_WRITE_REG_BIT(sc, RPO_LRO_INACTIVE_IVAL_REG,
	    RPO_LRO_INACTIVE_IVAL, 0);
	/*
	 * the LRO timebase divider is 5 uS (0x61a),
	 * to get a maximum coalescing interval of 250 uS,
	 * we need to multiply by 50(0x32) to get
	 * the default value 250 uS
	 */
	AQ_WRITE_REG_BIT(sc, RPO_LRO_MAX_COALESCING_IVAL_REG,
	    RPO_LRO_MAX_COALESCING_IVAL, 50);
	AQ_WRITE_REG_BIT(sc, RPO_LRO_CONF_REG,
	    RPO_LRO_CONF_QSESSION_LIMIT, 1);
	AQ_WRITE_REG_BIT(sc, RPO_LRO_CONF_REG,
	    RPO_LRO_CONF_TOTAL_DESC_LIMIT, 2);
	AQ_WRITE_REG_BIT(sc, RPO_LRO_CONF_REG,
	    RPO_LRO_CONF_PATCHOPTIMIZATION_EN, 0);
	AQ_WRITE_REG_BIT(sc, RPO_LRO_CONF_REG,
	    RPO_LRO_CONF_MIN_PAYLOAD_OF_FIRST_PKT, 10);
	AQ_WRITE_REG(sc, RPO_LRO_RSC_MAX_REG, 1);
	AQ_WRITE_REG(sc, RPO_LRO_ENABLE_REG, lro);

	return 0;
}

static int
aq_set_filter(struct aq_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep step;
	int idx, error = 0;

	if (ifp->if_flags & IFF_PROMISC) {
		AQ_WRITE_REG_BIT(sc, RPF_L2BC_REG, RPF_L2BC_PROMISC,
		    (ifp->if_flags & IFF_PROMISC) ? 1 : 0);
		ec->ec_flags |= ETHER_F_ALLMULTI;
		goto done;
	}

	/* clear all table */
	for (idx = 0; idx < AQ_HW_MAC_NUM; idx++) {
		if (idx == AQ_HW_MAC_OWN)	/* already used for own */
			continue;
		aq_set_mac_addr(sc, idx, NULL);
	}

	/* don't accept all multicast */
	AQ_WRITE_REG_BIT(sc, RPF_MCAST_FILTER_MASK_REG,
	    RPF_MCAST_FILTER_MASK_ALLMULTI, 0);
	AQ_WRITE_REG_BIT(sc, RPF_MCAST_FILTER_REG(0),
	    RPF_MCAST_FILTER_EN, 0);

	idx = 0;
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (idx == AQ_HW_MAC_OWN)
			idx++;

		if ((idx >= AQ_HW_MAC_NUM) ||
		    memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * too many filters.
			 * fallback to accept all multicast addresses.
			 */
			AQ_WRITE_REG_BIT(sc, RPF_MCAST_FILTER_MASK_REG,
			    RPF_MCAST_FILTER_MASK_ALLMULTI, 1);
			AQ_WRITE_REG_BIT(sc, RPF_MCAST_FILTER_REG(0),
			    RPF_MCAST_FILTER_EN, 1);
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			goto done;
		}

		/* add a filter */
		aq_set_mac_addr(sc, idx++, enm->enm_addrlo);

		ETHER_NEXT_MULTI(step, enm);
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_UNLOCK(ec);

 done:
	return error;
}

static int
aq_ifmedia_change(struct ifnet * const ifp)
{
	struct aq_softc *sc = ifp->if_softc;
	aq_link_speed_t rate = AQ_LINK_NONE;
	aq_link_fc_t fc = AQ_FC_NONE;
	aq_link_eee_t eee = AQ_EEE_DISABLE;

	if (IFM_TYPE(sc->sc_media.ifm_media) != IFM_ETHER)
		return EINVAL;

	switch (IFM_SUBTYPE(sc->sc_media.ifm_media)) {
	case IFM_AUTO:
		rate = AQ_LINK_AUTO;
		break;
	case IFM_NONE:
		rate = AQ_LINK_NONE;
		break;
	case IFM_100_TX:
		rate = AQ_LINK_100M;
		break;
	case IFM_1000_T:
		rate = AQ_LINK_1G;
		break;
	case IFM_2500_T:
		rate = AQ_LINK_2G5;
		break;
	case IFM_5000_T:
		rate = AQ_LINK_5G;
		break;
	case IFM_10G_T:
		rate = AQ_LINK_10G;
		break;
	default:
		device_printf(sc->sc_dev, "unknown media: 0x%X\n",
		    IFM_SUBTYPE(sc->sc_media.ifm_media));
		return ENODEV;
	}

	if (sc->sc_media.ifm_media & IFM_FLOW)
		fc = AQ_FC_ALL;

	/* XXX: todo EEE */

	/* re-initialize hardware with new parameters */
	aq_set_linkmode(sc, rate, fc, eee);

	return 0;
}

static void
aq_ifmedia_status(struct ifnet * const ifp, struct ifmediareq *ifmr)
{
	struct aq_softc *sc = ifp->if_softc;

	/* update ifm_active */
	ifmr->ifm_active = IFM_ETHER;
	if (sc->sc_link_fc & AQ_FC_RX)
		ifmr->ifm_active |= IFM_ETH_RXPAUSE;
	if (sc->sc_link_fc & AQ_FC_TX)
		ifmr->ifm_active |= IFM_ETH_TXPAUSE;

	switch (sc->sc_link_rate) {
	case AQ_LINK_100M:
		/* XXX: need to detect fulldup or halfdup */
		ifmr->ifm_active |= IFM_100_TX | IFM_FDX;
		break;
	case AQ_LINK_1G:
		ifmr->ifm_active |= IFM_1000_T | IFM_FDX;
		break;
	case AQ_LINK_2G5:
		ifmr->ifm_active |= IFM_2500_T | IFM_FDX;
		break;
	case AQ_LINK_5G:
		ifmr->ifm_active |= IFM_5000_T | IFM_FDX;
		break;
	case AQ_LINK_10G:
		ifmr->ifm_active |= IFM_10G_T | IFM_FDX;
		break;
	default:
		ifmr->ifm_active |= IFM_NONE;
		break;
	}

	/* update ifm_status */
	ifmr->ifm_status = IFM_AVALID;
	if (sc->sc_link_rate != AQ_LINK_NONE)
		ifmr->ifm_status |= IFM_ACTIVE;
}

static void
aq_initmedia(struct aq_softc *sc)
{
#define IFMEDIA_ETHER_ADD(sc, media)	\
	ifmedia_add(&(sc)->sc_media, IFM_ETHER | media, 0, NULL);

	IFMEDIA_ETHER_ADD(sc, IFM_NONE);
	if (sc->sc_available_rates & AQ_LINK_100M) {
		IFMEDIA_ETHER_ADD(sc, IFM_100_TX);
		IFMEDIA_ETHER_ADD(sc, IFM_100_TX | IFM_FLOW);
		IFMEDIA_ETHER_ADD(sc, IFM_100_TX | IFM_FDX | IFM_FLOW);
	}
	if (sc->sc_available_rates & AQ_LINK_1G) {
		IFMEDIA_ETHER_ADD(sc, IFM_1000_T | IFM_FDX);
		IFMEDIA_ETHER_ADD(sc, IFM_1000_T | IFM_FDX | IFM_FLOW);
	}
	if (sc->sc_available_rates & AQ_LINK_2G5) {
		IFMEDIA_ETHER_ADD(sc, IFM_2500_T | IFM_FDX);
		IFMEDIA_ETHER_ADD(sc, IFM_2500_T | IFM_FDX | IFM_FLOW);
	}
	if (sc->sc_available_rates & AQ_LINK_5G) {
		IFMEDIA_ETHER_ADD(sc, IFM_5000_T | IFM_FDX);
		IFMEDIA_ETHER_ADD(sc, IFM_5000_T | IFM_FDX | IFM_FLOW);
	}
	if (sc->sc_available_rates & AQ_LINK_10G) {
		IFMEDIA_ETHER_ADD(sc, IFM_10G_T | IFM_FDX);
		IFMEDIA_ETHER_ADD(sc, IFM_10G_T | IFM_FDX | IFM_FLOW);
	}
	IFMEDIA_ETHER_ADD(sc, IFM_AUTO);
	IFMEDIA_ETHER_ADD(sc, IFM_AUTO | IFM_FLOW);

	/* default: auto without flowcontrol */
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);
	aq_set_linkmode(sc, AQ_LINK_AUTO, AQ_FC_NONE, AQ_EEE_DISABLE);
}

static int
aq_set_linkmode(struct aq_softc *sc, aq_link_speed_t speed, aq_link_fc_t fc,
    aq_link_eee_t eee)
{
	return sc->sc_fw_ops->set_mode(sc, MPI_INIT, speed, fc, eee);
}

static int
aq_get_linkmode(struct aq_softc *sc, aq_link_speed_t *speed, aq_link_fc_t *fc,
   aq_link_eee_t *eee)
{
	aq_hw_fw_mpi_state_t mode;
	int error;

	error = sc->sc_fw_ops->get_mode(sc, &mode, speed, fc, eee);
	if (error != 0)
		return error;
	if (mode != MPI_INIT)
		return ENXIO;

	return 0;
}

static void
aq_hw_init_tx_path(struct aq_softc *sc)
{
	/* Tx TC/RSS number config */
	AQ_WRITE_REG_BIT(sc, TPB_TX_BUF_REG, TPB_TX_BUF_TC_MODE_EN, 1);

	AQ_WRITE_REG_BIT(sc, THM_LSO_TCP_FLAG1_REG,
	    THM_LSO_TCP_FLAG1_FIRST, 0x0ff6);
	AQ_WRITE_REG_BIT(sc, THM_LSO_TCP_FLAG1_REG,
	    THM_LSO_TCP_FLAG1_MID,   0x0ff6);
	AQ_WRITE_REG_BIT(sc, THM_LSO_TCP_FLAG2_REG,
	   THM_LSO_TCP_FLAG2_LAST,  0x0f7f);

	/* misc */
	AQ_WRITE_REG(sc, TX_TPO2_REG,
	   (sc->sc_features & FEATURES_TPO2) ? TX_TPO2_EN : 0);
	AQ_WRITE_REG_BIT(sc, TDM_DCA_REG, TDM_DCA_EN, 0);
	AQ_WRITE_REG_BIT(sc, TDM_DCA_REG, TDM_DCA_MODE, 0);

	AQ_WRITE_REG_BIT(sc, TPB_TX_BUF_REG, TPB_TX_BUF_SCP_INS_EN, 1);
}

static void
aq_hw_init_rx_path(struct aq_softc *sc)
{
	int i;

	/* clear setting */
	AQ_WRITE_REG_BIT(sc, RPB_RPF_RX_REG, RPB_RPF_RX_TC_MODE, 0);
	AQ_WRITE_REG_BIT(sc, RPB_RPF_RX_REG, RPB_RPF_RX_FC_MODE, 0);
	AQ_WRITE_REG(sc, RX_FLR_RSS_CONTROL1_REG, 0);
	for (i = 0; i < 32; i++) {
		AQ_WRITE_REG_BIT(sc, RPF_ETHERTYPE_FILTER_REG(i),
		   RPF_ETHERTYPE_FILTER_EN, 0);
	}

	if (sc->sc_rss_enable) {
		/* Rx TC/RSS number config */
		AQ_WRITE_REG_BIT(sc, RPB_RPF_RX_REG, RPB_RPF_RX_TC_MODE, 1);

		/* Rx flow control */
		AQ_WRITE_REG_BIT(sc, RPB_RPF_RX_REG, RPB_RPF_RX_FC_MODE, 1);

		/* RSS Ring selection */
		switch (sc->sc_nqueues) {
		case 2:
			AQ_WRITE_REG(sc, RX_FLR_RSS_CONTROL1_REG,
			    RX_FLR_RSS_CONTROL1_EN | 0x11111111);
			break;
		case 4:
			AQ_WRITE_REG(sc, RX_FLR_RSS_CONTROL1_REG,
			    RX_FLR_RSS_CONTROL1_EN | 0x22222222);
			break;
		case 8:
			AQ_WRITE_REG(sc, RX_FLR_RSS_CONTROL1_REG,
			    RX_FLR_RSS_CONTROL1_EN | 0x33333333);
			break;
		}
	}

	/* L2 and Multicast filters */
	for (i = 0; i < AQ_HW_MAC_NUM; i++) {
		AQ_WRITE_REG_BIT(sc, RPF_L2UC_MSW_REG(i), RPF_L2UC_MSW_EN, 0);
		AQ_WRITE_REG_BIT(sc, RPF_L2UC_MSW_REG(i), RPF_L2UC_MSW_ACTION,
		    RPF_ACTION_HOST);
	}
	AQ_WRITE_REG(sc, RPF_MCAST_FILTER_MASK_REG, 0);
	AQ_WRITE_REG(sc, RPF_MCAST_FILTER_REG(0), 0x00010fff);

	/* Vlan filters */
	AQ_WRITE_REG_BIT(sc, RPF_VLAN_TPID_REG, RPF_VLAN_TPID_OUTER,
	    ETHERTYPE_QINQ);
	AQ_WRITE_REG_BIT(sc, RPF_VLAN_TPID_REG, RPF_VLAN_TPID_INNER,
	    ETHERTYPE_VLAN);
	AQ_WRITE_REG_BIT(sc, RPF_VLAN_MODE_REG, RPF_VLAN_MODE_PROMISC, 0);

	if (sc->sc_features & FEATURES_REV_B) {
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_MODE_REG,
		    RPF_VLAN_MODE_ACCEPT_UNTAGGED, 1);
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_MODE_REG,
		    RPF_VLAN_MODE_UNTAGGED_ACTION, RPF_ACTION_HOST);
	}

	/* misc */
	if (sc->sc_features & FEATURES_RPF2)
		AQ_WRITE_REG(sc, RX_TCP_RSS_HASH_REG, RX_TCP_RSS_HASH_RPF2);
	else
		AQ_WRITE_REG(sc, RX_TCP_RSS_HASH_REG, 0);

	/*
	 * XXX: RX_TCP_RSS_HASH_REG:
	 *  linux   set 0x000f0000
	 *  freebsd set 0x000f001e
	 */
	/* RSS hash type set for IP/TCP */
	AQ_WRITE_REG_BIT(sc, RX_TCP_RSS_HASH_REG,
	    RX_TCP_RSS_HASH_TYPE, 0x001e);

	AQ_WRITE_REG_BIT(sc, RPF_L2BC_REG, RPF_L2BC_EN, 1);
	AQ_WRITE_REG_BIT(sc, RPF_L2BC_REG, RPF_L2BC_ACTION, RPF_ACTION_HOST);
	AQ_WRITE_REG_BIT(sc, RPF_L2BC_REG, RPF_L2BC_THRESHOLD, 0xffff);

	AQ_WRITE_REG_BIT(sc, RX_DMA_DCA_REG, RX_DMA_DCA_EN, 0);
	AQ_WRITE_REG_BIT(sc, RX_DMA_DCA_REG, RX_DMA_DCA_MODE, 0);
}

static void
aq_hw_interrupt_moderation_set(struct aq_softc *sc)
{
	int i;

	if (sc->sc_intr_moderation_enable) {
		unsigned int tx_min, rx_min;	/* 0-255 */
		unsigned int tx_max, rx_max;	/* 0-511? */

		switch (sc->sc_link_rate) {
		case AQ_LINK_100M:
			tx_min = 0x4f;
			tx_max = 0xff;
			rx_min = 0x04;
			rx_max = 0x50;
			break;
		case AQ_LINK_1G:
		default:
			tx_min = 0x4f;
			tx_max = 0xff;
			rx_min = 0x30;
			rx_max = 0x80;
			break;
		case AQ_LINK_2G5:
			tx_min = 0x4f;
			tx_max = 0xff;
			rx_min = 0x18;
			rx_max = 0xe0;
			break;
		case AQ_LINK_5G:
			tx_min = 0x4f;
			tx_max = 0xff;
			rx_min = 0x0c;
			rx_max = 0x70;
			break;
		case AQ_LINK_10G:
			tx_min = 0x4f;
			tx_max = 0x1ff;
			rx_min = 0x06;	/* freebsd use 80 */
			rx_max = 0x38;	/* freebsd use 120 */
			break;
		}

		AQ_WRITE_REG_BIT(sc, TX_DMA_INT_DESC_WRWB_EN_REG,
		    TX_DMA_INT_DESC_WRWB_EN, 0);
		AQ_WRITE_REG_BIT(sc, TX_DMA_INT_DESC_WRWB_EN_REG,
		    TX_DMA_INT_DESC_MODERATE_EN, 1);
		AQ_WRITE_REG_BIT(sc, RX_DMA_INT_DESC_WRWB_EN_REG,
		    RX_DMA_INT_DESC_WRWB_EN, 0);
		AQ_WRITE_REG_BIT(sc, RX_DMA_INT_DESC_WRWB_EN_REG,
		    RX_DMA_INT_DESC_MODERATE_EN, 1);

		for (i = 0; i < AQ_RINGS_NUM; i++) {
			AQ_WRITE_REG(sc, TX_INTR_MODERATION_CTL_REG(i),
			    __SHIFTIN(tx_min, TX_INTR_MODERATION_CTL_MIN) |
			    __SHIFTIN(tx_max, TX_INTR_MODERATION_CTL_MAX) |
			    TX_INTR_MODERATION_CTL_EN);
		}
		for (i = 0; i < AQ_RINGS_NUM; i++) {
			AQ_WRITE_REG(sc, RX_INTR_MODERATION_CTL_REG(i),
			    __SHIFTIN(rx_min, RX_INTR_MODERATION_CTL_MIN) |
			    __SHIFTIN(rx_max, RX_INTR_MODERATION_CTL_MAX) |
			    RX_INTR_MODERATION_CTL_EN);
		}

	} else {
		AQ_WRITE_REG_BIT(sc, TX_DMA_INT_DESC_WRWB_EN_REG,
		    TX_DMA_INT_DESC_WRWB_EN, 1);
		AQ_WRITE_REG_BIT(sc, TX_DMA_INT_DESC_WRWB_EN_REG,
		    TX_DMA_INT_DESC_MODERATE_EN, 0);
		AQ_WRITE_REG_BIT(sc, RX_DMA_INT_DESC_WRWB_EN_REG,
		    RX_DMA_INT_DESC_WRWB_EN, 1);
		AQ_WRITE_REG_BIT(sc, RX_DMA_INT_DESC_WRWB_EN_REG,
		    RX_DMA_INT_DESC_MODERATE_EN, 0);

		for (i = 0; i < AQ_RINGS_NUM; i++) {
			AQ_WRITE_REG(sc, TX_INTR_MODERATION_CTL_REG(i), 0);
		}
		for (i = 0; i < AQ_RINGS_NUM; i++) {
			AQ_WRITE_REG(sc, RX_INTR_MODERATION_CTL_REG(i), 0);
		}
	}
}

static void
aq_hw_qos_set(struct aq_softc *sc)
{
	uint32_t tc = 0;
	uint32_t buff_size;

	/* TPS Descriptor rate init */
	AQ_WRITE_REG_BIT(sc, TPS_DESC_RATE_REG, TPS_DESC_RATE_TA_RST, 0);
	AQ_WRITE_REG_BIT(sc, TPS_DESC_RATE_REG, TPS_DESC_RATE_LIM, 0xa);

	/* TPS VM init */
	AQ_WRITE_REG_BIT(sc, TPS_DESC_VM_ARB_MODE_REG, TPS_DESC_VM_ARB_MODE, 0);

	/* TPS TC credits init */
	AQ_WRITE_REG_BIT(sc, TPS_DESC_TC_ARB_MODE_REG, TPS_DESC_TC_ARB_MODE, 0);
	AQ_WRITE_REG_BIT(sc, TPS_DATA_TC_ARB_MODE_REG, TPS_DATA_TC_ARB_MODE, 0);

	AQ_WRITE_REG_BIT(sc, TPS_DATA_TCT_REG(tc),
	    TPS_DATA_TCT_CREDIT_MAX, 0xfff);
	AQ_WRITE_REG_BIT(sc, TPS_DATA_TCT_REG(tc),
	    TPS_DATA_TCT_WEIGHT, 0x64);
	AQ_WRITE_REG_BIT(sc, TPS_DESC_TCT_REG(tc),
	    TPS_DESC_TCT_CREDIT_MAX, 0x50);
	AQ_WRITE_REG_BIT(sc, TPS_DESC_TCT_REG(tc),
	    TPS_DESC_TCT_WEIGHT, 0x1e);

	/* Tx buf size */
	tc = 0;
	buff_size = AQ_HW_TXBUF_MAX;
	AQ_WRITE_REG_BIT(sc, TPB_TXB_BUFSIZE_REG(tc), TPB_TXB_BUFSIZE,
	    buff_size);
	AQ_WRITE_REG_BIT(sc, TPB_TXB_THRESH_REG(tc), TPB_TXB_THRESH_HI,
	    (buff_size * (1024 / 32) * 66) / 100);
	AQ_WRITE_REG_BIT(sc, TPB_TXB_THRESH_REG(tc), TPB_TXB_THRESH_LO,
	    (buff_size * (1024 / 32) * 50) / 100);

	/* QoS Rx buf size per TC */
	tc = 0;
	buff_size = AQ_HW_RXBUF_MAX;
	AQ_WRITE_REG_BIT(sc, RPB_RXB_BUFSIZE_REG(tc), RPB_RXB_BUFSIZE,
	    buff_size);
	AQ_WRITE_REG_BIT(sc, RPB_RXB_XOFF_REG(tc), RPB_RXB_XOFF_EN, 0);
	AQ_WRITE_REG_BIT(sc, RPB_RXB_XOFF_REG(tc), RPB_RXB_XOFF_THRESH_HI,
	    (buff_size * (1024 / 32) * 66) / 100);
	AQ_WRITE_REG_BIT(sc, RPB_RXB_XOFF_REG(tc), RPB_RXB_XOFF_THRESH_LO,
	    (buff_size * (1024 / 32) * 50) / 100);

	/* QoS 802.1p priority -> TC mapping */
	int i_priority;
	for (i_priority = 0; i_priority < 8; i_priority++) {
		AQ_WRITE_REG_BIT(sc, RPF_RPB_RX_TC_UPT_REG,
		    RPF_RPB_RX_TC_UPT_MASK(i_priority), 0);
	}
}

/* called once from aq_attach */
static int
aq_init_rss(struct aq_softc *sc)
{
	CTASSERT(AQ_RSS_HASHKEY_SIZE == RSS_KEYSIZE);
	uint32_t rss_key[RSS_KEYSIZE / sizeof(uint32_t)];
	uint8_t rss_table[AQ_RSS_INDIRECTION_TABLE_MAX];
	unsigned int i;
	int error;

	/* initialize rss key */
	rss_getkey((uint8_t *)rss_key);

	/* hash to ring table */
	for (i = 0; i < AQ_RSS_INDIRECTION_TABLE_MAX; i++) {
		rss_table[i] = i % sc->sc_nqueues;
	}

	/*
	 * set rss key
	 */
	for (i = 0; i < __arraycount(rss_key); i++) {
		uint32_t key_data = sc->sc_rss_enable ? ntohl(rss_key[i]) : 0;
		AQ_WRITE_REG(sc, RPF_RSS_KEY_WR_DATA_REG, key_data);
		AQ_WRITE_REG_BIT(sc, RPF_RSS_KEY_ADDR_REG,
		    RPF_RSS_KEY_ADDR, __arraycount(rss_key) - 1 - i);
		AQ_WRITE_REG_BIT(sc, RPF_RSS_KEY_ADDR_REG,
		    RPF_RSS_KEY_WR_EN, 1);
		WAIT_FOR(AQ_READ_REG_BIT(sc, RPF_RSS_KEY_ADDR_REG,
		    RPF_RSS_KEY_WR_EN) == 0, 1000, 10, &error);
		if (error != 0) {
			device_printf(sc->sc_dev, "%s: rss key write timeout\n",
			    __func__);
			goto rss_set_timeout;
		}
	}

	/*
	 * set rss indirection table
	 *
	 * AQ's rss redirect table is consist of 3bit*64 (192bit) packed array.
	 * we'll make it by __BITMAP(3) macros.
	 */
	__BITMAP_TYPE(, uint16_t, 3 * AQ_RSS_INDIRECTION_TABLE_MAX) bit3x64;
	__BITMAP_ZERO(&bit3x64);

#define AQ_3BIT_PACKED_ARRAY_SET(bitmap, idx, val)		\
	do {							\
		if (val & 1) {					\
			__BITMAP_SET((idx) * 3, (bitmap));	\
		} else {					\
			__BITMAP_CLR((idx) * 3, (bitmap));	\
		}						\
		if (val & 2) {					\
			__BITMAP_SET((idx) * 3 + 1, (bitmap));	\
		} else {					\
			__BITMAP_CLR((idx) * 3 + 1, (bitmap));	\
		}						\
		if (val & 4) {					\
			__BITMAP_SET((idx) * 3 + 2, (bitmap));	\
		} else {					\
			__BITMAP_CLR((idx) * 3 + 2, (bitmap));	\
		}						\
	} while (0 /* CONSTCOND */)

	for (i = 0; i < AQ_RSS_INDIRECTION_TABLE_MAX; i++) {
		AQ_3BIT_PACKED_ARRAY_SET(&bit3x64, i, rss_table[i]);
	}

	/* write 192bit data in steps of 16bit */
	for (i = 0; i < (int)__arraycount(bit3x64._b); i++) {
		AQ_WRITE_REG_BIT(sc, RPF_RSS_REDIR_WR_DATA_REG,
		    RPF_RSS_REDIR_WR_DATA, bit3x64._b[i]);
		AQ_WRITE_REG_BIT(sc, RPF_RSS_REDIR_ADDR_REG,
		    RPF_RSS_REDIR_ADDR, i);
		AQ_WRITE_REG_BIT(sc, RPF_RSS_REDIR_ADDR_REG,
		    RPF_RSS_REDIR_WR_EN, 1);

		WAIT_FOR(AQ_READ_REG_BIT(sc, RPF_RSS_REDIR_ADDR_REG,
		    RPF_RSS_REDIR_WR_EN) == 0, 1000, 10, &error);
		if (error != 0)
			break;
	}

 rss_set_timeout:
	return error;
}

static void
aq_hw_l3_filter_set(struct aq_softc *sc)
{
	int i;

	/* clear all filter */
	for (i = 0; i < 8; i++) {
		AQ_WRITE_REG_BIT(sc, RPF_L3_FILTER_REG(i),
		    RPF_L3_FILTER_L4_EN, 0);
	}
}

static void
aq_set_vlan_filters(struct aq_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct vlanid_list *vlanidp;
	int i;

	ETHER_LOCK(ec);

	/* disable all vlan filters */
	for (i = 0; i < RPF_VLAN_MAX_FILTERS; i++)
		AQ_WRITE_REG(sc, RPF_VLAN_FILTER_REG(i), 0);

	/* count VID */
	i = 0;
	SIMPLEQ_FOREACH(vlanidp, &ec->ec_vids, vid_list)
		i++;

	if (((sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_HWFILTER) == 0) ||
	    (ifp->if_flags & IFF_PROMISC) ||
	    (i > RPF_VLAN_MAX_FILTERS)) {
		/*
		 * no vlan hwfilter, in promiscuous mode, or too many VID?
		 * must receive all VID
		 */
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_MODE_REG,
		    RPF_VLAN_MODE_PROMISC, 1);
		goto done;
	}

	/* receive only selected VID */
	AQ_WRITE_REG_BIT(sc, RPF_VLAN_MODE_REG, RPF_VLAN_MODE_PROMISC, 0);
	i = 0;
	SIMPLEQ_FOREACH(vlanidp, &ec->ec_vids, vid_list) {
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_FILTER_REG(i),
		    RPF_VLAN_FILTER_EN, 1);
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_FILTER_REG(i),
		    RPF_VLAN_FILTER_RXQ_EN, 0);
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_FILTER_REG(i),
		    RPF_VLAN_FILTER_RXQ, 0);
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_FILTER_REG(i),
		    RPF_VLAN_FILTER_ACTION, RPF_ACTION_HOST);
		AQ_WRITE_REG_BIT(sc, RPF_VLAN_FILTER_REG(i),
		    RPF_VLAN_FILTER_ID, vlanidp->vid);
		i++;
	}

 done:
	ETHER_UNLOCK(ec);
}

static int
aq_hw_init(struct aq_softc *sc)
{
	uint32_t v;

	/* Force limit MRRS on RDM/TDM to 2K */
	v = AQ_READ_REG(sc, AQ_PCI_REG_CONTROL_6_REG);
	AQ_WRITE_REG(sc, AQ_PCI_REG_CONTROL_6_REG, (v & ~0x0707) | 0x0404);

	/*
	 * TX DMA total request limit. B0 hardware is not capable to
	 * handle more than (8K-MRRS) incoming DMA data.
	 * Value 24 in 256byte units
	 */
	AQ_WRITE_REG(sc, AQ_HW_TX_DMA_TOTAL_REQ_LIMIT_REG, 24);

	aq_hw_init_tx_path(sc);
	aq_hw_init_rx_path(sc);

	aq_hw_interrupt_moderation_set(sc);

	aq_set_mac_addr(sc, AQ_HW_MAC_OWN, sc->sc_enaddr.ether_addr_octet);
	aq_set_linkmode(sc, AQ_LINK_NONE, AQ_FC_NONE, AQ_EEE_DISABLE);

	aq_hw_qos_set(sc);

	/* Enable interrupt */
	int irqmode;
	if (sc->sc_msix)
		irqmode =  AQ_INTR_CTRL_IRQMODE_MSIX;
	else
		irqmode =  AQ_INTR_CTRL_IRQMODE_MSI;

	AQ_WRITE_REG(sc, AQ_INTR_CTRL_REG, AQ_INTR_CTRL_RESET_DIS);
	AQ_WRITE_REG_BIT(sc, AQ_INTR_CTRL_REG, AQ_INTR_CTRL_MULTIVEC,
	    sc->sc_msix ? 1 : 0);
	AQ_WRITE_REG_BIT(sc, AQ_INTR_CTRL_REG, AQ_INTR_CTRL_IRQMODE, irqmode);

	AQ_WRITE_REG(sc, AQ_INTR_AUTOMASK_REG, 0xffffffff);

	AQ_WRITE_REG(sc, AQ_GEN_INTR_MAP_REG(0),
	    ((AQ_B0_ERR_INT << 24) | (1U << 31)) |
	    ((AQ_B0_ERR_INT << 16) | (1 << 23))
	);

	/* link interrupt */
	if (!sc->sc_msix)
		sc->sc_linkstat_irq = AQ_LINKSTAT_IRQ;
	AQ_WRITE_REG(sc, AQ_GEN_INTR_MAP_REG(3),
	    __BIT(7) | sc->sc_linkstat_irq);

	return 0;
}

static int
aq_update_link_status(struct aq_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	aq_link_speed_t rate = AQ_LINK_NONE;
	aq_link_fc_t fc = AQ_FC_NONE;
	aq_link_eee_t eee = AQ_EEE_DISABLE;
	unsigned int speed;
	int changed = 0;

	aq_get_linkmode(sc, &rate, &fc, &eee);

	if (sc->sc_link_rate != rate)
		changed = 1;
	if (sc->sc_link_fc != fc)
		changed = 1;
	if (sc->sc_link_eee != eee)
		changed = 1;

	if (changed) {
		switch (rate) {
		case AQ_LINK_100M:
			speed = 100;
			break;
		case AQ_LINK_1G:
			speed = 1000;
			break;
		case AQ_LINK_2G5:
			speed = 2500;
			break;
		case AQ_LINK_5G:
			speed = 5000;
			break;
		case AQ_LINK_10G:
			speed = 10000;
			break;
		case AQ_LINK_NONE:
		default:
			speed = 0;
			break;
		}

		if (sc->sc_link_rate == AQ_LINK_NONE) {
			/* link DOWN -> UP */
			device_printf(sc->sc_dev, "link is UP: speed=%u\n",
			    speed);
			if_link_state_change(ifp, LINK_STATE_UP);
		} else if (rate == AQ_LINK_NONE) {
			/* link UP -> DOWN */
			device_printf(sc->sc_dev, "link is DOWN\n");
			if_link_state_change(ifp, LINK_STATE_DOWN);
		} else {
			device_printf(sc->sc_dev,
			    "link mode changed: speed=%u, fc=0x%x, eee=%x\n",
			    speed, fc, eee);
		}

		sc->sc_link_rate = rate;
		sc->sc_link_fc = fc;
		sc->sc_link_eee = eee;

		/* update interrupt timing according to new link speed */
		aq_hw_interrupt_moderation_set(sc);
	}

	return changed;
}

#ifdef AQ_EVENT_COUNTERS
static void
aq_update_statistics(struct aq_softc *sc)
{
	int prev = sc->sc_statistics_idx;
	int cur = prev ^ 1;

	sc->sc_fw_ops->get_stats(sc, &sc->sc_statistics[cur]);

	/*
	 * aq's internal statistics counter is 32bit.
	 * cauculate delta, and add to evcount
	 */
#define ADD_DELTA(cur, prev, name)				\
	do {							\
		uint32_t n;					\
		n = (uint32_t)(sc->sc_statistics[cur].name -	\
		    sc->sc_statistics[prev].name);		\
		if (n != 0) {					\
			AQ_EVCNT_ADD(sc, name, n);		\
		}						\
	} while (/*CONSTCOND*/0);

	ADD_DELTA(cur, prev, uprc);
	ADD_DELTA(cur, prev, mprc);
	ADD_DELTA(cur, prev, bprc);
	ADD_DELTA(cur, prev, prc);
	ADD_DELTA(cur, prev, erpr);
	ADD_DELTA(cur, prev, uptc);
	ADD_DELTA(cur, prev, mptc);
	ADD_DELTA(cur, prev, bptc);
	ADD_DELTA(cur, prev, ptc);
	ADD_DELTA(cur, prev, erpt);
	ADD_DELTA(cur, prev, mbtc);
	ADD_DELTA(cur, prev, bbtc);
	ADD_DELTA(cur, prev, mbrc);
	ADD_DELTA(cur, prev, bbrc);
	ADD_DELTA(cur, prev, ubrc);
	ADD_DELTA(cur, prev, ubtc);
	ADD_DELTA(cur, prev, dpc);
	ADD_DELTA(cur, prev, cprc);

	sc->sc_statistics_idx = cur;
}
#endif /* AQ_EVENT_COUNTERS */

/* allocate and map one DMA block */
static int
_alloc_dma(struct aq_softc *sc, bus_size_t size, bus_size_t *sizep,
    void **addrp, bus_dmamap_t *mapp, bus_dma_segment_t *seg)
{
	int nsegs, error;

	if ((error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, seg,
	    1, &nsegs, 0)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate DMA buffer, error=%d\n", error);
		goto fail_alloc;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, seg, 1, size, addrp,
	    BUS_DMA_COHERENT)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to map DMA buffer, error=%d\n", error);
		goto fail_map;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    0, mapp)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create DMA map, error=%d\n", error);
		goto fail_create;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, *mapp, *addrp, size, NULL,
	    0)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to load DMA map, error=%d\n", error);
		goto fail_load;
	}

	*sizep = size;
	return 0;

 fail_load:
	bus_dmamap_destroy(sc->sc_dmat, *mapp);
	*mapp = NULL;
 fail_create:
	bus_dmamem_unmap(sc->sc_dmat, *addrp, size);
	*addrp = NULL;
 fail_map:
	bus_dmamem_free(sc->sc_dmat, seg, 1);
	memset(seg, 0, sizeof(*seg));
 fail_alloc:
	*sizep = 0;
	return error;
}

static void
_free_dma(struct aq_softc *sc, bus_size_t *sizep, void **addrp,
    bus_dmamap_t *mapp, bus_dma_segment_t *seg)
{
	if (*mapp != NULL) {
		bus_dmamap_destroy(sc->sc_dmat, *mapp);
		*mapp = NULL;
	}
	if (*addrp != NULL) {
		bus_dmamem_unmap(sc->sc_dmat, *addrp, *sizep);
		*addrp = NULL;
	}
	if (*sizep != 0) {
		bus_dmamem_free(sc->sc_dmat, seg, 1);
		memset(seg, 0, sizeof(*seg));
		*sizep = 0;
	}
}

static int
aq_txring_alloc(struct aq_softc *sc, struct aq_txring *txring)
{
	int i, error;

	/* allocate tx descriptors */
	error = _alloc_dma(sc, sizeof(aq_tx_desc_t) * AQ_TXD_NUM,
	    &txring->txr_txdesc_size, (void **)&txring->txr_txdesc,
	    &txring->txr_txdesc_dmamap, txring->txr_txdesc_seg);
	if (error != 0)
		return error;

	memset(txring->txr_txdesc, 0, sizeof(aq_tx_desc_t) * AQ_TXD_NUM);

	/* fill tx ring with dmamap */
	for (i = 0; i < AQ_TXD_NUM; i++) {
#define AQ_MAXDMASIZE	(16 * 1024)
#define AQ_NTXSEGS	32
		/* XXX: TODO: error check */
		bus_dmamap_create(sc->sc_dmat, AQ_MAXDMASIZE, AQ_NTXSEGS,
		    AQ_MAXDMASIZE, 0, 0, &txring->txr_mbufs[i].dmamap);
	}
	return 0;
}

static void
aq_txring_free(struct aq_softc *sc, struct aq_txring *txring)
{
	int i;

	_free_dma(sc, &txring->txr_txdesc_size, (void **)&txring->txr_txdesc,
	    &txring->txr_txdesc_dmamap, txring->txr_txdesc_seg);

	for (i = 0; i < AQ_TXD_NUM; i++) {
		if (txring->txr_mbufs[i].dmamap != NULL) {
			if (txring->txr_mbufs[i].m != NULL) {
				bus_dmamap_unload(sc->sc_dmat,
				    txring->txr_mbufs[i].dmamap);
				m_freem(txring->txr_mbufs[i].m);
				txring->txr_mbufs[i].m = NULL;
			}
			bus_dmamap_destroy(sc->sc_dmat,
			    txring->txr_mbufs[i].dmamap);
			txring->txr_mbufs[i].dmamap = NULL;
		}
	}
}

static int
aq_rxring_alloc(struct aq_softc *sc, struct aq_rxring *rxring)
{
	int i, error;

	/* allocate rx descriptors */
	error = _alloc_dma(sc, sizeof(aq_rx_desc_t) * AQ_RXD_NUM,
	    &rxring->rxr_rxdesc_size, (void **)&rxring->rxr_rxdesc,
	    &rxring->rxr_rxdesc_dmamap, rxring->rxr_rxdesc_seg);
	if (error != 0)
		return error;

	memset(rxring->rxr_rxdesc, 0, sizeof(aq_rx_desc_t) * AQ_RXD_NUM);

	/* fill rxring with dmamaps */
	for (i = 0; i < AQ_RXD_NUM; i++) {
		rxring->rxr_mbufs[i].m = NULL;
		/* XXX: TODO: error check */
		bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES, 0, 0,
		    &rxring->rxr_mbufs[i].dmamap);
	}
	return 0;
}

static void
aq_rxdrain(struct aq_softc *sc, struct aq_rxring *rxring)
{
	int i;

	/* free all mbufs allocated for RX */
	for (i = 0; i < AQ_RXD_NUM; i++) {
		if (rxring->rxr_mbufs[i].m != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    rxring->rxr_mbufs[i].dmamap);
			m_freem(rxring->rxr_mbufs[i].m);
			rxring->rxr_mbufs[i].m = NULL;
		}
	}
}

static void
aq_rxring_free(struct aq_softc *sc, struct aq_rxring *rxring)
{
	int i;

	/* free all mbufs and dmamaps */
	aq_rxdrain(sc, rxring);
	for (i = 0; i < AQ_RXD_NUM; i++) {
		if (rxring->rxr_mbufs[i].dmamap != NULL) {
			bus_dmamap_destroy(sc->sc_dmat,
			    rxring->rxr_mbufs[i].dmamap);
			rxring->rxr_mbufs[i].dmamap = NULL;
		}
	}

	/* free RX descriptor */
	_free_dma(sc, &rxring->rxr_rxdesc_size, (void **)&rxring->rxr_rxdesc,
	    &rxring->rxr_rxdesc_dmamap, rxring->rxr_rxdesc_seg);
}

static void
aq_rxring_setmbuf(struct aq_softc *sc, struct aq_rxring *rxring, int idx,
    struct mbuf *m)
{
	int error;

	/* if mbuf already exists, unload and free */
	if (rxring->rxr_mbufs[idx].m != NULL) {
		bus_dmamap_unload(sc->sc_dmat, rxring->rxr_mbufs[idx].dmamap);
		m_freem(rxring->rxr_mbufs[idx].m);
		rxring->rxr_mbufs[idx].m = NULL;
	}

	rxring->rxr_mbufs[idx].m = m;

	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	error = bus_dmamap_load_mbuf(sc->sc_dmat, rxring->rxr_mbufs[idx].dmamap,
	    m, BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error) {
		device_printf(sc->sc_dev,
		    "unable to load rx DMA map %d, error = %d\n", idx, error);
		panic("%s: unable to load rx DMA map. error=%d",
		    __func__, error);
	}
	bus_dmamap_sync(sc->sc_dmat, rxring->rxr_mbufs[idx].dmamap, 0,
	    rxring->rxr_mbufs[idx].dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
}

static inline void
aq_rxring_reset_desc(struct aq_softc *sc, struct aq_rxring *rxring, int idx)
{
	/* refill rxdesc, and sync */
	rxring->rxr_rxdesc[idx].read.buf_addr =
	   htole64(rxring->rxr_mbufs[idx].dmamap->dm_segs[0].ds_addr);
	rxring->rxr_rxdesc[idx].read.hdr_addr = 0;
	bus_dmamap_sync(sc->sc_dmat, rxring->rxr_rxdesc_dmamap,
	    sizeof(aq_rx_desc_t) * idx, sizeof(aq_rx_desc_t),
	    BUS_DMASYNC_PREWRITE);
}

static struct mbuf *
aq_alloc_mbuf(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return NULL;
	}

	return m;
}

/* allocate mbuf and unload dmamap */
static int
aq_rxring_add(struct aq_softc *sc, struct aq_rxring *rxring, int idx)
{
	struct mbuf *m;

	m = aq_alloc_mbuf();
	if (m == NULL)
		return ENOBUFS;

	aq_rxring_setmbuf(sc, rxring, idx, m);
	return 0;
}

static int
aq_txrx_rings_alloc(struct aq_softc *sc)
{
	int n, error;

	for (n = 0; n < sc->sc_nqueues; n++) {
		sc->sc_queue[n].sc = sc;
		sc->sc_queue[n].txring.txr_sc = sc;
		sc->sc_queue[n].txring.txr_index = n;
		mutex_init(&sc->sc_queue[n].txring.txr_mutex, MUTEX_DEFAULT,
		    IPL_NET);
		error = aq_txring_alloc(sc, &sc->sc_queue[n].txring);
		if (error != 0)
			goto failure;

		error = aq_tx_pcq_alloc(sc, &sc->sc_queue[n].txring);
		if (error != 0)
			goto failure;

		sc->sc_queue[n].rxring.rxr_sc = sc;
		sc->sc_queue[n].rxring.rxr_index = n;
		mutex_init(&sc->sc_queue[n].rxring.rxr_mutex, MUTEX_DEFAULT,
		   IPL_NET);
		error = aq_rxring_alloc(sc, &sc->sc_queue[n].rxring);
		if (error != 0)
			break;
	}

 failure:
	return error;
}

static void
aq_txrx_rings_free(struct aq_softc *sc)
{
	int n;

	for (n = 0; n < sc->sc_nqueues; n++) {
		aq_txring_free(sc, &sc->sc_queue[n].txring);
		mutex_destroy(&sc->sc_queue[n].txring.txr_mutex);

		aq_tx_pcq_free(sc, &sc->sc_queue[n].txring);

		aq_rxring_free(sc, &sc->sc_queue[n].rxring);
		mutex_destroy(&sc->sc_queue[n].rxring.rxr_mutex);
	}
}

static int
aq_tx_pcq_alloc(struct aq_softc *sc, struct aq_txring *txring)
{
	int error = 0;
	txring->txr_softint = NULL;

	txring->txr_pcq = pcq_create(AQ_TXD_NUM, KM_NOSLEEP);
	if (txring->txr_pcq == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate pcq for TXring[%d]\n",
		    txring->txr_index);
		error = ENOMEM;
		goto done;
	}

	txring->txr_softint = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    aq_deferred_transmit, txring);
	if (txring->txr_softint == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish softint for TXring[%d]\n",
		    txring->txr_index);
		error = ENOENT;
	}

 done:
	return error;
}

static void
aq_tx_pcq_free(struct aq_softc *sc, struct aq_txring *txring)
{
	struct mbuf *m;

	if (txring->txr_softint != NULL) {
		softint_disestablish(txring->txr_softint);
		txring->txr_softint = NULL;
	}

	if (txring->txr_pcq != NULL) {
		while ((m = pcq_get(txring->txr_pcq)) != NULL)
			m_freem(m);
		pcq_destroy(txring->txr_pcq);
		txring->txr_pcq = NULL;
	}
}

#if NSYSMON_ENVSYS > 0
static void
aq_temp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct aq_softc *sc;
	uint32_t temp;
	int error;

	sc = sme->sme_cookie;

	error = sc->sc_fw_ops->get_temperature(sc, &temp);
	if (error == 0) {
		edata->value_cur = temp;
		edata->state = ENVSYS_SVALID;
	} else {
		edata->state = ENVSYS_SINVALID;
	}
}
#endif

static void
aq_tick(void *arg)
{
	struct aq_softc *sc = arg;

	if (sc->sc_poll_linkstat || sc->sc_detect_linkstat) {
		sc->sc_detect_linkstat = false;
		aq_update_link_status(sc);
	}

#ifdef AQ_EVENT_COUNTERS
	if (sc->sc_poll_statistics)
		aq_update_statistics(sc);
#endif

	if (sc->sc_poll_linkstat
#ifdef AQ_EVENT_COUNTERS
	    || sc->sc_poll_statistics
#endif
	    ) {
		callout_schedule(&sc->sc_tick_ch, hz);
	}
}

/* interrupt enable/disable */
static void
aq_enable_intr(struct aq_softc *sc, bool link, bool txrx)
{
	uint32_t imask = 0;
	int i;

	if (txrx) {
		for (i = 0; i < sc->sc_nqueues; i++) {
			imask |= __BIT(sc->sc_tx_irq[i]);
			imask |= __BIT(sc->sc_rx_irq[i]);
		}
	}

	if (link)
		imask |= __BIT(sc->sc_linkstat_irq);

	AQ_WRITE_REG(sc, AQ_INTR_MASK_REG, imask);
	AQ_WRITE_REG(sc, AQ_INTR_STATUS_CLR_REG, 0xffffffff);
}

static int
aq_legacy_intr(void *arg)
{
	struct aq_softc *sc = arg;
	uint32_t status;
	int nintr = 0;

	status = AQ_READ_REG(sc, AQ_INTR_STATUS_REG);
	AQ_WRITE_REG(sc, AQ_INTR_STATUS_CLR_REG, 0xffffffff);

	if (status & __BIT(sc->sc_linkstat_irq)) {
		sc->sc_detect_linkstat = true;
		callout_schedule(&sc->sc_tick_ch, 0);
		nintr++;
	}

	if (status & __BIT(sc->sc_rx_irq[0])) {
		nintr += aq_rx_intr(&sc->sc_queue[0].rxring);
	}

	if (status & __BIT(sc->sc_tx_irq[0])) {
		nintr += aq_tx_intr(&sc->sc_queue[0].txring);
	}

	return nintr;
}

static int
aq_txrx_intr(void *arg)
{
	struct aq_queue *queue = arg;
	struct aq_softc *sc = queue->sc;
	struct aq_txring *txring = &queue->txring;
	struct aq_rxring *rxring = &queue->rxring;
	uint32_t status;
	int nintr = 0;
	int txringidx, rxringidx, txirq, rxirq;

	txringidx = txring->txr_index;
	rxringidx = rxring->rxr_index;
	txirq = sc->sc_tx_irq[txringidx];
	rxirq = sc->sc_rx_irq[rxringidx];

	status = AQ_READ_REG(sc, AQ_INTR_STATUS_REG);
	if ((status & (__BIT(txirq) | __BIT(rxirq))) == 0) {
		/* stray interrupt? */
		return 0;
	}

	nintr += aq_rx_intr(rxring);
	nintr += aq_tx_intr(txring);

	return nintr;
}

static int
aq_link_intr(void *arg)
{
	struct aq_softc *sc = arg;
	uint32_t status;
	int nintr = 0;

	status = AQ_READ_REG(sc, AQ_INTR_STATUS_REG);
	if (status & __BIT(sc->sc_linkstat_irq)) {
		sc->sc_detect_linkstat = true;
		callout_schedule(&sc->sc_tick_ch, 0);
		AQ_WRITE_REG(sc, AQ_INTR_STATUS_CLR_REG,
		    __BIT(sc->sc_linkstat_irq));
		nintr++;
	}

	return nintr;
}

static void
aq_txring_reset(struct aq_softc *sc, struct aq_txring *txring, bool start)
{
	const int ringidx = txring->txr_index;
	int i;

	mutex_enter(&txring->txr_mutex);

	txring->txr_prodidx = 0;
	txring->txr_considx = 0;
	txring->txr_nfree = AQ_TXD_NUM;
	txring->txr_active = false;

	/* free mbufs untransmitted */
	for (i = 0; i < AQ_TXD_NUM; i++) {
		if (txring->txr_mbufs[i].m != NULL) {
			m_freem(txring->txr_mbufs[i].m);
			txring->txr_mbufs[i].m = NULL;
		}
	}

	/* disable DMA */
	AQ_WRITE_REG_BIT(sc, TX_DMA_DESC_REG(ringidx), TX_DMA_DESC_EN, 0);

	if (start) {
		/* TX descriptor physical address */
		paddr_t paddr = txring->txr_txdesc_dmamap->dm_segs[0].ds_addr;
		AQ_WRITE_REG(sc, TX_DMA_DESC_BASE_ADDRLSW_REG(ringidx), paddr);
		AQ_WRITE_REG(sc, TX_DMA_DESC_BASE_ADDRMSW_REG(ringidx),
		    (uint32_t)((uint64_t)paddr >> 32));

		/* TX descriptor size */
		AQ_WRITE_REG_BIT(sc, TX_DMA_DESC_REG(ringidx), TX_DMA_DESC_LEN,
		    AQ_TXD_NUM / 8);

		/* reload TAIL pointer */
		txring->txr_prodidx = txring->txr_considx =
		    AQ_READ_REG(sc, TX_DMA_DESC_TAIL_PTR_REG(ringidx));
		AQ_WRITE_REG(sc, TX_DMA_DESC_WRWB_THRESH_REG(ringidx), 0);

		/* Mapping interrupt vector */
		AQ_WRITE_REG_BIT(sc, AQ_INTR_IRQ_MAP_TX_REG(ringidx),
		    AQ_INTR_IRQ_MAP_TX_IRQMAP(ringidx), sc->sc_tx_irq[ringidx]);
		AQ_WRITE_REG_BIT(sc, AQ_INTR_IRQ_MAP_TX_REG(ringidx),
		    AQ_INTR_IRQ_MAP_TX_EN(ringidx), true);

		/* enable DMA */
		AQ_WRITE_REG_BIT(sc, TX_DMA_DESC_REG(ringidx),
		    TX_DMA_DESC_EN, 1);

		const int cpuid = 0;	/* XXX? */
		AQ_WRITE_REG_BIT(sc, TDM_DCAD_REG(ringidx),
		    TDM_DCAD_CPUID, cpuid);
		AQ_WRITE_REG_BIT(sc, TDM_DCAD_REG(ringidx),
		    TDM_DCAD_CPUID_EN, 0);

		txring->txr_active = true;
	}

	mutex_exit(&txring->txr_mutex);
}

static int
aq_rxring_reset(struct aq_softc *sc, struct aq_rxring *rxring, bool start)
{
	const int ringidx = rxring->rxr_index;
	int i;
	int error = 0;

	mutex_enter(&rxring->rxr_mutex);
	rxring->rxr_active = false;

	/* disable DMA */
	AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_REG(ringidx), RX_DMA_DESC_EN, 0);

	/* free all RX mbufs */
	aq_rxdrain(sc, rxring);

	if (start) {
		for (i = 0; i < AQ_RXD_NUM; i++) {
			error = aq_rxring_add(sc, rxring, i);
			if (error != 0) {
				aq_rxdrain(sc, rxring);
				return error;
			}
			aq_rxring_reset_desc(sc, rxring, i);
		}

		/* RX descriptor physical address */
		paddr_t paddr = rxring->rxr_rxdesc_dmamap->dm_segs[0].ds_addr;
		AQ_WRITE_REG(sc, RX_DMA_DESC_BASE_ADDRLSW_REG(ringidx), paddr);
		AQ_WRITE_REG(sc, RX_DMA_DESC_BASE_ADDRMSW_REG(ringidx),
		    (uint32_t)((uint64_t)paddr >> 32));

		/* RX descriptor size */
		AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_REG(ringidx), RX_DMA_DESC_LEN,
		    AQ_RXD_NUM / 8);

		/* maximum receive frame size */
		AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_BUFSIZE_REG(ringidx),
		    RX_DMA_DESC_BUFSIZE_DATA, MCLBYTES / 1024);
		AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_BUFSIZE_REG(ringidx),
		    RX_DMA_DESC_BUFSIZE_HDR, 0 / 1024);

		AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_REG(ringidx),
		    RX_DMA_DESC_HEADER_SPLIT, 0);
		AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_REG(ringidx),
		    RX_DMA_DESC_VLAN_STRIP,
		    (sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_HWTAGGING) ?
		    1 : 0);

		/*
		 * reload TAIL pointer, and update readidx
		 * (HEAD pointer cannot write)
		 */
		rxring->rxr_readidx = AQ_READ_REG_BIT(sc,
		    RX_DMA_DESC_HEAD_PTR_REG(ringidx), RX_DMA_DESC_HEAD_PTR);
		AQ_WRITE_REG(sc, RX_DMA_DESC_TAIL_PTR_REG(ringidx),
		    (rxring->rxr_readidx + AQ_RXD_NUM - 1) % AQ_RXD_NUM);

		/* Rx ring set mode */

		/* Mapping interrupt vector */
		AQ_WRITE_REG_BIT(sc, AQ_INTR_IRQ_MAP_RX_REG(ringidx),
		    AQ_INTR_IRQ_MAP_RX_IRQMAP(ringidx), sc->sc_rx_irq[ringidx]);
		AQ_WRITE_REG_BIT(sc, AQ_INTR_IRQ_MAP_RX_REG(ringidx),
		    AQ_INTR_IRQ_MAP_RX_EN(ringidx), 1);

		const int cpuid = 0;	/* XXX? */
		AQ_WRITE_REG_BIT(sc, RX_DMA_DCAD_REG(ringidx),
		    RX_DMA_DCAD_CPUID, cpuid);
		AQ_WRITE_REG_BIT(sc, RX_DMA_DCAD_REG(ringidx),
		    RX_DMA_DCAD_DESC_EN, 0);
		AQ_WRITE_REG_BIT(sc, RX_DMA_DCAD_REG(ringidx),
		    RX_DMA_DCAD_HEADER_EN, 0);
		AQ_WRITE_REG_BIT(sc, RX_DMA_DCAD_REG(ringidx),
		    RX_DMA_DCAD_PAYLOAD_EN, 0);

		/* enable DMA. start receiving */
		AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_REG(ringidx),
		    RX_DMA_DESC_EN, 1);

		rxring->rxr_active = true;
	}

	mutex_exit(&rxring->rxr_mutex);
	return error;
}

#define TXRING_NEXTIDX(idx)	\
	(((idx) >= (AQ_TXD_NUM - 1)) ? 0 : ((idx) + 1))
#define RXRING_NEXTIDX(idx)	\
	(((idx) >= (AQ_RXD_NUM - 1)) ? 0 : ((idx) + 1))

static int
aq_encap_txring(struct aq_softc *sc, struct aq_txring *txring, struct mbuf **mp)
{
	bus_dmamap_t map;
	struct mbuf *m = *mp;
	uint32_t ctl1, ctl1_ctx, ctl2;
	int idx, i, error;

	idx = txring->txr_prodidx;
	map = txring->txr_mbufs[idx].dmamap;

	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
	    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error == EFBIG) {
		struct mbuf *n;
		n = m_defrag(m, M_DONTWAIT);
		if (n == NULL)
			return EFBIG;
		/* m_defrag() preserve m */
		KASSERT(n == m);
		error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
		    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	}
	if (error != 0)
		return error;

	/*
	 * check spaces of free descriptors.
	 * +1 is additional descriptor for context (vlan, etc,.)
	 */
	if ((map->dm_nsegs + 1) > txring->txr_nfree) {
		device_printf(sc->sc_dev,
		    "TX: not enough descriptors left %d for %d segs\n",
		    txring->txr_nfree, map->dm_nsegs + 1);
		bus_dmamap_unload(sc->sc_dmat, map);
		return ENOBUFS;
	}

	/* sync dma for mbuf */
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	ctl1_ctx = 0;
	ctl2 = __SHIFTIN(m->m_pkthdr.len, AQ_TXDESC_CTL2_LEN);

	if (vlan_has_tag(m)) {
		ctl1 = AQ_TXDESC_CTL1_TYPE_TXC;
		ctl1 |= __SHIFTIN(vlan_get_tag(m), AQ_TXDESC_CTL1_VID);

		ctl1_ctx |= AQ_TXDESC_CTL1_CMD_VLAN;
		ctl2 |= AQ_TXDESC_CTL2_CTX_EN;

		/* fill context descriptor and forward index */
		txring->txr_txdesc[idx].buf_addr = 0;
		txring->txr_txdesc[idx].ctl1 = htole32(ctl1);
		txring->txr_txdesc[idx].ctl2 = 0;

		idx = TXRING_NEXTIDX(idx);
		txring->txr_nfree--;
	}

	if (m->m_pkthdr.csum_flags & M_CSUM_IPv4)
		ctl1_ctx |= AQ_TXDESC_CTL1_CMD_IP4CSUM;
	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TCPv4 | M_CSUM_UDPv4 | M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
		ctl1_ctx |= AQ_TXDESC_CTL1_CMD_L4CSUM;
	}

	/* fill descriptor(s) */
	for (i = 0; i < map->dm_nsegs; i++) {
		ctl1 = ctl1_ctx | AQ_TXDESC_CTL1_TYPE_TXD |
		    __SHIFTIN(map->dm_segs[i].ds_len, AQ_TXDESC_CTL1_BLEN);
		ctl1 |= AQ_TXDESC_CTL1_CMD_FCS;

		if (i == 0) {
			/* remember mbuf of these descriptors */
			txring->txr_mbufs[idx].m = m;
		} else {
			txring->txr_mbufs[idx].m = NULL;
		}

		if (i == map->dm_nsegs - 1) {
			/* last segment, mark an EndOfPacket, and cause intr */
			ctl1 |= AQ_TXDESC_CTL1_EOP | AQ_TXDESC_CTL1_CMD_WB;
		}

		txring->txr_txdesc[idx].buf_addr =
		    htole64(map->dm_segs[i].ds_addr);
		txring->txr_txdesc[idx].ctl1 = htole32(ctl1);
		txring->txr_txdesc[idx].ctl2 = htole32(ctl2);

		bus_dmamap_sync(sc->sc_dmat, txring->txr_txdesc_dmamap,
		    sizeof(aq_tx_desc_t) * idx, sizeof(aq_tx_desc_t),
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		idx = TXRING_NEXTIDX(idx);
		txring->txr_nfree--;
	}

	txring->txr_prodidx = idx;

	return 0;
}

static int
aq_tx_intr(void *arg)
{
	struct aq_txring *txring = arg;
	struct aq_softc *sc = txring->txr_sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	const int ringidx = txring->txr_index;
	unsigned int idx, hw_head, n = 0;

	mutex_enter(&txring->txr_mutex);

	if (!txring->txr_active)
		goto tx_intr_done;

	hw_head = AQ_READ_REG_BIT(sc, TX_DMA_DESC_HEAD_PTR_REG(ringidx),
	    TX_DMA_DESC_HEAD_PTR);
	if (hw_head == txring->txr_considx) {
		goto tx_intr_done;
	}

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);

	for (idx = txring->txr_considx; idx != hw_head;
	    idx = TXRING_NEXTIDX(idx), n++) {

		if ((m = txring->txr_mbufs[idx].m) != NULL) {
			bus_dmamap_unload(sc->sc_dmat,
			    txring->txr_mbufs[idx].dmamap);

			if_statinc_ref(nsr, if_opackets);
			if_statadd_ref(nsr, if_obytes, m->m_pkthdr.len);
			if (m->m_flags & M_MCAST)
				if_statinc_ref(nsr, if_omcasts);

			m_freem(m);
			txring->txr_mbufs[idx].m = NULL;
		}

		txring->txr_nfree++;
	}
	txring->txr_considx = idx;

	IF_STAT_PUTREF(ifp);

	if (ringidx == 0 && txring->txr_nfree >= AQ_TXD_MIN)
		ifp->if_flags &= ~IFF_OACTIVE;

	/* no more pending TX packet, cancel watchdog */
	if (txring->txr_nfree >= AQ_TXD_NUM)
		ifp->if_timer = 0;

 tx_intr_done:
	mutex_exit(&txring->txr_mutex);

	AQ_WRITE_REG(sc, AQ_INTR_STATUS_CLR_REG, __BIT(sc->sc_tx_irq[ringidx]));
	return n;
}

static int
aq_rx_intr(void *arg)
{
	struct aq_rxring *rxring = arg;
	struct aq_softc *sc = rxring->rxr_sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	const int ringidx = rxring->rxr_index;
	aq_rx_desc_t *rxd;
	struct mbuf *m, *m0, *mprev, *new_m;
	uint32_t rxd_type, rxd_hash __unused;
	uint16_t rxd_status, rxd_pktlen;
	uint16_t rxd_nextdescptr __unused, rxd_vlan __unused;
	unsigned int idx, n = 0;

	mutex_enter(&rxring->rxr_mutex);

	if (!rxring->rxr_active)
		goto rx_intr_done;

	if (rxring->rxr_readidx == AQ_READ_REG_BIT(sc,
	    RX_DMA_DESC_HEAD_PTR_REG(ringidx), RX_DMA_DESC_HEAD_PTR)) {
		goto rx_intr_done;
	}

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);

	m0 = mprev = NULL;
	for (idx = rxring->rxr_readidx;
	    idx != AQ_READ_REG_BIT(sc, RX_DMA_DESC_HEAD_PTR_REG(ringidx),
	    RX_DMA_DESC_HEAD_PTR); idx = RXRING_NEXTIDX(idx), n++) {

		bus_dmamap_sync(sc->sc_dmat, rxring->rxr_rxdesc_dmamap,
		    sizeof(aq_rx_desc_t) * idx, sizeof(aq_rx_desc_t),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		rxd = &rxring->rxr_rxdesc[idx];
		rxd_status = le16toh(rxd->wb.status);

		if ((rxd_status & RXDESC_STATUS_DD) == 0)
			break;	/* not yet done */

		rxd_type = le32toh(rxd->wb.type);
		rxd_pktlen = le16toh(rxd->wb.pkt_len);
		rxd_nextdescptr = le16toh(rxd->wb.next_desc_ptr);
		rxd_hash = le32toh(rxd->wb.rss_hash);
		rxd_vlan = le16toh(rxd->wb.vlan);

		if ((rxd_status & RXDESC_STATUS_MACERR) ||
		    (rxd_type & RXDESC_TYPE_MAC_DMA_ERR)) {
			if_statinc_ref(nsr, if_ierrors);
			goto rx_next;
		}

		bus_dmamap_sync(sc->sc_dmat, rxring->rxr_mbufs[idx].dmamap, 0,
		    rxring->rxr_mbufs[idx].dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);
		m = rxring->rxr_mbufs[idx].m;

		new_m = aq_alloc_mbuf();
		if (new_m == NULL) {
			/*
			 * cannot allocate new mbuf.
			 * discard this packet, and reuse mbuf for next.
			 */
			if_statinc_ref(nsr, if_iqdrops);
			goto rx_next;
		}
		rxring->rxr_mbufs[idx].m = NULL;
		aq_rxring_setmbuf(sc, rxring, idx, new_m);

		if (m0 == NULL) {
			m0 = m;
		} else {
			if (m->m_flags & M_PKTHDR)
				m_remove_pkthdr(m);
			mprev->m_next = m;
		}
		mprev = m;

		if ((rxd_status & RXDESC_STATUS_EOP) == 0) {
			m->m_len = MCLBYTES;
		} else {
			/* last buffer */
			int mlen = rxd_pktlen % MCLBYTES;
			if (mlen == 0)
				mlen = MCLBYTES;
			m->m_len = mlen;
			m0->m_pkthdr.len = rxd_pktlen;
			/* VLAN offloading */
			if ((sc->sc_ethercom.ec_capenable &
			    ETHERCAP_VLAN_HWTAGGING) &&
			    (__SHIFTOUT(rxd_type, RXDESC_TYPE_PKTTYPE_VLAN) ||
			    __SHIFTOUT(rxd_type,
			    RXDESC_TYPE_PKTTYPE_VLAN_DOUBLE))) {
				vlan_set_tag(m0, rxd_vlan);
			}

			/* Checksum offloading */
			unsigned int pkttype_eth =
			    __SHIFTOUT(rxd_type, RXDESC_TYPE_PKTTYPE_ETHER);
			if ((ifp->if_capabilities & IFCAP_CSUM_IPv4_Rx) &&
			    (pkttype_eth == RXDESC_TYPE_PKTTYPE_ETHER_IPV4) &&
			    __SHIFTOUT(rxd_type,
			    RXDESC_TYPE_IPV4_CSUM_CHECKED)) {
				m0->m_pkthdr.csum_flags |= M_CSUM_IPv4;
				if (__SHIFTOUT(rxd_status,
				    RXDESC_STATUS_IPV4_CSUM_NG))
					m0->m_pkthdr.csum_flags |=
					    M_CSUM_IPv4_BAD;
			}

			/*
			 * aq will always mark BAD for fragment packets,
			 * but this is not a problem because the IP stack
			 * ignores the CSUM flag in fragment packets.
			 */
			if (__SHIFTOUT(rxd_type,
			    RXDESC_TYPE_TCPUDP_CSUM_CHECKED)) {
				bool checked = false;
				unsigned int pkttype_proto =
				    __SHIFTOUT(rxd_type,
				    RXDESC_TYPE_PKTTYPE_PROTO);

				if (pkttype_proto ==
				    RXDESC_TYPE_PKTTYPE_PROTO_TCP) {
					if ((pkttype_eth ==
					    RXDESC_TYPE_PKTTYPE_ETHER_IPV4) &&
					    (ifp->if_capabilities &
					    IFCAP_CSUM_TCPv4_Rx)) {
						m0->m_pkthdr.csum_flags |=
						    M_CSUM_TCPv4;
						checked = true;
					} else if ((pkttype_eth ==
					    RXDESC_TYPE_PKTTYPE_ETHER_IPV6) &&
					    (ifp->if_capabilities &
					    IFCAP_CSUM_TCPv6_Rx)) {
						m0->m_pkthdr.csum_flags |=
						    M_CSUM_TCPv6;
						checked = true;
					}
				} else if (pkttype_proto ==
				    RXDESC_TYPE_PKTTYPE_PROTO_UDP) {
					if ((pkttype_eth ==
					    RXDESC_TYPE_PKTTYPE_ETHER_IPV4) &&
					    (ifp->if_capabilities &
					    IFCAP_CSUM_UDPv4_Rx)) {
						m0->m_pkthdr.csum_flags |=
						    M_CSUM_UDPv4;
						checked = true;
					} else if ((pkttype_eth ==
					    RXDESC_TYPE_PKTTYPE_ETHER_IPV6) &&
					    (ifp->if_capabilities &
					    IFCAP_CSUM_UDPv6_Rx)) {
						m0->m_pkthdr.csum_flags |=
						    M_CSUM_UDPv6;
						checked = true;
					}
				}
				if (checked &&
				    (__SHIFTOUT(rxd_status,
				    RXDESC_STATUS_TCPUDP_CSUM_ERROR) ||
				    !__SHIFTOUT(rxd_status,
				    RXDESC_STATUS_TCPUDP_CSUM_OK))) {
					m0->m_pkthdr.csum_flags |=
					    M_CSUM_TCP_UDP_BAD;
				}
			}

			m_set_rcvif(m0, ifp);
			if_statinc_ref(nsr, if_ipackets);
			if_statadd_ref(nsr, if_ibytes, m0->m_pkthdr.len);
			if_percpuq_enqueue(ifp->if_percpuq, m0);
			m0 = mprev = NULL;
		}

 rx_next:
		aq_rxring_reset_desc(sc, rxring, idx);
		AQ_WRITE_REG(sc, RX_DMA_DESC_TAIL_PTR_REG(ringidx), idx);
	}
	rxring->rxr_readidx = idx;

	IF_STAT_PUTREF(ifp);

 rx_intr_done:
	mutex_exit(&rxring->rxr_mutex);

	AQ_WRITE_REG(sc, AQ_INTR_STATUS_CLR_REG, __BIT(sc->sc_rx_irq[ringidx]));
	return n;
}

static int
aq_vlan_cb(struct ethercom *ec, uint16_t vid, bool set)
{
	struct ifnet *ifp = &ec->ec_if;
	struct aq_softc *sc = ifp->if_softc;

	aq_set_vlan_filters(sc);
	return 0;
}

static int
aq_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct aq_softc *sc = ifp->if_softc;
	int i, ecchange, error = 0;
	unsigned short iffchange;

	AQ_LOCK(sc);

	iffchange = ifp->if_flags ^ sc->sc_if_flags;
	if ((iffchange & IFF_PROMISC) != 0)
		error = aq_set_filter(sc);

	ecchange = ec->ec_capenable ^ sc->sc_ec_capenable;
	if (ecchange & ETHERCAP_VLAN_HWTAGGING) {
		for (i = 0; i < AQ_RINGS_NUM; i++) {
			AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_REG(i),
			    RX_DMA_DESC_VLAN_STRIP,
			    (ec->ec_capenable & ETHERCAP_VLAN_HWTAGGING) ?
			    1 : 0);
		}
	}

	/* vlan configuration depends on also interface promiscuous mode */
	if ((ecchange & ETHERCAP_VLAN_HWFILTER) || (iffchange & IFF_PROMISC))
		aq_set_vlan_filters(sc);

	sc->sc_ec_capenable = ec->ec_capenable;
	sc->sc_if_flags = ifp->if_flags;

	AQ_UNLOCK(sc);

	return error;
}

static int
aq_init(struct ifnet *ifp)
{
	struct aq_softc *sc = ifp->if_softc;
	int i, error = 0;

	aq_stop(ifp, false);

	AQ_LOCK(sc);

	aq_set_vlan_filters(sc);
	aq_set_capability(sc);

	for (i = 0; i < sc->sc_nqueues; i++) {
		aq_txring_reset(sc, &sc->sc_queue[i].txring, true);
	}

	/* invalidate RX descriptor cache */
	AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_CACHE_INIT_REG, RX_DMA_DESC_CACHE_INIT,
	    AQ_READ_REG_BIT(sc,
	    RX_DMA_DESC_CACHE_INIT_REG, RX_DMA_DESC_CACHE_INIT) ^ 1);

	/* start RX */
	for (i = 0; i < sc->sc_nqueues; i++) {
		error = aq_rxring_reset(sc, &sc->sc_queue[i].rxring, true);
		if (error != 0) {
			device_printf(sc->sc_dev, "%s: cannot allocate rxbuf\n",
			    __func__);
			goto aq_init_failure;
		}
	}
	aq_init_rss(sc);
	aq_hw_l3_filter_set(sc);

	/* need to start callout? */
	if (sc->sc_poll_linkstat
#ifdef AQ_EVENT_COUNTERS
	    || sc->sc_poll_statistics
#endif
	    ) {
		callout_schedule(&sc->sc_tick_ch, hz);
	}

	/* ready */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* start TX and RX */
	aq_enable_intr(sc, true, true);
	AQ_WRITE_REG_BIT(sc, TPB_TX_BUF_REG, TPB_TX_BUF_EN, 1);
	AQ_WRITE_REG_BIT(sc, RPB_RPF_RX_REG, RPB_RPF_RX_BUF_EN, 1);

 aq_init_failure:
	sc->sc_if_flags = ifp->if_flags;

	AQ_UNLOCK(sc);

	return error;
}

static void
aq_send_common_locked(struct ifnet *ifp, struct aq_softc *sc,
    struct aq_txring *txring, bool is_transmit)
{
	struct mbuf *m;
	int npkt, error;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	for (npkt = 0; ; npkt++) {
		if (is_transmit)
			m = pcq_peek(txring->txr_pcq);
		else
			IFQ_POLL(&ifp->if_snd, m);

		if (m == NULL)
			break;

		if (txring->txr_nfree < AQ_TXD_MIN)
			break;

		if (is_transmit)
			pcq_get(txring->txr_pcq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);

		error = aq_encap_txring(sc, txring, &m);
		if (error != 0) {
			/* too many mbuf chains? or not enough descriptors? */
			m_freem(m);
			if_statinc(ifp, if_oerrors);
			if (txring->txr_index == 0 && error == ENOBUFS)
				ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		/* update tail ptr */
		AQ_WRITE_REG(sc, TX_DMA_DESC_TAIL_PTR_REG(txring->txr_index),
		    txring->txr_prodidx);

		/* Pass the packet to any BPF listeners */
		bpf_mtap(ifp, m, BPF_D_OUT);
	}

	if (txring->txr_index == 0 && txring->txr_nfree < AQ_TXD_MIN)
		ifp->if_flags |= IFF_OACTIVE;

	if (npkt)
		ifp->if_timer = 5;
}

static void
aq_start(struct ifnet *ifp)
{
	struct aq_softc *sc;
	struct aq_txring *txring;

	sc = ifp->if_softc;
	txring = &sc->sc_queue[0].txring; /* aq_start() always use TX ring[0] */

	mutex_enter(&txring->txr_mutex);
	if (txring->txr_active && !ISSET(ifp->if_flags, IFF_OACTIVE))
		aq_send_common_locked(ifp, sc, txring, false);
	mutex_exit(&txring->txr_mutex);
}

static inline unsigned int
aq_select_txqueue(struct aq_softc *sc, struct mbuf *m)
{
	return (cpu_index(curcpu()) % sc->sc_nqueues);
}

static int
aq_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct aq_softc *sc = ifp->if_softc;
	struct aq_txring *txring;
	int ringidx;

	ringidx = aq_select_txqueue(sc, m);
	txring = &sc->sc_queue[ringidx].txring;

	if (__predict_false(!pcq_put(txring->txr_pcq, m))) {
		m_freem(m);
		return ENOBUFS;
	}

	if (mutex_tryenter(&txring->txr_mutex)) {
		aq_send_common_locked(ifp, sc, txring, true);
		mutex_exit(&txring->txr_mutex);
	} else {
		softint_schedule(txring->txr_softint);
	}
	return 0;
}

static void
aq_deferred_transmit(void *arg)
{
	struct aq_txring *txring = arg;
	struct aq_softc *sc = txring->txr_sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	mutex_enter(&txring->txr_mutex);
	if (pcq_peek(txring->txr_pcq) != NULL)
		aq_send_common_locked(ifp, sc, txring, true);
	mutex_exit(&txring->txr_mutex);
}

static void
aq_stop(struct ifnet *ifp, int disable)
{
	struct aq_softc *sc = ifp->if_softc;
	int i;

	AQ_LOCK(sc);

	ifp->if_timer = 0;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		goto already_stopped;

	/* disable tx/rx interrupts */
	aq_enable_intr(sc, true, false);

	AQ_WRITE_REG_BIT(sc, TPB_TX_BUF_REG, TPB_TX_BUF_EN, 0);
	for (i = 0; i < sc->sc_nqueues; i++) {
		aq_txring_reset(sc, &sc->sc_queue[i].txring, false);
	}

	AQ_WRITE_REG_BIT(sc, RPB_RPF_RX_REG, RPB_RPF_RX_BUF_EN, 0);
	for (i = 0; i < sc->sc_nqueues; i++) {
		aq_rxring_reset(sc, &sc->sc_queue[i].rxring, false);
	}

	/* invalidate RX descriptor cache */
	AQ_WRITE_REG_BIT(sc, RX_DMA_DESC_CACHE_INIT_REG, RX_DMA_DESC_CACHE_INIT,
	    AQ_READ_REG_BIT(sc,
	    RX_DMA_DESC_CACHE_INIT_REG, RX_DMA_DESC_CACHE_INIT) ^ 1);

	ifp->if_timer = 0;

 already_stopped:
	if (!disable) {
		/* when pmf stop, disable link status intr and callout */
		aq_enable_intr(sc, false, false);
		callout_stop(&sc->sc_tick_ch);
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	AQ_UNLOCK(sc);
}

static void
aq_watchdog(struct ifnet *ifp)
{
	struct aq_softc *sc = ifp->if_softc;
	struct aq_txring *txring;
	int n, head, tail;

	AQ_LOCK(sc);

	device_printf(sc->sc_dev, "%s: INTR_MASK/STATUS = %08x/%08x\n",
	    __func__, AQ_READ_REG(sc, AQ_INTR_MASK_REG),
	    AQ_READ_REG(sc, AQ_INTR_STATUS_REG));

	for (n = 0; n < sc->sc_nqueues; n++) {
		txring = &sc->sc_queue[n].txring;
		head = AQ_READ_REG_BIT(sc,
		    TX_DMA_DESC_HEAD_PTR_REG(txring->txr_index),
		    TX_DMA_DESC_HEAD_PTR),
		tail = AQ_READ_REG(sc,
		    TX_DMA_DESC_TAIL_PTR_REG(txring->txr_index));

		device_printf(sc->sc_dev, "%s: TXring[%d] HEAD/TAIL=%d/%d\n",
		    __func__, txring->txr_index, head, tail);

		aq_tx_intr(txring);
	}

	AQ_UNLOCK(sc);

	aq_init(ifp);
}

static int
aq_ioctl(struct ifnet *ifp, unsigned long cmd, void *data)
{
	struct aq_softc *sc __unused;
	struct ifreq *ifr __unused;
	int error, s;

	sc = (struct aq_softc *)ifp->if_softc;
	ifr = (struct ifreq *)data;
	error = 0;

	s = splnet();
	switch (cmd) {
	case SIOCSIFMTU:
		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > sc->sc_max_mtu) {
			error = EINVAL;
		} else {
			ifp->if_mtu = ifr->ifr_mtu;
			error = 0;	/* no need to reset (no ENETRESET) */
		}
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}
	splx(s);

	if (error != ENETRESET)
		return error;

	switch (cmd) {
	case SIOCSIFCAP:
		error = aq_set_capability(sc);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			break;

		/*
		 * Multicast list has changed; set the hardware filter
		 * accordingly.
		 */
		error = aq_set_filter(sc);
		break;
	}

	return error;
}


MODULE(MODULE_CLASS_DRIVER, if_aq, "pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
if_aq_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_if_aq,
		    cfattach_ioconf_if_aq, cfdata_ioconf_if_aq);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_if_aq,
		    cfattach_ioconf_if_aq, cfdata_ioconf_if_aq);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
