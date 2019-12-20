/*	$NetBSD: if_ixlvar.h,v 1.2 2019/12/20 02:12:31 yamaguchi Exp $	*/

/*
 * Copyright (c) 2019 Internet Initiative Japan, Inc.
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

#ifndef _DEV_PCI_IF_IXLVAR_H_
#define _DEV_PCI_IF_IXLVAR_H_

enum i40e_filter_pctype {
	/* Note: Values 0-28 are reserved for future use.
	 * Value 29, 30, 32 are not supported on XL710 and X710.
	 */
	I40E_FILTER_PCTYPE_NONF_UNICAST_IPV4_UDP        = 29,
	I40E_FILTER_PCTYPE_NONF_MULTICAST_IPV4_UDP      = 30,
	I40E_FILTER_PCTYPE_NONF_IPV4_UDP                = 31,
	I40E_FILTER_PCTYPE_NONF_IPV4_TCP_SYN_NO_ACK     = 32,
	I40E_FILTER_PCTYPE_NONF_IPV4_TCP                = 33,
	I40E_FILTER_PCTYPE_NONF_IPV4_SCTP               = 34,
	I40E_FILTER_PCTYPE_NONF_IPV4_OTHER              = 35,
	I40E_FILTER_PCTYPE_FRAG_IPV4                    = 36,
	/* Note: Values 37-38 are reserved for future use.
	 * Value 39, 40, 42 are not supported on XL710 and X710.
	 */
	I40E_FILTER_PCTYPE_NONF_UNICAST_IPV6_UDP        = 39,
	I40E_FILTER_PCTYPE_NONF_MULTICAST_IPV6_UDP      = 40,
	I40E_FILTER_PCTYPE_NONF_IPV6_UDP                = 41,
	I40E_FILTER_PCTYPE_NONF_IPV6_TCP_SYN_NO_ACK     = 42,
	I40E_FILTER_PCTYPE_NONF_IPV6_TCP                = 43,
	I40E_FILTER_PCTYPE_NONF_IPV6_SCTP               = 44,
	I40E_FILTER_PCTYPE_NONF_IPV6_OTHER              = 45,
	I40E_FILTER_PCTYPE_FRAG_IPV6                    = 46,
	/* Note: Value 47 is reserved for future use */
	I40E_FILTER_PCTYPE_FCOE_OX                      = 48,
	I40E_FILTER_PCTYPE_FCOE_RX                      = 49,
	I40E_FILTER_PCTYPE_FCOE_OTHER                   = 50,
	/* Note: Values 51-62 are reserved for future use */
	I40E_FILTER_PCTYPE_L2_PAYLOAD                   = 63,
};

enum i40e_reset_type {
	I40E_RESET_POR          = 0,
	I40E_RESET_CORER        = 1,
	I40E_RESET_GLOBR        = 2,
	I40E_RESET_EMPR         = 3,
};

struct ixl_aq_desc {
	uint16_t	iaq_flags;
#define	IXL_AQ_DD		(1U << 0)
#define	IXL_AQ_CMP		(1U << 1)
#define IXL_AQ_ERR		(1U << 2)
#define IXL_AQ_VFE		(1U << 3)
#define IXL_AQ_LB		(1U << 9)
#define IXL_AQ_RD		(1U << 10)
#define IXL_AQ_VFC		(1U << 11)
#define IXL_AQ_BUF		(1U << 12)
#define IXL_AQ_SI		(1U << 13)
#define IXL_AQ_EI		(1U << 14)
#define IXL_AQ_FE		(1U << 15)

#define IXL_AQ_FLAGS_FMT	"\020" "\020FE" "\017EI" "\016SI" "\015BUF" \
				    "\014VFC" "\013DB" "\012LB" "\004VFE" \
				    "\003ERR" "\002CMP" "\001DD"

	uint16_t	iaq_opcode;

	uint16_t	iaq_datalen;
	uint16_t	iaq_retval;

	uint64_t	iaq_cookie;

	uint32_t	iaq_param[4];
/*	iaq_data_hi	iaq_param[2] */
/*	iaq_data_lo	iaq_param[3] */
} __packed __aligned(16);

