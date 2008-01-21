/*	$NetBSD: pnpbiosreg.h,v 1.2.48.2 2008/01/21 09:37:14 yamt Exp $ */
/*
 * Copyright (c) 2000 Christian E. Hopps
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* functions */
#define	PNP_FC_GET_NUM_NODES	0x00
#define	PNP_FC_GET_DEVICE_NODE	0x01
#define	PNP_FC_SET_DEVICE_NODE	0x02
#define	PNP_FC_GET_EVENT	0x03
#define	PNP_FC_SEND_MESSAGE	0x04
#define	PNP_FC_GET_DOCK_INFO	0x05
#define	PNP_FC_SET_STATIC_RES	0x09	/* no support currently */
#define	PNP_FC_GET_STATIC_RES	0x0A	/* no support currently */
#define	PNP_FC_GET_APM_TABLE	0x0B
#define	PNP_FC_GET_ISA_CONFIG	0x40	/* no support currently */
#define	PNP_FC_GET_ESCD_SYS_CONFIG	0x41	/* no support currently */
#define	PNP_FC_READ_ESCD_SYS_CONFIG	0x42	/* no support currently */
#define	PNP_FC_WRITE_ESCD_SYS_CONFIG	0x43	/* no support currently */

/* return codes from pnp bios calls */
#define	PNP_RC_SUCCESS			0x00
#define	PNP_RC_ERROR_MASK		0x80
#define	PNP_RC_RESERVED			0x01
#define	PNP_RC_NOT_SET_STATICALLY	0x7f	/* warning */
#define	PNP_RC_UNKNOWN_FUNCTION		0x81
#define	PNP_RC_FUNCTION_NOT_SUPPORTED	0x82
#define	PNP_RC_INVALID_HANDLE		0x83
#define	PNP_RC_BAD_PARAMETER		0x84
#define	PNP_RC_SET_FAILED		0x85
#define	PNP_RC_EVENTS_NOT_PENDING	0x86
#define	PNP_RC_SYSTEM_NOT_DOCKED	0x87
#define	PNP_RC_NO_ISA_PNP_CARDS		0x88
#define	PNP_RC_UNABLE_TO_DETERMINE_DOCK_CAPABILITIES	0x89
#define	PNP_RC_CONFIG_CHANGE_FAILED_NO_BATTERY		0x8a
#define	PNP_RC_CONFIG_CHANGE_FAILED_RESOURCE_CONFLICT	0x8b
#define	PNP_RC_BUFFER_TOO_SMALL		0x8c
#define	PNP_RC_USE_ESCD_SUPPORT		0x8d
#define	PNP_RC_MESSAGE_NOT_SUPPORTED	0x8e
#define	PNP_RC_HARDWARE_ERROR		0x8f

/* event identifiers */
#define	PNP_EID_ABOUT_TO_CHANGE_CONFIG	0x0001
#define	PNP_EID_DOCK_CHANGED		0x0002
#define	PNP_EID_SYSTEM_DEVICE_CHANGED	0x0003
#define	PNP_EID_CONFIG_CHANGE_FAILED	0x0004
#define	PNP_EID_UNKNOWN_SYSTEM_EVENT	0xffff
#define	PNP_EID_OEM_DEFINED_BIT		0x8000

/* response messages */
#define	PNP_RM_OK		0x00
#define	PNP_RM_ABORT		0x01

/* control messages */
#define	PNP_CM_UNDOCK_DEFAULT_ACTION	0x0040
#define	PNP_CM_POWER_OFF		0x0041
#define	PNP_CM_PNP_OS_ACTIVE		0x0042
#define	PNP_CM_PNP_OS_INACTIVE		0x0043
#define	PNP_CM_OEM_DEFINED_BIT		0x8000

/* control flags -- used with [GS]ET_DEVICE_NODE */
#define PNP_CF_DEVCONF_DYNAMIC	0x01
#define PNP_CF_DEVCONF_STATIC	0x02

/* main pnpbios structure -- note not naturally aligned */
struct pnpinstcheck {
	uint32_t	ic_sig;			/* '$PnP' */
	uint8_t		ic_version;		/* 0x10 currently */
	uint8_t		ic_length;		/* 0x21 currently */
	uint16_t	ic_control;
	uint8_t		ic_cksum;
	uint32_t	ic_evaddr;
	uint16_t	ic_rcodeoff;
	uint16_t	ic_rcodeseg;
	uint16_t	ic_pcodeoff;
	uint32_t	ic_pcodeseg;
	uint32_t	ic_oemid;
	uint16_t	ic_rdataseg;
	uint32_t	ic_pdataseg;
} __packed;
#define	PNP_IC_VERSION_1_0		0x10
#define	PNP_IC_CONTORL_EVENT_MASK	0x0003
#define	PNP_IC_CONTROL_EVENT_NONE	0x0000
#define	PNP_IC_CONTROL_EVENT_POLL	0x0001
#define	PNP_IC_CONTROL_EVENT_ASYNC	0x0002

