/*	$NetBSD: if_iavfvar.h,v 1.1 2020/09/08 10:05:47 yamaguchi Exp $	*/

/*
 * Copyright (c) 2020 Internet Initiative Japan, Inc.
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

#ifndef _DEV_PCI_IF_IAVFVAR_H_
#define _DEV_PCI_IF_IAVFVAR_H_

/* aq commands */
#define IAVF_AQ_OP_SEND_TO_PF		0x0801
#define IAVF_AQ_OP_MSG_FROM_PF		0x0802
#define IAVF_AQ_OP_SHUTDOWN		0x0803

/* virt channel messages */
#define IAVF_VC_OP_VERSION		1
#define IAVF_VC_OP_RESET_VF		2
#define IAVF_VC_OP_GET_VF_RESOURCES	3
#define IAVF_VC_OP_CONFIG_TX_QUEUE	4
#define IAVF_VC_OP_CONFIG_RX_QUEUE	5
#define IAVF_VC_OP_CONFIG_VSI_QUEUES	6
#define IAVF_VC_OP_CONFIG_IRQ_MAP	7
#define IAVF_VC_OP_ENABLE_QUEUES	8
#define IAVF_VC_OP_DISABLE_QUEUES	9
#define IAVF_VC_OP_ADD_ETH_ADDR		10
#define IAVF_VC_OP_DEL_ETH_ADDR		11
#define IAVF_VC_OP_ADD_VLAN		12
#define IAVF_VC_OP_DEL_VLAN		13
#define IAVF_VC_OP_CONFIG_PROMISC	14
#define IAVF_VC_OP_GET_STATS		15
#define IAVF_VC_OP_EVENT		17
#define IAVF_VC_OP_CONFIG_RSS_KEY	23
#define IAVF_VC_OP_CONFIG_RSS_LUT	24
#define IAVF_VC_OP_GET_RSS_HENA_CAPS	25
#define IAVF_VC_OP_SET_RSS_HENA		26
#define IAVF_VC_OP_ENABLE_VLAN_STRIP	27
#define IAVF_VC_OP_DISABLE_VLAN_STRIP	28
#define IAVF_VC_OP_REQUEST_QUEUES	29

/* virt channel response codes */
#define IAVF_VC_RC_SUCCESS		0
#define IAVF_VC_RC_ERR_PARAM		-5
#define IAVF_VC_RC_ERR_OPCODE		-38
#define IAVF_VC_RC_ERR_CQP_COMPL	-39
#define IAVF_VC_RC_ERR_VF_ID		-40
#define IAVF_VC_RC_ERR_NOT_SUP		-64

/* virt channel events */
#define IAVF_VC_EVENT_LINK_CHANGE	1
#define IAVF_VC_EVENT_RESET_IMPENDING	2
#define IAVF_VC_EVENT_PF_DRIVER_CLOSE	3

/* virt channel offloads */
#define IAVF_VC_OFFLOAD_L2		0x00000001
#define IAVF_VC_OFFLOAD_IWARP		0x00000002
#define IAVF_VC_OFFLOAD_RSVD		0x00000004
#define IAVF_VC_OFFLOAD_RSS_AQ		0x00000008
#define IAVF_VC_OFFLOAD_RSS_REG		0x00000010
#define IAVF_VC_OFFLOAD_WB_ON_ITR	0x00000020
#define IAVF_VC_OFFLOAD_REQ_QUEUES	0x00000040
#define IAVF_VC_OFFLOAD_VLAN		0x00010000
#define IAVF_VC_OFFLOAD_RX_POLLING	0x00020000
#define IAVF_VC_OFFLOAD_RSS_PCTYPE_V2	0x00040000
#define IAVF_VC_OFFLOAD_RSS_PF		0x00080000
#define IAVF_VC_OFFLOAD_ENCAP		0x00100000
#define IAVF_VC_OFFLOAD_ENCAP_CSUM	0x00200000
#define IAVF_VC_OFFLOAD_RX_ENCAP_CSUM	0x00400000

#define IAVF_VC_OFFLOAD_FMT	"\020"					\
				"\027RENCAP" "\026ENCAPC" "\025ENCAP"	\
				"\024RSSPF" "\023RSSV2" "\022RPOLL"	\
				"\021VLAN" "\007REQQ" "\006WB"		\
				"\005RSSREG" "\004RSSAQ" "\003RSVD"	\
				"\002IWARP" "\001L2"