/* aq commands */
#define IXL_AQ_OP_GET_VERSION		0x0001
#define IXL_AQ_OP_DRIVER_VERSION	0x0002
#define IXL_AQ_OP_QUEUE_SHUTDOWN	0x0003
#define IXL_AQ_OP_SET_PF_CONTEXT	0x0004
#define IXL_AQ_OP_GET_AQ_ERR_REASON	0x0005
#define IXL_AQ_OP_REQUEST_RESOURCE	0x0008
#define IXL_AQ_OP_RELEASE_RESOURCE	0x0009
#define IXL_AQ_OP_LIST_FUNC_CAP		0x000a
#define IXL_AQ_OP_LIST_DEV_CAP		0x000b
#define IXL_AQ_OP_MAC_ADDRESS_READ	0x0107
#define IXL_AQ_OP_CLEAR_PXE_MODE	0x0110
#define IXL_AQ_OP_SWITCH_GET_CONFIG	0x0200
#define IXL_AQ_OP_RX_CTL_REG_READ	0x0206
#define IXL_AQ_OP_RX_CTL_REG_WRITE	0x0207
#define IXL_AQ_OP_ADD_VSI		0x0210
#define IXL_AQ_OP_UPD_VSI_PARAMS	0x0211
#define IXL_AQ_OP_GET_VSI_PARAMS	0x0212
#define IXL_AQ_OP_ADD_VEB		0x0230
#define IXL_AQ_OP_UPD_VEB_PARAMS	0x0231
#define IXL_AQ_OP_GET_VEB_PARAMS	0x0232
#define IXL_AQ_OP_ADD_MACVLAN		0x0250
#define IXL_AQ_OP_REMOVE_MACVLAN	0x0251
#define IXL_AQ_OP_SET_VSI_PROMISC	0x0254
#define IXL_AQ_OP_PHY_GET_ABILITIES	0x0600
#define IXL_AQ_OP_PHY_SET_CONFIG	0x0601
#define IXL_AQ_OP_PHY_SET_MAC_CONFIG	0x0603
#define IXL_AQ_OP_PHY_RESTART_AN	0x0605
#define IXL_AQ_OP_PHY_LINK_STATUS	0x0607
#define IXL_AQ_OP_PHY_SET_EVENT_MASK	0x0613
#define IXL_AQ_OP_PHY_SET_REGISTER	0x0628
#define IXL_AQ_OP_PHY_GET_REGISTER	0x0629
#define IXL_AQ_OP_LLDP_GET_MIB		0x0a00
#define IXL_AQ_OP_LLDP_MIB_CHG_EV	0x0a01
#define IXL_AQ_OP_LLDP_ADD_TLV		0x0a02
#define IXL_AQ_OP_LLDP_UPD_TLV		0x0a03
#define IXL_AQ_OP_LLDP_DEL_TLV		0x0a04
#define IXL_AQ_OP_LLDP_STOP_AGENT	0x0a05
#define IXL_AQ_OP_LLDP_START_AGENT	0x0a06
#define IXL_AQ_OP_LLDP_GET_CEE_DCBX	0x0a07
#define IXL_AQ_OP_LLDP_SPECIFIC_AGENT	0x0a09

struct ixl_aq_mac_addresses {
	uint8_t		pf_lan[ETHER_ADDR_LEN];
	uint8_t		pf_san[ETHER_ADDR_LEN];
	uint8_t		port[ETHER_ADDR_LEN];
	uint8_t		pf_wol[ETHER_ADDR_LEN];
} __packed;

#define IXL_AQ_MAC_PF_LAN_VALID		(1U << 4)
#define IXL_AQ_MAC_PF_SAN_VALID		(1U << 5)
#define IXL_AQ_MAC_PORT_VALID		(1U << 6)
#define IXL_AQ_MAC_PF_WOL_VALID		(1U << 7)

struct ixl_aq_capability {
	uint16_t	cap_id;
#define IXL_AQ_CAP_SWITCH_MODE		0x0001
#define IXL_AQ_CAP_MNG_MODE		0x0002
#define IXL_AQ_CAP_NPAR_ACTIVE		0x0003
#define IXL_AQ_CAP_OS2BMC_CAP		0x0004
#define IXL_AQ_CAP_FUNCTIONS_VALID	0x0005
#define IXL_AQ_CAP_ALTERNATE_RAM	0x0006
#define IXL_AQ_CAP_WOL_AND_PROXY	0x0008
#define IXL_AQ_CAP_SRIOV		0x0012
#define IXL_AQ_CAP_VF			0x0013
#define IXL_AQ_CAP_VMDQ			0x0014
#define IXL_AQ_CAP_8021QBG		0x0015
#define IXL_AQ_CAP_8021QBR		0x0016
#define IXL_AQ_CAP_VSI			0x0017
#define IXL_AQ_CAP_DCB			0x0018
#define IXL_AQ_CAP_FCOE			0x0021
#define IXL_AQ_CAP_ISCSI		0x0022
#define IXL_AQ_CAP_RSS			0x0040
#define IXL_AQ_CAP_RXQ			0x0041
#define IXL_AQ_CAP_TXQ			0x0042
#define IXL_AQ_CAP_MSIX			0x0043
#define IXL_AQ_CAP_VF_MSIX		0x0044
#define IXL_AQ_CAP_FLOW_DIRECTOR	0x0045
#define IXL_AQ_CAP_1588			0x0046
#define IXL_AQ_CAP_IWARP		0x0051
#define IXL_AQ_CAP_LED			0x0061
#define IXL_AQ_CAP_SDP			0x0062
#define IXL_AQ_CAP_MDIO			0x0063
#define IXL_AQ_CAP_WSR_PROT		0x0064
#define IXL_AQ_CAP_NVM_MGMT		0x0080
#define IXL_AQ_CAP_FLEX10		0x00F1
#define IXL_AQ_CAP_CEM			0x00F2
	uint8_t		major_rev;
	uint8_t		minor_rev;
	uint32_t	number;
	uint32_t	logical_id;
	uint32_t	phys_id;
	uint8_t		_reserved[16];
} __packed __aligned(4);

#define IXL_LLDP_SHUTDOWN		0x1

struct ixl_aq_switch_config {
	uint16_t	num_reported;
	uint16_t	num_total;
	uint8_t		_reserved[12];
} __packed __aligned(4);

struct ixl_aq_switch_config_element {
	uint8_t		type;
#define IXL_AQ_SW_ELEM_TYPE_MAC		1
#define IXL_AQ_SW_ELEM_TYPE_PF		2
#define IXL_AQ_SW_ELEM_TYPE_VF		3
#define IXL_AQ_SW_ELEM_TYPE_EMP		4
#define IXL_AQ_SW_ELEM_TYPE_BMC		5
#define IXL_AQ_SW_ELEM_TYPE_PV		16
#define IXL_AQ_SW_ELEM_TYPE_VEB		17
#define IXL_AQ_SW_ELEM_TYPE_PA		18
#define IXL_AQ_SW_ELEM_TYPE_VSI		19
	uint8_t		revision;
#define IXL_AQ_SW_ELEM_REV_1		1
	uint16_t	seid;

	uint16_t	uplink_seid;
	uint16_t	downlink_seid;

