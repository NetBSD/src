/*	$NetBSD: nvmereg.h,v 1.1.2.4 2016/10/05 20:55:41 skrll Exp $	*/
/*	$OpenBSD: nvmereg.h,v 1.10 2016/04/14 11:18:32 dlg Exp $ */

/*
 * Copyright (c) 2014 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef	__NVMEREG_H__
#define	__NVMEREG_H__

#define NVME_CAP	0x0000	/* Controller Capabilities */
#define  NVME_CAP_MPSMAX(_r)	(12 + (((_r) >> 52) & 0xf)) /* shift */
#define  NVME_CAP_MPSMIN(_r)	(12 + (((_r) >> 48) & 0xf)) /* shift */
#define  NVME_CAP_CSS(_r)	(((_r) >> 37) & 0x7f)
#define  NVME_CAP_CSS_NVM	__BIT(0)
#define  NVME_CAP_NSSRS(_r)	ISSET((_r), __BIT(36))
#define  NVME_CAP_DSTRD(_r)	__BIT(2 + (((_r) >> 32) & 0xf)) /* bytes */
#define  NVME_CAP_TO(_r)	(500 * (((_r) >> 24) & 0xff)) /* ms */
#define  NVME_CAP_AMS(_r)	(((_r) >> 17) & 0x3)
#define  NVME_CAP_AMS_WRR	__BIT(0)
#define  NVME_CAP_AMS_VENDOR	__BIT(1)
#define  NVME_CAP_CQR(_r)	ISSET((_r), __BIT(16))
#define  NVME_CAP_MQES(_r)	(((_r) & 0xffff) + 1)
#define NVME_CAP_LO	0x0000
#define NVME_CAP_HI	0x0004
#define NVME_VS		0x0008	/* Version */
#define  NVME_VS_MJR(_r)	(((_r) >> 16) & 0xffff)
#define  NVME_VS_MNR(_r)	((_r) & 0xffff)
#define  NVME_VS_1_0		0x00010000
#define  NVME_VS_1_1		0x00010100
#define  NVME_VS_1_2		0x00010200
#define NVME_INTMS	0x000c	/* Interrupt Mask Set */
#define NVME_INTMC	0x0010	/* Interrupt Mask Clear */
#define NVME_CC		0x0014	/* Controller Configuration */
#define  NVME_CC_IOCQES(_v)	(((_v) & 0xf) << 20)
#define  NVME_CC_IOCQES_MASK	NVME_CC_IOCQES(0xf)
#define  NVME_CC_IOCQES_R(_v)	(((_v) >> 20) & 0xf)
#define  NVME_CC_IOSQES(_v)	(((_v) & 0xf) << 16)
#define  NVME_CC_IOSQES_MASK	NVME_CC_IOSQES(0xf)
#define  NVME_CC_IOSQES_R(_v)	(((_v) >> 16) & 0xf)
#define  NVME_CC_SHN(_v)	(((_v) & 0x3) << 14)
#define  NVME_CC_SHN_MASK	NVME_CC_SHN(0x3)
#define  NVME_CC_SHN_R(_v)	(((_v) >> 15) & 0x3)
#define  NVME_CC_SHN_NONE	0
#define  NVME_CC_SHN_NORMAL	1
#define  NVME_CC_SHN_ABRUPT	2
#define  NVME_CC_AMS(_v)	(((_v) & 0x7) << 11)
#define  NVME_CC_AMS_MASK	NVME_CC_AMS(0x7)
#define  NVME_CC_AMS_R(_v)	(((_v) >> 11) & 0xf)
#define  NVME_CC_AMS_RR		0 /* round-robin */
#define  NVME_CC_AMS_WRR_U	1 /* weighted round-robin w/ urgent */
#define  NVME_CC_AMS_VENDOR	7 /* vendor */
#define  NVME_CC_MPS(_v)	((((_v) - 12) & 0xf) << 7)
#define  NVME_CC_MPS_MASK	(0xf << 7)
#define  NVME_CC_MPS_R(_v)	(12 + (((_v) >> 7) & 0xf))
#define  NVME_CC_CSS(_v)	(((_v) & 0x7) << 4)
#define  NVME_CC_CSS_MASK	NVME_CC_CSS(0x7)
#define  NVME_CC_CSS_R(_v)	(((_v) >> 4) & 0x7)
#define  NVME_CC_CSS_NVM	0
#define  NVME_CC_EN		__BIT(0)
#define NVME_CSTS	0x001c	/* Controller Status */
#define  NVME_CSTS_SHST_MASK	(0x3 << 2)
#define  NVME_CSTS_SHST_NONE	(0x0 << 2) /* normal operation */
#define  NVME_CSTS_SHST_WAIT	(0x1 << 2) /* shutdown processing occurring */
#define  NVME_CSTS_SHST_DONE	(0x2 << 2) /* shutdown processing complete */
#define  NVME_CSTS_CFS		(1 << 1)
#define  NVME_CSTS_RDY		(1 << 0)
#define NVME_NSSR	0x0020	/* NVM Subsystem Reset (Optional) */
#define NVME_AQA	0x0024	/* Admin Queue Attributes */
				/* Admin Completion Queue Size */