struct iavf_aq_vc {
	uint32_t	 iaq_vc_opcode;
	uint32_t	 iaq_vc_retval;
} __packed;

struct iavf_vc_version_info {
	uint32_t	major;
	uint32_t	minor;
} __packed;

struct iavf_vc_vsi_resource {
	uint16_t	vsi_id;
	uint16_t	num_queue_pairs;
	uint32_t	vsi_type;
	uint16_t	qset_handle;
	uint8_t		default_mac[ETHER_ADDR_LEN];
} __packed;

struct iavf_vc_vf_resource {
	uint16_t	num_vsis;
	uint16_t	num_qp;
	uint16_t	max_vectors;
	uint16_t	max_mtu;
	uint32_t	offload_flags;
	uint32_t	rss_key_size;
	uint32_t	rss_lut_size;
	struct iavf_vc_vsi_resource
			vsi_res[1];
} __packed;

struct iavf_vc_vector_map {
	uint16_t	vsi_id;
	uint16_t	vector_id;
	uint16_t	rxq_map;
	uint16_t	txq_map;
	uint16_t	rxitr_idx;
	uint16_t	txitr_idx;
} __packed;

struct iavf_vc_irq_map_info {
	uint16_t	num_vectors;
	struct iavf_vc_vector_map vecmap[1];
} __packed;

struct iavf_vc_txq_info {
	uint16_t	vsi_id;
	uint16_t	queue_id;
	uint16_t	ring_len;
	uint16_t	headwb_ena;		/* deprecated */
	uint64_t	dma_ring_addr;
	uint64_t	dma_headwb_addr;	/* deprecated */
} __packed;

struct iavf_vc_rxq_info {
	uint16_t	vsi_id;
	uint16_t	queue_id;
	uint32_t	ring_len;
	uint16_t	hdr_size;
	uint16_t	splithdr_ena;
	uint32_t	databuf_size;
	uint32_t	max_pkt_size;
	uint32_t	pad1;
	uint64_t	dma_ring_addr;
	uint32_t	rx_split_pos;
	uint32_t	pad2;
} __packed;

struct iavf_vc_queue_pair_info {
	struct iavf_vc_txq_info	txq;
	struct iavf_vc_rxq_info	rxq;
} __packed;

struct iavf_vc_queue_config_info {
	uint16_t	vsi_id;
	uint16_t	num_queue_pairs;
	uint32_t	pad;
	struct iavf_vc_queue_pair_info qpair[1];
} __packed;

struct iavf_vc_queue_select {
	uint16_t	vsi_id;
	uint16_t	pad;
	uint32_t	rx_queues;
	uint32_t	tx_queues;
} __packed;

struct iavf_vc_promisc_info {
	uint16_t	vsi_id;
	uint16_t	flags;
#define IAVF_FLAG_VF_UNICAST_PROMISC	0x0001
#define IAVF_FLAG_VF_MULTICAST_PROMISC	0x0002
} __packed;

struct iavf_vc_eth_addr {
	uint8_t		addr[ETHER_ADDR_LEN];
	uint8_t		pad[2];
} __packed;

struct iavf_vc_eth_addr_list {
	uint16_t	vsi_id;
	uint16_t	num_elements;
	struct iavf_vc_eth_addr list[1];
} __packed;

struct iavf_vc_pf_event {
	uint32_t	event;
	uint32_t	link_speed;
	uint8_t		link_status;
	uint8_t		pad[3];
	uint32_t	severity;
} __packed;

struct iavf_vc_vlan_filter {
	uint16_t	vsi_id;
	uint16_t	num_vlan_id;
	uint16_t	vlan_id[1];
} __packed;

struct iavf_vc_rss_key {
	uint16_t	vsi_id;
	uint16_t	key_len;
	uint8_t		key[1];
	uint8_t		pad[1];
} __packed;

struct iavf_vc_rss_lut {
	uint16_t	vsi_id;
	uint16_t	lut_entries;
	uint8_t		lut[1];
	uint8_t		pad[1];
}__packed;

#define IAVF_RSS_VSI_LUT_ENTRY_MASK	0x3F

struct iavf_vc_res_request {
	uint16_t	 num_queue_pairs;
} __packed;

#define I40E_MAX_VF_QUEUES	16

#endif