	uint8_t		_reserved[3];
	uint8_t		connection_type;
#define IXL_AQ_CONN_TYPE_REGULAR	0x1
#define IXL_AQ_CONN_TYPE_DEFAULT	0x2
#define IXL_AQ_CONN_TYPE_CASCADED	0x3

	uint16_t	scheduler_id;
	uint16_t	element_info;
} __packed __aligned(4);

#define IXL_PHY_TYPE_SGMII		0x00
#define IXL_PHY_TYPE_1000BASE_KX	0x01
#define IXL_PHY_TYPE_10GBASE_KX4	0x02
#define IXL_PHY_TYPE_10GBASE_KR		0x03
#define IXL_PHY_TYPE_40GBASE_KR4	0x04
#define IXL_PHY_TYPE_XAUI		0x05
#define IXL_PHY_TYPE_XFI		0x06
#define IXL_PHY_TYPE_SFI		0x07
#define IXL_PHY_TYPE_XLAUI		0x08
#define IXL_PHY_TYPE_XLPPI		0x09
#define IXL_PHY_TYPE_40GBASE_CR4_CU	0x0a
#define IXL_PHY_TYPE_10GBASE_CR1_CU	0x0b
#define IXL_PHY_TYPE_10GBASE_AOC	0x0c
#define IXL_PHY_TYPE_40GBASE_AOC	0x0d
#define IXL_PHY_TYPE_100BASE_TX		0x11
#define IXL_PHY_TYPE_1000BASE_T		0x12
#define IXL_PHY_TYPE_10GBASE_T		0x13
#define IXL_PHY_TYPE_10GBASE_SR		0x14
#define IXL_PHY_TYPE_10GBASE_LR		0x15
#define IXL_PHY_TYPE_10GBASE_SFPP_CU	0x16
#define IXL_PHY_TYPE_10GBASE_CR1	0x17
#define IXL_PHY_TYPE_40GBASE_CR4	0x18
#define IXL_PHY_TYPE_40GBASE_SR4	0x19
#define IXL_PHY_TYPE_40GBASE_LR4	0x1a
#define IXL_PHY_TYPE_1000BASE_SX	0x1b
#define IXL_PHY_TYPE_1000BASE_LX	0x1c
#define IXL_PHY_TYPE_1000BASE_T_OPTICAL	0x1d
#define IXL_PHY_TYPE_20GBASE_KR2	0x1e

#define IXL_PHY_TYPE_25GBASE_KR		0x1f
#define IXL_PHY_TYPE_25GBASE_CR		0x20
#define IXL_PHY_TYPE_25GBASE_SR		0x21
#define IXL_PHY_TYPE_25GBASE_LR		0x22
#define IXL_PHY_TYPE_25GBASE_AOC	0x23
#define IXL_PHY_TYPE_25GBASE_ACC	0x24

struct ixl_aq_module_desc {
	uint8_t		oui[3];
	uint8_t		_reserved1;
	uint8_t		part_number[16];
	uint8_t		revision[4];
	uint8_t		_reserved2[8];
} __packed __aligned(4);

struct ixl_aq_phy_abilities {
	uint32_t	phy_type;

	uint8_t		link_speed;
#define IXL_AQ_PHY_LINK_SPEED_100MB	(1 << 1)
#define IXL_AQ_PHY_LINK_SPEED_1000MB	(1 << 2)
#define IXL_AQ_PHY_LINK_SPEED_10GB	(1 << 3)
#define IXL_AQ_PHY_LINK_SPEED_40GB	(1 << 4)
#define IXL_AQ_PHY_LINK_SPEED_20GB	(1 << 5)
#define IXL_AQ_PHY_LINK_SPEED_25GB	(1 << 6)
	uint8_t		abilities;
	uint16_t	eee_capability;

	uint32_t	eeer_val;

	uint8_t		d3_lpan;
	uint8_t		phy_type_ext;
#define IXL_AQ_PHY_TYPE_EXT_25G_KR	0x01
#define IXL_AQ_PHY_TYPE_EXT_25G_CR	0x02
#define IXL_AQ_PHY_TYPE_EXT_25G_SR	0x04
#define IXL_AQ_PHY_TYPE_EXT_25G_LR	0x08
	uint8_t		fec_cfg_curr_mod_ext_info;
#define IXL_AQ_ENABLE_FEC_KR		0x01
#define IXL_AQ_ENABLE_FEC_RS		0x02
#define IXL_AQ_REQUEST_FEC_KR		0x04
#define IXL_AQ_REQUEST_FEC_RS		0x08
#define IXL_AQ_ENABLE_FEC_AUTO		0x10
#define IXL_AQ_MODULE_TYPE_EXT_MASK	0xe0
#define IXL_AQ_MODULE_TYPE_EXT_SHIFT	5
	uint8_t		ext_comp_code;

	uint8_t		phy_id[4];

	uint8_t		module_type[3];
#define IXL_SFF8024_ID_SFP		0x03
#define IXL_SFF8024_ID_QSFP		0x0c
#define IXL_SFF8024_ID_QSFP_PLUS	0x0d
#define IXL_SFF8024_ID_QSFP28		0x11
	uint8_t		qualified_module_count;
#define IXL_AQ_PHY_MAX_QMS		16
	struct ixl_aq_module_desc
			qualified_module[IXL_AQ_PHY_MAX_QMS];
} __packed __aligned(4);

struct ixl_aq_link_param {
	uint8_t		notify;
#define IXL_AQ_LINK_NOTIFY	0x03
	uint8_t		_reserved1;
	uint8_t		phy;
	uint8_t		speed;
	uint8_t		status;
	uint8_t		_reserved2[11];
} __packed __aligned(4);