/* structure used by [GS]ET_DEVICE_NODE -- note not naturally aligned */
struct pnpdevnode {
	uint16_t	dn_size;
	uint8_t		dn_handle;
	uint32_t	dn_product;
	uint8_t		dn_type;	/* base type */
	uint8_t		dn_subtype;	/* sub type depends on base */
	uint8_t		dn_dpi;		/* dev prog intf depends on subtype */
	uint16_t	dn_attr;
	/* variable - allocated resource */
	/* variable - possible resource */
	/* variable - compatible identifiers */
} __packed;
#define	PNP_DN_ATTR_CONFIG_TIME_MASK		0x0180
#define	PNP_DN_ATTR_CONFIG_TIME_NEXT_BOOT	0x0000
#define	PNP_DN_ATTR_CONFIG_TIME_BOTH		0x0080
#define	PNP_DN_ATTR_CONFIG_TIME_RUNTIME		0x0180
#define	PNP_DN_ATTR_REMOVABLE		0x0040
#define	PNP_DN_ATTR_DOCK_DEVICE		0x0010
#define	PNP_DN_ATTR_CAP_PRIMARY_IPL	0x0010
#define	PNP_DN_ATTR_CAP_PRIMARY_INPUT	0x0008
#define	PNP_DN_ATTR_CAP_PRIMARY_OUTPUT	0x0004
#define	PNP_DN_ATTR_CAN_CONFIGURE	0x0002
#define	PNP_DN_ATTR_CAN_DISABLE		0x0001


/* returned by GET_DOCK_INFO bios call */
struct pnpdockinfo {
	uint32_t	di_id;		/* dock station id */
	uint32_t	di_serial;	/* serial number */
	uint16_t	di_cap;		/* capabilities */
} __packed;
#define	PNP_DI_ID_UNKNOWN_DOCKING_ID	0xffffffff
#define	PNP_DI_DOCK_WHEN_MASK		0x0006
#define	PNP_DI_DOCK_WHEN_NO_POWER	0x0000
#define	PNP_DI_DOCK_WHEN_SUSPENDED	0x0002
#define	PNP_DI_DOCK_WHEN_RUNNING	0x0004
#define	PNP_DI_DOCK_WHEN_RESERVED	0x0006
#define	PNP_DI_DOCK_STYLE_MASK		0x0001
#define	PNP_DI_DOCK_STYLE_SUPRISE	0x0000	/* just remove */
#define	PNP_DI_DOCK_STYLE_VCR		0x0001	/* controlled */

struct pnplargeres {
	uint8_t		r_type;
	uint16_t	r_len;
	/* variable */
} __packed;

/* resource descriptors */
struct pnpmem16rangeres {
	struct pnplargeres	r_hdr;
	uint8_t		r_flags;
	uint16_t	r_minbase;	/* bits 23-8 */
	uint16_t	r_maxbase;	/* bits 23-8 */
	uint16_t	r_align;	/* 0 == 0x10000 */
	uint16_t	r_len;		/* bits 23-8 */
} __packed;

struct pnpmem32rangeres {
	struct pnplargeres	r_hdr;
	uint8_t		r_flags;
	uint32_t	r_minbase;
	uint32_t	r_maxbase;
	uint32_t	r_align;
	uint32_t	r_len;
} __packed;

struct pnpfixedmem32rangeres {
	struct pnplargeres	r_hdr;
	uint8_t		r_flags;
	uint32_t	r_base;
	uint32_t	r_len;
} __packed;

struct pnpansiidentres {
	struct pnplargeres	r_hdr;
	uint8_t		r_id[1];	/* variable */
} __packed;

struct pnpdevidres {
	uint8_t		r_hdr;
	uint32_t	r_id;
	uint16_t	r_flags;
} __packed;

struct pnpcompatres {
	uint8_t		r_hdr;
	uint32_t	r_id;
} __packed;

struct pnpirqres {
	uint8_t		r_hdr;
	uint16_t	r_mask;
	uint8_t		r_info;		/* may not be present */
} __packed;

struct pnpdmares {
	uint8_t		r_hdr;
	uint8_t		r_mask;
	uint8_t		r_flags;
} __packed;

struct pnpportres {
	uint8_t		r_hdr;
	uint8_t		r_flags;
	uint16_t	r_minbase;
	uint16_t	r_maxbase;
	uint8_t		r_align;
	uint8_t		r_len;
} __packed;

struct pnpfixedportres {
	uint8_t		r_hdr;
	uint16_t	r_base;
	uint8_t		r_len;
} __packed;

struct pnpdepstartres {
	uint8_t		r_hdr;
	uint8_t		r_pri;	/* may not be present */
} __packed;

struct pnpendres {
	uint8_t		r_hdr;
	uint8_t		r_cksum;
} __packed;