#define  NVME_AQA_ACQS(_v)	(((_v) - 1) << 16)
#define  NVME_AQA_ACQS_R(_v)	((_v >> 16) & ((1 << 12) - 1))
				/* Admin Submission Queue Size */
#define  NVME_AQA_ASQS(_v)	(((_v) - 1) << 0)
#define  NVME_AQA_ASQS_R(_v)	(_v & ((1 << 12) - 1))
#define NVME_ASQ	0x0028	/* Admin Submission Queue Base Address */
#define NVME_ACQ	0x0030	/* Admin Completion Queue Base Address */

#define NVME_ADMIN_Q		0
/* Submission Queue Tail Doorbell */
#define NVME_SQTDBL(_q, _s)	(0x1000 + (2 * (_q) + 0) * (_s))
/* Completion Queue Head Doorbell */
#define NVME_CQHDBL(_q, _s)	(0x1000 + (2 * (_q) + 1) * (_s))

struct nvme_sge {
	uint8_t		id;
	uint8_t		_reserved[15];
} __packed __aligned(8);

struct nvme_sge_data {
	uint8_t		id;
	uint8_t		_reserved[3];

	uint32_t	length;

	uint64_t	address;
} __packed __aligned(8);

struct nvme_sge_bit_bucket {
	uint8_t		id;
	uint8_t		_reserved[3];

	uint32_t	length;

	uint64_t	address;
} __packed __aligned(8);

struct nvme_sqe {
	uint8_t		opcode;
	uint8_t		flags;
	uint16_t	cid;

	uint32_t	nsid;

	uint8_t		_reserved[8];

	uint64_t	mptr;

	union {
		uint64_t	prp[2];
		struct nvme_sge	sge;
	} __packed	entry;

	uint32_t	cdw10;
	uint32_t	cdw11;
	uint32_t	cdw12;
	uint32_t	cdw13;
	uint32_t	cdw14;
	uint32_t	cdw15;
} __packed __aligned(8);

struct nvme_sqe_q {
	uint8_t		opcode;
	uint8_t		flags;
	uint16_t	cid;

	uint8_t		_reserved1[20];

	uint64_t	prp1;

	uint8_t		_reserved2[8];

	uint16_t	qid;
	uint16_t	qsize;

	uint8_t		qflags;
#define NVM_SQE_SQ_QPRIO_URG	(0x0 << 1)
#define NVM_SQE_SQ_QPRIO_HI	(0x1 << 1)
#define NVM_SQE_SQ_QPRIO_MED	(0x2 << 1)
#define NVM_SQE_SQ_QPRIO_LOW	(0x3 << 1)
#define NVM_SQE_CQ_IEN		(1 << 1)
#define NVM_SQE_Q_PC		(1 << 0)
	uint8_t		_reserved3;
	uint16_t	cqid; /* XXX interrupt vector for cq */