struct ixl_aq_vsi_param {
	uint16_t	uplink_seid;
	uint8_t		connect_type;
#define IXL_AQ_VSI_CONN_TYPE_NORMAL	(0x1)
#define IXL_AQ_VSI_CONN_TYPE_DEFAULT	(0x2)
#define IXL_AQ_VSI_CONN_TYPE_CASCADED	(0x3)
	uint8_t		_reserved1;

	uint8_t		vf_id;
	uint8_t		_reserved2;
	uint16_t	vsi_flags;
#define IXL_AQ_VSI_TYPE_SHIFT		0x0
#define IXL_AQ_VSI_TYPE_MASK		(0x3 << IXL_AQ_VSI_TYPE_SHIFT)
#define IXL_AQ_VSI_TYPE_VF		0x0
#define IXL_AQ_VSI_TYPE_VMDQ2		0x1
#define IXL_AQ_VSI_TYPE_PF		0x2
#define IXL_AQ_VSI_TYPE_EMP_MNG		0x3
#define IXL_AQ_VSI_FLAG_CASCADED_PV	0x4

	uint32_t	addr_hi;
	uint32_t	addr_lo;
} __packed __aligned(16);

struct ixl_aq_add_macvlan {
	uint16_t	num_addrs;
	uint16_t	seid0;
	uint16_t	seid1;
	uint16_t	seid2;
	uint32_t	addr_hi;
	uint32_t	addr_lo;
} __packed __aligned(16);

struct ixl_aq_add_macvlan_elem {
	uint8_t		macaddr[6];
	uint16_t	vlan;
	uint16_t	flags;
#define IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH	0x0001
#define IXL_AQ_OP_ADD_MACVLAN_IGNORE_VLAN	0x0004
	uint16_t	queue;
	uint32_t	_reserved;
} __packed __aligned(16);

struct ixl_aq_remove_macvlan {
	uint16_t	num_addrs;
	uint16_t	seid0;
	uint16_t	seid1;
	uint16_t	seid2;
	uint32_t	addr_hi;
	uint32_t	addr_lo;
} __packed __aligned(16);

struct ixl_aq_remove_macvlan_elem {
	uint8_t		macaddr[6];
	uint16_t	vlan;
	uint8_t		flags;
#define IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH	0x0001
#define IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN	0x0008
	uint8_t		_reserved[7];
} __packed __aligned(16);

struct ixl_aq_vsi_reply {
	uint16_t	seid;
	uint16_t	vsi_number;

	uint16_t	vsis_used;
	uint16_t	vsis_free;

	uint32_t	addr_hi;
	uint32_t	addr_lo;
} __packed __aligned(16);

struct ixl_aq_vsi_data {
	/* first 96 byte are written by SW */
	uint16_t	valid_sections;
#define IXL_AQ_VSI_VALID_SWITCH		(1 << 0)
#define IXL_AQ_VSI_VALID_SECURITY	(1 << 1)
#define IXL_AQ_VSI_VALID_VLAN		(1 << 2)
#define IXL_AQ_VSI_VALID_CAS_PV		(1 << 3)
#define IXL_AQ_VSI_VALID_INGRESS_UP	(1 << 4)
#define IXL_AQ_VSI_VALID_EGRESS_UP	(1 << 5)
#define IXL_AQ_VSI_VALID_QUEUE_MAP	(1 << 6)
#define IXL_AQ_VSI_VALID_QUEUE_OPT	(1 << 7)
#define IXL_AQ_VSI_VALID_OUTER_UP	(1 << 8)
#define IXL_AQ_VSI_VALID_SCHED		(1 << 9)
	/* switch section */
	uint16_t	switch_id;
#define IXL_AQ_VSI_SWITCH_ID_SHIFT	0
#define IXL_AQ_VSI_SWITCH_ID_MASK	(0xfff << IXL_AQ_VSI_SWITCH_ID_SHIFT)
#define IXL_AQ_VSI_SWITCH_NOT_STAG	(1 << 12)
#define IXL_AQ_VSI_SWITCH_LOCAL_LB	(1 << 14)

	uint8_t		_reserved1[2];
	/* security section */
	uint8_t		sec_flags;
#define IXL_AQ_VSI_SEC_ALLOW_DEST_OVRD	(1 << 0)
#define IXL_AQ_VSI_SEC_ENABLE_VLAN_CHK	(1 << 1)
#define IXL_AQ_VSI_SEC_ENABLE_MAC_CHK	(1 << 2)
	uint8_t		_reserved2;

	/* vlan section */
	uint16_t	pvid;
	uint16_t	fcoe_pvid;

	uint8_t		port_vlan_flags;
#define IXL_AQ_VSI_PVLAN_MODE_SHIFT	0
#define IXL_AQ_VSI_PVLAN_MODE_MASK	(0x3 << IXL_AQ_VSI_PVLAN_MODE_SHIFT)
#define IXL_AQ_VSI_PVLAN_MODE_TAGGED	(0x1 << IXL_AQ_VSI_PVLAN_MODE_SHIFT)
#define IXL_AQ_VSI_PVLAN_MODE_UNTAGGED	(0x2 << IXL_AQ_VSI_PVLAN_MODE_SHIFT)
#define IXL_AQ_VSI_PVLAN_MODE_ALL	(0x3 << IXL_AQ_VSI_PVLAN_MODE_SHIFT)
#define IXL_AQ_VSI_PVLAN_INSERT_PVID	(0x4 << IXL_AQ_VSI_PVLAN_MODE_SHIFT)
#define IXL_AQ_VSI_PVLAN_EMOD_SHIFT	0x3
#define IXL_AQ_VSI_PVLAN_EMOD_MASK	(0x3 << IXL_AQ_VSI_PVLAN_EMOD_SHIFT)
#define IXL_AQ_VSI_PVLAN_EMOD_STR_BOTH	(0x0 << IXL_AQ_VSI_PVLAN_EMOD_SHIFT)
#define IXL_AQ_VSI_PVLAN_EMOD_STR_UP	(0x1 << IXL_AQ_VSI_PVLAN_EMOD_SHIFT)
#define IXL_AQ_VSI_PVLAN_EMOD_STR	(0x2 << IXL_AQ_VSI_PVLAN_EMOD_SHIFT)
#define IXL_AQ_VSI_PVLAN_EMOD_NOTHING	(0x3 << IXL_AQ_VSI_PVLAN_EMOD_SHIFT)
	uint8_t		_reserved3[3];

	/* ingress egress up section */
	uint32_t	ingress_table;
#define IXL_AQ_VSI_UP_SHIFT(_up)	((_up) * 3)
#define IXL_AQ_VSI_UP_MASK(_up)		(0x7 << (IXL_AQ_VSI_UP_SHIFT(_up))
	uint32_t	egress_table;

	/* cascaded pv section */
	uint16_t	cas_pv_tag;
	uint8_t		cas_pv_flags;
#define IXL_AQ_VSI_CAS_PV_TAGX_SHIFT	0
#define IXL_AQ_VSI_CAS_PV_TAGX_MASK	(0x3 << IXL_AQ_VSI_CAS_PV_TAGX_SHIFT)
#define IXL_AQ_VSI_CAS_PV_TAGX_LEAVE	(0x0 << IXL_AQ_VSI_CAS_PV_TAGX_SHIFT)
#define IXL_AQ_VSI_CAS_PV_TAGX_REMOVE	(0x1 << IXL_AQ_VSI_CAS_PV_TAGX_SHIFT)
#define IXL_AQ_VSI_CAS_PV_TAGX_COPY	(0x2 << IXL_AQ_VSI_CAS_PV_TAGX_SHIFT)
#define IXL_AQ_VSI_CAS_PV_INSERT_TAG	(1 << 4)
#define IXL_AQ_VSI_CAS_PV_ETAG_PRUNE	(1 << 5)
#define IXL_AQ_VSI_CAS_PV_ACCEPT_HOST_TAG \
					(1 << 6)
	uint8_t		_reserved4;

	/* queue mapping section */
	uint16_t	mapping_flags;
#define IXL_AQ_VSI_QUE_MAP_MASK		0x1
#define IXL_AQ_VSI_QUE_MAP_CONTIG	0x0
#define IXL_AQ_VSI_QUE_MAP_NONCONTIG	0x1
	uint16_t	queue_mapping[16];
#define IXL_AQ_VSI_QUEUE_SHIFT		0x0
#define IXL_AQ_VSI_QUEUE_MASK		(0x7ff << IXL_AQ_VSI_QUEUE_SHIFT)
	uint16_t	tc_mapping[8];
#define IXL_AQ_VSI_TC_Q_OFFSET_SHIFT	0
#define IXL_AQ_VSI_TC_Q_OFFSET_MASK	(0x1ff << IXL_AQ_VSI_TC_Q_OFFSET_SHIFT)
#define IXL_AQ_VSI_TC_Q_NUMBER_SHIFT	9
#define IXL_AQ_VSI_TC_Q_NUMBER_MASK	(0x7 << IXL_AQ_VSI_TC_Q_NUMBER_SHIFT)

	/* queueing option section */
	uint8_t		queueing_opt_flags;
#define IXL_AQ_VSI_QUE_OPT_MCAST_UDP_EN	(1 << 2)
#define IXL_AQ_VSI_QUE_OPT_UCAST_UDP_EN	(1 << 3)
#define IXL_AQ_VSI_QUE_OPT_TCP_EN	(1 << 4)
#define IXL_AQ_VSI_QUE_OPT_FCOE_EN	(1 << 5)
#define IXL_AQ_VSI_QUE_OPT_RSS_LUT_PF	0
#define IXL_AQ_VSI_QUE_OPT_RSS_LUT_VSI	(1 << 6)
	uint8_t		_reserved5[3];

	/* scheduler section */
	uint8_t		up_enable_bits;
	uint8_t		_reserved6;

	/* outer up section */
	uint32_t	outer_up_table; /* same as ingress/egress tables */
	uint8_t		_reserved7[8];

	/* last 32 bytes are written by FW */
	uint16_t	qs_handle[8];
#define IXL_AQ_VSI_QS_HANDLE_INVALID	0xffff
	uint16_t	stat_counter_idx;
	uint16_t	sched_id;

	uint8_t		_reserved8[12];
} __packed __aligned(8);

CTASSERT(sizeof(struct ixl_aq_vsi_data) == 128);

struct ixl_aq_vsi_promisc_param {
	uint16_t	flags;
	uint16_t	valid_flags;
#define IXL_AQ_VSI_PROMISC_FLAG_UCAST	(1 << 0)
#define IXL_AQ_VSI_PROMISC_FLAG_MCAST	(1 << 1)
#define IXL_AQ_VSI_PROMISC_FLAG_BCAST	(1 << 2)
#define IXL_AQ_VSI_PROMISC_FLAG_DFLT	(1 << 3)
#define IXL_AQ_VSI_PROMISC_FLAG_VLAN	(1 << 4)
#define IXL_AQ_VSI_PROMISC_FLAG_RXONLY	(1 << 15)

	uint16_t	seid;
#define IXL_AQ_VSI_PROMISC_SEID_VALID	(1 << 15)
	uint16_t	vlan;
#define IXL_AQ_VSI_PROMISC_VLAN_VALID	(1 << 15)
	uint32_t	reserved[2];
} __packed __aligned(8);