	uint8_t		_reserved4[16];
} __packed __aligned(8);

struct nvme_sqe_io {
	uint8_t		opcode;
	uint8_t		flags;
	uint16_t	cid;

	uint32_t	nsid;

	uint8_t		_reserved[8];

	uint64_t	mptr;

	union {
		uint64_t	prp[2];
		struct nvme_sge	sge;
	} __packed	entry;

	uint64_t	slba;	/* Starting LBA */

	uint16_t	nlb;	/* Number of Logical Blocks */
	uint16_t	ioflags;
#define NVM_SQE_IO_FUA	__BIT(14)	/* Force Unit Access (bypass cache) */
#define NVM_SQE_IO_LR	__BIT(15)	/* Limited Retry */

	uint8_t		dsm;	/* Dataset Management */
	uint8_t		_reserved2[3];

	uint32_t	eilbrt;	/* Expected Initial Logical Block
				   Reference Tag */

	uint16_t	elbat;	/* Expected Logical Block
				   Application Tag */
	uint16_t	elbatm;	/* Expected Logical Block
				   Application Tag Mask */
} __packed __aligned(8);

struct nvme_cqe {
	uint32_t	cdw0;

	uint32_t	_reserved;

	uint16_t	sqhd; /* SQ Head Pointer */
	uint16_t	sqid; /* SQ Identifier */