struct ixl_aq_veb_param {
	uint16_t	uplink_seid;
	uint16_t	downlink_seid;
	uint16_t	veb_flags;
#define IXL_AQ_ADD_VEB_FLOATING		(1 << 0)
#define IXL_AQ_ADD_VEB_PORT_TYPE_SHIFT	1
#define IXL_AQ_ADD_VEB_PORT_TYPE_MASK	(0x3 << IXL_AQ_ADD_VEB_PORT_TYPE_SHIFT)
#define IXL_AQ_ADD_VEB_PORT_TYPE_DEFAULT \
					(0x2 << IXL_AQ_ADD_VEB_PORT_TYPE_SHIFT)
#define IXL_AQ_ADD_VEB_PORT_TYPE_DATA	(0x4 << IXL_AQ_ADD_VEB_PORT_TYPE_SHIFT)
#define IXL_AQ_ADD_VEB_ENABLE_L2_FILTER	(1 << 3) /* deprecated */
#define IXL_AQ_ADD_VEB_DISABLE_STATS	(1 << 4)
	uint8_t		enable_tcs;
	uint8_t		_reserved[9];
} __packed __aligned(16);

struct ixl_aq_veb_reply {
	uint16_t	_reserved1;
	uint16_t	_reserved2;
	uint16_t	_reserved3;
	uint16_t	switch_seid;
	uint16_t	veb_seid;
#define IXL_AQ_VEB_ERR_FLAG_NO_VEB	(1 << 0)
#define IXL_AQ_VEB_ERR_FLAG_NO_SCHED	(1 << 1)
#define IXL_AQ_VEB_ERR_FLAG_NO_COUNTER	(1 << 2)
#define IXL_AQ_VEB_ERR_FLAG_NO_ENTRY	(1 << 3);
	uint16_t	statistic_index;
	uint16_t	vebs_used;
	uint16_t	vebs_free;
} __packed __aligned(16);

/* GET PHY ABILITIES param[0] */
#define IXL_AQ_PHY_REPORT_QUAL		(1 << 0)
#define IXL_AQ_PHY_REPORT_INIT		(1 << 1)

struct ixl_aq_phy_reg_access {
	uint8_t		phy_iface;
#define IXL_AQ_PHY_IF_INTERNAL		0
#define IXL_AQ_PHY_IF_EXTERNAL		1
#define IXL_AQ_PHY_IF_MODULE		2
	uint8_t		dev_addr;
	uint16_t	recall;
#define IXL_AQ_PHY_QSFP_DEV_ADDR	0
#define IXL_AQ_PHY_QSFP_LAST		1
	uint32_t	reg;
	uint32_t	val;
	uint32_t	_reserved2;
} __packed __aligned(16);

/* RESTART_AN param[0] */
#define IXL_AQ_PHY_RESTART_AN		(1 << 1)
#define IXL_AQ_PHY_LINK_ENABLE		(1 << 2)

struct ixl_aq_link_status { /* this occupies the iaq_param space */
	uint16_t	command_flags; /* only field set on command */
#define IXL_AQ_LSE_MASK			0x3
#define IXL_AQ_LSE_NOP			0x0
#define IXL_AQ_LSE_DISABLE		0x2
#define IXL_AQ_LSE_ENABLE		0x3
#define IXL_AQ_LSE_IS_ENABLED		0x1 /* only set in response */
	uint8_t		phy_type;
	uint8_t		link_speed;
#define IXL_AQ_LINK_SPEED_100MB		(1 << 1)
#define IXL_AQ_LINK_SPEED_1000MB	(1 << 2)
#define IXL_AQ_LINK_SPEED_10GB		(1 << 3)
#define IXL_AQ_LINK_SPEED_40GB		(1 << 4)
#define IXL_AQ_LINK_SPEED_25GB		(1 << 6)
	uint8_t		link_info;
#define IXL_AQ_LINK_UP_FUNCTION		0x01
#define IXL_AQ_LINK_FAULT		0x02
#define IXL_AQ_LINK_FAULT_TX		0x04
#define IXL_AQ_LINK_FAULT_RX		0x08
#define IXL_AQ_LINK_FAULT_REMOTE	0x10
#define IXL_AQ_LINK_UP_PORT		0x20
#define IXL_AQ_MEDIA_AVAILABLE		0x40
#define IXL_AQ_SIGNAL_DETECT		0x80
	uint8_t		an_info;
#define IXL_AQ_AN_COMPLETED		0x01
#define IXL_AQ_LP_AN_ABILITY		0x02
#define IXL_AQ_PD_FAULT			0x04
#define IXL_AQ_FEC_EN			0x08
#define IXL_AQ_PHY_LOW_POWER		0x10
#define IXL_AQ_LINK_PAUSE_TX		0x20
#define IXL_AQ_LINK_PAUSE_RX		0x40
#define IXL_AQ_QUALIFIED_MODULE		0x80

	uint8_t		ext_info;
#define IXL_AQ_LINK_PHY_TEMP_ALARM	0x01
#define IXL_AQ_LINK_XCESSIVE_ERRORS	0x02
#define IXL_AQ_LINK_TX_SHIFT		0x02
#define IXL_AQ_LINK_TX_MASK		(0x03 << IXL_AQ_LINK_TX_SHIFT)
#define IXL_AQ_LINK_TX_ACTIVE		0x00
#define IXL_AQ_LINK_TX_DRAINED		0x01
#define IXL_AQ_LINK_TX_FLUSHED		0x03
#define IXL_AQ_LINK_FORCED_40G		0x10
/* 25G Error Codes */
#define IXL_AQ_25G_NO_ERR		0X00
#define IXL_AQ_25G_NOT_PRESENT		0X01
#define IXL_AQ_25G_NVM_CRC_ERR		0X02
#define IXL_AQ_25G_SBUS_UCODE_ERR	0X03
#define IXL_AQ_25G_SERDES_UCODE_ERR	0X04
#define IXL_AQ_25G_NIMB_UCODE_ERR	0X05
	uint8_t		loopback;
	uint16_t	max_frame_size;