	uint16_t	cid; /* Command Identifier */
	uint16_t	flags;
#define NVME_CQE_DNR		__BIT(15)
#define NVME_CQE_M		__BIT(14)
#define NVME_CQE_SCT_MASK	__BITS(8, 10)
#define NVME_CQE_SCT(_f)	((_f) & (0x07 << 8))
#define  NVME_CQE_SCT_GENERIC		(0x00 << 8)
#define  NVME_CQE_SCT_COMMAND		(0x01 << 8)
#define  NVME_CQE_SCT_MEDIAERR		(0x02 << 8)
#define  NVME_CQE_SCT_VENDOR		(0x07 << 8)
#define NVME_CQE_SC_MASK	__BITS(1, 7)
#define NVME_CQE_SC(_f)		((_f) & (0x7f << 1))
/* generic command status codes */
#define  NVME_CQE_SC_SUCCESS		(0x00 << 1)
#define  NVME_CQE_SC_INVALID_OPCODE	(0x01 << 1)
#define  NVME_CQE_SC_INVALID_FIELD	(0x02 << 1)
#define  NVME_CQE_SC_CID_CONFLICT	(0x03 << 1)
#define  NVME_CQE_SC_DATA_XFER_ERR	(0x04 << 1)
#define  NVME_CQE_SC_ABRT_BY_NO_PWR	(0x05 << 1)
#define  NVME_CQE_SC_INTERNAL_DEV_ERR	(0x06 << 1)
#define  NVME_CQE_SC_CMD_ABRT_REQD	(0x07 << 1)
#define  NVME_CQE_SC_CMD_ABDR_SQ_DEL	(0x08 << 1)
#define  NVME_CQE_SC_CMD_ABDR_FUSE_ERR	(0x09 << 1)
#define  NVME_CQE_SC_CMD_ABDR_FUSE_MISS	(0x0a << 1)
#define  NVME_CQE_SC_INVALID_NS		(0x0b << 1)
#define  NVME_CQE_SC_CMD_SEQ_ERR	(0x0c << 1)
#define  NVME_CQE_SC_INVALID_LAST_SGL	(0x0d << 1)
#define  NVME_CQE_SC_INVALID_NUM_SGL	(0x0e << 1)
#define  NVME_CQE_SC_DATA_SGL_LEN	(0x0f << 1)
#define  NVME_CQE_SC_MDATA_SGL_LEN	(0x10 << 1)
#define  NVME_CQE_SC_SGL_TYPE_INVALID	(0x11 << 1)
#define  NVME_CQE_SC_LBA_RANGE		(0x80 << 1)
#define  NVME_CQE_SC_CAP_EXCEEDED	(0x81 << 1)
#define  NVME_CQE_SC_NS_NOT_RDY		(0x82 << 1)
#define  NVME_CQE_SC_RSV_CONFLICT	(0x83 << 1)
/* command specific status codes */
#define  NVME_CQE_SC_CQE_INVALID	(0x00 << 1)
#define  NVME_CQE_SC_INVALID_QID	(0x01 << 1)
#define  NVME_CQE_SC_MAX_Q_SIZE		(0x02 << 1)
#define  NVME_CQE_SC_ABORT_LIMIT	(0x03 << 1)
#define  NVME_CQE_SC_ASYNC_EV_REQ_LIMIT	(0x05 << 1)
#define  NVME_CQE_SC_INVALID_FW_SLOT	(0x06 << 1)
#define  NVME_CQE_SC_INVALID_FW_IMAGE	(0x07 << 1)
#define  NVME_CQE_SC_INVALID_INT_VEC	(0x08 << 1)
#define  NVME_CQE_SC_INVALID_LOG_PAGE	(0x09 << 1)
#define  NVME_CQE_SC_INVALID_FORMAT	(0x0a << 1)
#define  NVME_CQE_SC_FW_REQ_CNV_RESET	(0x0b << 1)
#define  NVME_CQE_SC_FW_REQ_NVM_RESET	(0x10 << 1)
#define  NVME_CQE_SC_FW_REQ_RESET	(0x11 << 1)
#define  NVME_CQE_SC_FW_MAX_TIME_VIO	(0x12 << 1)
#define  NVME_CQE_SC_FW_PROHIBIT	(0x13 << 1)
#define  NVME_CQE_SC_OVERLAP_RANGE	(0x14 << 1)
#define  NVME_CQE_SC_CONFLICT_ATTRS	(0x80 << 1)
#define  NVME_CQE_SC_INVALID_PROT_INFO	(0x81 << 1)
#define  NVME_CQE_SC_ATT_WR_TO_RO_PAGE	(0x82 << 1)
/* media error status codes */
#define  NVME_CQE_SC_WRITE_FAULTS	(0x80 << 1)
#define  NVME_CQE_SC_UNRECV_READ_ERR	(0x81 << 1)
#define  NVME_CQE_SC_GUARD_CHECK_ERR	(0x82 << 1)
#define  NVME_CQE_SC_APPL_TAG_CHECK_ERR	(0x83 << 1)
#define  NVME_CQE_SC_REF_TAG_CHECK_ERR	(0x84 << 1)
#define  NVME_CQE_SC_CMP_FAIL		(0x85 << 1)
#define  NVME_CQE_SC_ACCESS_DENIED	(0x86 << 1)
#define NVME_CQE_PHASE		__BIT(0)
} __packed __aligned(8);

#define NVM_ADMIN_DEL_IOSQ	0x00 /* Delete I/O Submission Queue */
#define NVM_ADMIN_ADD_IOSQ	0x01 /* Create I/O Submission Queue */
#define NVM_ADMIN_GET_LOG_PG	0x02 /* Get Log Page */
#define NVM_ADMIN_DEL_IOCQ	0x04 /* Delete I/O Completion Queue */
#define NVM_ADMIN_ADD_IOCQ	0x05 /* Create I/O Completion Queue */
#define NVM_ADMIN_IDENTIFY	0x06 /* Identify */
#define NVM_ADMIN_ABORT		0x08 /* Abort */
#define NVM_ADMIN_SET_FEATURES	0x09 /* Set Features */
#define NVM_ADMIN_GET_FEATURES	0x0a /* Get Features */
#define NVM_ADMIN_ASYNC_EV_REQ	0x0c /* Asynchronous Event Request */
#define NVM_ADMIN_FW_COMMIT	0x10 /* Firmware Commit */
#define NVM_ADMIN_FW_DOWNLOAD	0x11 /* Firmware Image Download */