	uint8_t		config;
#define IXL_AQ_CONFIG_FEC_KR_ENA	0x01
#define IXL_AQ_CONFIG_FEC_RS_ENA	0x02
#define IXL_AQ_CONFIG_CRC_ENA	0x04
#define IXL_AQ_CONFIG_PACING_MASK	0x78
	uint8_t		power_desc;
#define IXL_AQ_LINK_POWER_CLASS_1	0x00
#define IXL_AQ_LINK_POWER_CLASS_2	0x01
#define IXL_AQ_LINK_POWER_CLASS_3	0x02
#define IXL_AQ_LINK_POWER_CLASS_4	0x03
#define IXL_AQ_PWR_CLASS_MASK		0x03

	uint8_t		reserved[4];
} __packed __aligned(4);

/* event mask command flags for param[2] */
#define IXL_AQ_PHY_EV_MASK		0x3ff
#define IXL_AQ_PHY_EV_LINK_UPDOWN	(1 << 1)
#define IXL_AQ_PHY_EV_MEDIA_NA		(1 << 2)
#define IXL_AQ_PHY_EV_LINK_FAULT	(1 << 3)
#define IXL_AQ_PHY_EV_PHY_TEMP_ALARM	(1 << 4)
#define IXL_AQ_PHY_EV_EXCESS_ERRORS	(1 << 5)
#define IXL_AQ_PHY_EV_SIGNAL_DETECT	(1 << 6)
#define IXL_AQ_PHY_EV_AN_COMPLETED	(1 << 7)
#define IXL_AQ_PHY_EV_MODULE_QUAL_FAIL	(1 << 8)
#define IXL_AQ_PHY_EV_PORT_TX_SUSPENDED	(1 << 9)

/* aq response codes */
#define IXL_AQ_RC_OK			0  /* success */
#define IXL_AQ_RC_EPERM			1  /* Operation not permitted */
#define IXL_AQ_RC_ENOENT		2  /* No such element */
#define IXL_AQ_RC_ESRCH			3  /* Bad opcode */
#define IXL_AQ_RC_EINTR			4  /* operation interrupted */
#define IXL_AQ_RC_EIO			5  /* I/O error */
#define IXL_AQ_RC_ENXIO			6  /* No such resource */
#define IXL_AQ_RC_E2BIG			7  /* Arg too long */
#define IXL_AQ_RC_EAGAIN		8  /* Try again */
#define IXL_AQ_RC_ENOMEM		9  /* Out of memory */
#define IXL_AQ_RC_EACCES		10 /* Permission denied */
#define IXL_AQ_RC_EFAULT		11 /* Bad address */
#define IXL_AQ_RC_EBUSY			12 /* Device or resource busy */
#define IXL_AQ_RC_EEXIST		13 /* object already exists */
#define IXL_AQ_RC_EINVAL		14 /* invalid argument */
#define IXL_AQ_RC_ENOTTY		15 /* not a typewriter */
#define IXL_AQ_RC_ENOSPC		16 /* No space or alloc failure */
#define IXL_AQ_RC_ENOSYS		17 /* function not implemented */
#define IXL_AQ_RC_ERANGE		18 /* parameter out of range */
#define IXL_AQ_RC_EFLUSHED		19 /* cmd flushed due to prev error */
#define IXL_AQ_RC_BAD_ADDR		20 /* contains a bad pointer */
#define IXL_AQ_RC_EMODE			21 /* not allowed in current mode */
#define IXL_AQ_RC_EFBIG			22 /* file too large */

struct ixl_tx_desc {
	uint64_t		addr;
	uint64_t		cmd;
#define IXL_TX_DESC_DTYPE_SHIFT		0
#define IXL_TX_DESC_DTYPE_MASK		(0xfULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_DATA		(0x0ULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_NOP		(0x1ULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_CONTEXT	(0x1ULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_FCOE_CTX	(0x2ULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_FD		(0x8ULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_DDP_CTX	(0x9ULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_FLEX_DATA	(0xbULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_FLEX_CTX_1	(0xcULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_FLEX_CTX_2	(0xdULL << IXL_TX_DESC_DTYPE_SHIFT)
#define IXL_TX_DESC_DTYPE_DONE		(0xfULL << IXL_TX_DESC_DTYPE_SHIFT)

#define IXL_TX_DESC_CMD_SHIFT		4
#define IXL_TX_DESC_CMD_MASK		(0x3ffULL << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_EOP		(0x001 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_RS		(0x002 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_ICRC		(0x004 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_IL2TAG1		(0x008 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_DUMMY		(0x010 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_IIPT_MASK	(0x060 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_IIPT_NONIP	(0x000 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_IIPT_IPV6	(0x020 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_IIPT_IPV4	(0x040 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_IIPT_IPV4_CSUM	(0x060 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_FCOET		(0x080 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_L4T_EOFT_MASK	(0x300 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_L4T_EOFT_UNK	(0x000 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_L4T_EOFT_TCP	(0x100 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_L4T_EOFT_SCTP	(0x200 << IXL_TX_DESC_CMD_SHIFT)
#define IXL_TX_DESC_CMD_L4T_EOFT_UDP	(0x300 << IXL_TX_DESC_CMD_SHIFT)

#define IXL_TX_DESC_MACLEN_SHIFT	16
#define IXL_TX_DESC_MACLEN_MASK		(0x7fULL << IXL_TX_DESC_MACLEN_SHIFT)
#define IXL_TX_DESC_IPLEN_SHIFT		23
#define IXL_TX_DESC_IPLEN_MASK		(0x7fULL << IXL_TX_DESC_IPLEN_SHIFT)
#define IXL_TX_DESC_L4LEN_SHIFT		30
#define IXL_TX_DESC_L4LEN_MASK		(0xfULL << IXL_TX_DESC_L4LEN_SHIFT)
#define IXL_TX_DESC_FCLEN_SHIFT		30
#define IXL_TX_DESC_FCLEN_MASK		(0xfULL << IXL_TX_DESC_FCLEN_SHIFT)

#define IXL_TX_DESC_BSIZE_SHIFT		34
#define IXL_TX_DESC_BSIZE_MAX		0x3fffULL
#define IXL_TX_DESC_BSIZE_MASK		\
	(IXL_TX_DESC_BSIZE_MAX << IXL_TX_DESC_BSIZE_SHIFT)
#define IXL_TX_DESC_L2TAG1_SHIFT	48
} __packed __aligned(16);