#define NVM_CMD_FLUSH		0x00 /* Flush */
#define NVM_CMD_WRITE		0x01 /* Write */
#define NVM_CMD_READ		0x02 /* Read */
#define NVM_CMD_WR_UNCOR	0x04 /* Write Uncorrectable */
#define NVM_CMD_COMPARE		0x05 /* Compare */
#define NVM_CMD_DSM		0x09 /* Dataset Management */

/* Power State Descriptor Data */
struct nvm_identify_psd {
	uint16_t	mp;		/* Max Power */
	uint8_t		_reserved1;
	uint8_t		flags;
#define	NVME_PSD_NOPS		__BIT(1)
#define	NVME_PSD_MPS		__BIT(0)

	uint32_t	enlat;		/* Entry Latency */

	uint32_t	exlat;		/* Exit Latency */

	uint8_t		rrt;		/* Relative Read Throughput */
#define	NVME_PSD_RRT_MASK	__BITS(0, 4)
	uint8_t		rrl;		/* Relative Read Latency */
#define	NVME_PSD_RRL_MASK	__BITS(0, 4)
	uint8_t		rwt;		/* Relative Write Throughput */
#define	NVME_PSD_RWT_MASK	__BITS(0, 4)
	uint8_t		rwl;		/* Relative Write Latency */
#define	NVME_PSD_RWL_MASK	__BITS(0, 4)

	uint16_t	idlp;		/* Idle Power */
	uint8_t		ips;		/* Idle Power Scale */
#define	NVME_PSD_IPS_MASK	__BITS(0, 1)
	uint8_t		_reserved2;
	uint16_t	actp;		/* Active Power */
	uint16_t	ap;		/* Active Power Workload/Scale */
#define	NVME_PSD_APW_MASK	__BITS(0, 2)
#define	NVME_PSD_APS_MASK	__BITS(6, 7)

	uint8_t		_reserved[8];
} __packed __aligned(8);

struct nvm_identify_controller {
	/* Controller Capabilities and Features */

	uint16_t	vid;		/* PCI Vendor ID */
	uint16_t	ssvid;		/* PCI Subsystem Vendor ID */

	uint8_t		sn[20];		/* Serial Number */
	uint8_t		mn[40];		/* Model Number */
	uint8_t		fr[8];		/* Firmware Revision */

	uint8_t		rab;		/* Recommended Arbitration Burst */
	uint8_t		ieee[3];	/* IEEE OUI Identifier */

	uint8_t		cmic;		/* Controller Multi-Path I/O and
					   Namespace Sharing Capabilities */
	uint8_t		mdts;		/* Maximum Data Transfer Size */
	uint16_t	cntlid;		/* Controller ID */

	uint8_t		_reserved1[176];

	/* Admin Command Set Attributes & Optional Controller Capabilities */

	uint16_t	oacs;		/* Optional Admin Command Support */
#define	NVME_ID_CTRLR_OACS_NS		__BIT(3)
#define	NVME_ID_CTRLR_OACS_FW		__BIT(2)
#define	NVME_ID_CTRLR_OACS_FORMAT	__BIT(1)
#define	NVME_ID_CTRLR_OACS_SECURITY	__BIT(0)
	uint8_t		acl;		/* Abort Command Limit */
	uint8_t		aerl;		/* Asynchronous Event Request Limit */

	uint8_t		frmw;		/* Firmware Updates */
#define	NVME_ID_CTRLR_FRMW_NOREQ_RESET	__BIT(4)
#define	NVME_ID_CTRLR_FRMW_NSLOT	__BITS(1, 3)
#define	NVME_ID_CTRLR_FRMW_SLOT1_RO	__BIT(0)
	uint8_t		lpa;		/* Log Page Attributes */
#define	NVME_ID_CTRLR_LPA_CMD_EFFECT	__BIT(1)
#define	NVME_ID_CTRLR_LPA_NS_SMART	__BIT(0)
	uint8_t		elpe;		/* Error Log Page Entries */
	uint8_t		npss;		/* Number of Power States Support */