struct ixl_rx_rd_desc_16 {
	uint64_t		paddr; /* packet addr */
	uint64_t		haddr; /* header addr */
} __packed __aligned(16);

struct ixl_rx_rd_desc_32 {
	uint64_t		paddr; /* packet addr */
	uint64_t		haddr; /* header addr */
	uint64_t		_reserved1;
	uint64_t		_reserved2;
} __packed __aligned(16);

struct ixl_rx_wb_desc_16 {
	uint64_t		qword0;
#define IXL_RX_DESC_L2TAG1_SHIFT	16
#define IXL_RX_DESC_L2TAG1_MASK		(0xffffULL << IXL_RX_DESC_L2TAG1_SHIFT)
	uint64_t		qword1;
#define IXL_RX_DESC_DD			(1 << 0)
#define IXL_RX_DESC_EOP			(1 << 1)
#define IXL_RX_DESC_L2TAG1P		(1 << 2)
#define IXL_RX_DESC_L3L4P		(1 << 3)
#define IXL_RX_DESC_CRCP		(1 << 4)
#define IXL_RX_DESC_TSYNINDX_SHIFT	5	/* TSYNINDX */
#define IXL_RX_DESC_TSYNINDX_MASK	(7 << IXL_RX_DESC_TSYNINDX_SHIFT)
#define IXL_RX_DESC_UMB_SHIFT		9
#define IXL_RX_DESC_UMB_MASK		(0x3 << IXL_RX_DESC_UMB_SHIFT)
#define IXL_RX_DESC_UMB_UCAST		(0x0 << IXL_RX_DESC_UMB_SHIFT)
#define IXL_RX_DESC_UMB_MCAST		(0x1 << IXL_RX_DESC_UMB_SHIFT)
#define IXL_RX_DESC_UMB_BCAST		(0x2 << IXL_RX_DESC_UMB_SHIFT)
#define IXL_RX_DESC_UMB_MIRROR		(0x3 << IXL_RX_DESC_UMB_SHIFT)
#define IXL_RX_DESC_FLM			(1 << 11)
#define IXL_RX_DESC_FLTSTAT_SHIFT	12
#define IXL_RX_DESC_FLTSTAT_MASK	(0x3 << IXL_RX_DESC_FLTSTAT_SHIFT)
#define IXL_RX_DESC_FLTSTAT_NODATA	(0x0 << IXL_RX_DESC_FLTSTAT_SHIFT)
#define IXL_RX_DESC_FLTSTAT_FDFILTID	(0x1 << IXL_RX_DESC_FLTSTAT_SHIFT)
#define IXL_RX_DESC_FLTSTAT_RSS		(0x3 << IXL_RX_DESC_FLTSTAT_SHIFT)
#define IXL_RX_DESC_LPBK		(1 << 14)
#define IXL_RX_DESC_IPV6EXTADD		(1 << 15)
#define IXL_RX_DESC_INT_UDP_0		(1 << 18)

#define IXL_RX_DESC_RXE			(1 << 19)
#define IXL_RX_DESC_HBO			(1 << 21)
#define IXL_RX_DESC_IPE			(1 << 22)
#define IXL_RX_DESC_L4E			(1 << 23)
#define IXL_RX_DESC_EIPE		(1 << 24)
#define IXL_RX_DESC_OVERSIZE		(1 << 25)

#define IXL_RX_DESC_PTYPE_SHIFT		30
#define IXL_RX_DESC_PTYPE_MASK		(0xffULL << IXL_RX_DESC_PTYPE_SHIFT)

#define IXL_RX_DESC_PLEN_SHIFT		38
#define IXL_RX_DESC_PLEN_MASK		(0x3fffULL << IXL_RX_DESC_PLEN_SHIFT)
#define IXL_RX_DESC_HLEN_SHIFT		42
#define IXL_RX_DESC_HLEN_MASK		(0x7ffULL << IXL_RX_DESC_HLEN_SHIFT)
} __packed __aligned(16);

struct ixl_rx_wb_desc_32 {
	uint64_t		qword0;
	uint64_t		qword1;
	uint64_t		qword2;
	uint64_t		qword3;
} __packed __aligned(16);

enum i40e_mac_type {
	I40E_MAC_XL710,
	I40E_MAC_X722,
	I40E_MAC_X722_VF,
	I40E_MAC_VF,
	I40E_MAC_GENERIC
};

#endif