	uint8_t		avscc;		/* Admin Vendor Specific Command
					   Configuration */
	uint8_t		apsta;		/* Autonomous Power State Transition
					   Attributes */

	uint8_t		_reserved2[246];

	/* NVM Command Set Attributes */

	uint8_t		sqes;		/* Submission Queue Entry Size */
#define	NVME_ID_CTRLR_SQES_MAX		__BITS(4, 7)
#define	NVME_ID_CTRLR_SQES_MIN		__BITS(0, 3)
	uint8_t		cqes;		/* Completion Queue Entry Size */
#define	NVME_ID_CTRLR_CQES_MAX		__BITS(4, 7)
#define	NVME_ID_CTRLR_CQES_MIN		__BITS(0, 3)
	uint8_t		_reserved3[2];

	uint32_t	nn;		/* Number of Namespaces */

	uint16_t	oncs;		/* Optional NVM Command Support */
#define	NVME_ID_CTRLR_ONCS_RESERVATION	__BIT(5)
#define	NVME_ID_CTRLR_ONCS_SET_FEATURES	__BIT(4)
#define	NVME_ID_CTRLR_ONCS_WRITE_ZERO	__BIT(3)
#define	NVME_ID_CTRLR_ONCS_DSM		__BIT(2)
#define	NVME_ID_CTRLR_ONCS_WRITE_UNC	__BIT(1)
#define	NVME_ID_CTRLR_ONCS_COMPARE	__BIT(0)
	uint16_t	fuses;		/* Fused Operation Support */

	uint8_t		fna;		/* Format NVM Attributes */
	uint8_t		vwc;		/* Volatile Write Cache */
#define	NVME_ID_CTRLR_VWC_PRESENT	__BIT(0)
	uint16_t	awun;		/* Atomic Write Unit Normal */

	uint16_t	awupf;		/* Atomic Write Unit Power Fail */
	uint8_t		nvscc;		/* NVM Vendor Specific Command */
	uint8_t		_reserved4[1];

	uint16_t	acwu;		/* Atomic Compare & Write Unit */
	uint8_t		_reserved5[2];

	uint32_t	sgls;		/* SGL Support */

	uint8_t		_reserved6[164];

	/* I/O Command Set Attributes */

	uint8_t		_reserved7[1344];

	/* Power State Descriptors */

	struct nvm_identify_psd psd[32]; /* Power State Descriptors */

	/* Vendor Specific */

	uint8_t		_reserved8[1024];
} __packed __aligned(8);

struct nvm_namespace_format {
	uint16_t	ms;		/* Metadata Size */
	uint8_t		lbads;		/* LBA Data Size */
	uint8_t		rp;		/* Relative Performance */
} __packed __aligned(4);

struct nvm_identify_namespace {
	uint64_t	nsze;		/* Namespace Size */

	uint64_t	ncap;		/* Namespace Capacity */

	uint64_t	nuse;		/* Namespace Utilization */

	uint8_t		nsfeat;		/* Namespace Features */
#define	NVME_ID_NS_NSFEAT_LOGICAL_BLK_ERR	__BIT(2)
#define	NVME_ID_NS_NSFEAT_NS			__BIT(1)
#define	NVME_ID_NS_NSFEAT_THIN_PROV		__BIT(0)
	uint8_t		nlbaf;		/* Number of LBA Formats */
	uint8_t		flbas;		/* Formatted LBA Size */
#define NVME_ID_NS_FLBAS(_f)			((_f) & 0x0f)
#define NVME_ID_NS_FLBAS_MD			0x10
	uint8_t		mc;		/* Metadata Capabilities */
	uint8_t		dpc;		/* End-to-end Data Protection
					   Capabilities */
	uint8_t		dps;		/* End-to-end Data Protection Type Settings */

	uint8_t		_reserved1[98];

	struct nvm_namespace_format
			lbaf[16];	/* LBA Format Support */

	uint8_t		_reserved2[192];

	uint8_t		vs[3712];
} __packed __aligned(8);

#endif	/* __NVMEREG_H__ */
