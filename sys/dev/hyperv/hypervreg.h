/*	$NetBSD: hypervreg.h,v 1.1 2019/02/15 08:54:01 nonaka Exp $	*/
/*	$OpenBSD: hypervreg.h,v 1.10 2017/01/05 13:17:22 mikeb Exp $	*/

/*-
 * Copyright (c) 2009-2012,2016 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
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

#ifndef _HYPERVREG_H_
#define _HYPERVREG_H_

#if defined(_KERNEL)

#define VMBUS_CONNID_MESSAGE		1
#define VMBUS_CONNID_EVENT		2
#define VMBUS_SINT_MESSAGE		2
#define VMBUS_SINT_TIMER		4

struct hyperv_guid {
	uint8_t		hv_guid[16];
} __packed;

/*
 * $FreeBSD: head/sys/dev/hyperv/vmbus/hyperv_reg.h 303283 2016-07-25 03:12:40Z sephe $
 */

/*
 * Hyper-V Monitor Notification Facility
 */
struct hyperv_mon_param {
	uint32_t	mp_connid;
	uint16_t	mp_evtflag_ofs;
	uint16_t	mp_rsvd;
} __packed;

/*
 * Hyper-V message types
 */
#define HYPERV_MSGTYPE_NONE		0
#define HYPERV_MSGTYPE_CHANNEL		1
#define HYPERV_MSGTYPE_TIMER_EXPIRED	0x80000010

/*
 * Hypercall status codes
 */
#define HYPERCALL_STATUS_SUCCESS	0x0000

/*
 * Hypercall input values
 */
#define HYPERCALL_POST_MESSAGE		0x005c
#define HYPERCALL_SIGNAL_EVENT		0x005d

/*
 * Hypercall input parameters
 */
#define HYPERCALL_PARAM_ALIGN		8
#if 0
/*
 * XXX
 * <<Hypervisor Top Level Functional Specification 4.0b>> requires
 * input parameters size to be multiple of 8, however, many post
 * message input parameters do _not_ meet this requirement.
 */
#define HYPERCALL_PARAM_SIZE_ALIGN	8
#endif

/*
 * HYPERCALL_POST_MESSAGE
 */
#define HYPERCALL_POSTMSGIN_DSIZE_MAX	240
#define HYPERCALL_POSTMSGIN_SIZE	256

struct hyperv_hypercall_postmsg_in {
	uint32_t	hc_connid;
	uint32_t	hc_rsvd;
	uint32_t	hc_msgtype;	/* VMBUS_MSGTYPE_ */
	uint32_t	hc_dsize;
	uint8_t		hc_data[HYPERCALL_POSTMSGIN_DSIZE_MAX];
} __packed;
__CTASSERT(sizeof(struct hyperv_hypercall_postmsg_in) == HYPERCALL_POSTMSGIN_SIZE);

/*
 * $FreeBSD: head/sys/dev/hyperv/include/vmbus.h 306389 2016-09-28 04:25:25Z sephe $
 */

/*
 * VMBUS version is 32 bit, upper 16 bit for major_number and lower
 * 16 bit for minor_number.
 *
 * 0.13  --  Windows Server 2008
 * 1.1   --  Windows 7
 * 2.4   --  Windows 8
 * 3.0   --  Windows 8.1
 * 4.0   --  Windows 10
 */
#define VMBUS_VERSION_WS2008		((0 << 16) | (13))
#define VMBUS_VERSION_WIN7		((1 << 16) | (1))
#define VMBUS_VERSION_WIN8		((2 << 16) | (4))
#define VMBUS_VERSION_WIN8_1		((3 << 16) | (0))
#define VMBUS_VERSION_WIN10		((4 << 16) | (0))

#define VMBUS_VERSION_MAJOR(ver)	(((uint32_t)(ver)) >> 16)
#define VMBUS_VERSION_MINOR(ver)	(((uint32_t)(ver)) & 0xffff)

/*
 * GPA stuffs.
 */
struct vmbus_gpa_range {
	uint32_t	gpa_len;
	uint32_t	gpa_ofs;
	uint64_t	gpa_page[0];
} __packed;

/* This is actually vmbus_gpa_range.gpa_page[1] */
struct vmbus_gpa {
	uint32_t	gpa_len;
	uint32_t	gpa_ofs;
	uint64_t	gpa_page;
} __packed;

#define VMBUS_CHANPKT_SIZE_SHIFT	3

#define VMBUS_CHANPKT_GETLEN(pktlen)	\
	(((int)(pktlen)) << VMBUS_CHANPKT_SIZE_SHIFT)

struct vmbus_chanpkt_hdr {
	uint16_t	cph_type;	/* VMBUS_CHANPKT_TYPE_ */
	uint16_t	cph_hlen;	/* header len, in 8 bytes */
	uint16_t	cph_tlen;	/* total len, in 8 bytes */
	uint16_t	cph_flags;	/* VMBUS_CHANPKT_FLAG_ */
	uint64_t	cph_tid;
} __packed;

#define VMBUS_CHANPKT_TYPE_INBAND	0x0006
#define VMBUS_CHANPKT_TYPE_RXBUF	0x0007
#define VMBUS_CHANPKT_TYPE_GPA		0x0009
#define VMBUS_CHANPKT_TYPE_COMP		0x000b

#define VMBUS_CHANPKT_FLAG_RC		0x0001	/* report completion */

#define VMBUS_CHANPKT_CONST_DATA(pkt)			\
	((const void *)((const uint8_t *)(pkt) +	\
	    VMBUS_CHANPKT_GETLEN((pkt)->cph_hlen)))

/*
 * $FreeBSD: head/sys/dev/hyperv/vmbus/vmbus_reg.h 305405 2016-09-05 03:21:31Z sephe $
 */

/*
 * Hyper-V SynIC message format.
 */

#define VMBUS_MSG_DSIZE_MAX		240
#define VMBUS_MSG_SIZE			256

struct vmbus_message {
	uint32_t	msg_type;	/* VMBUS_MSGTYPE_ */
	uint8_t		msg_dsize;	/* data size */
	uint8_t		msg_flags;	/* VMBUS_MSGFLAG_ */
	uint16_t	msg_rsvd;
	uint64_t	msg_id;
	uint8_t		msg_data[VMBUS_MSG_DSIZE_MAX];
} __packed;

#define VMBUS_MSGFLAG_PENDING		0x01

/*
 * Hyper-V SynIC event flags
 */

#define VMBUS_EVTFLAGS_SIZE	256
#define VMBUS_EVTFLAGS_MAX	((VMBUS_EVTFLAGS_SIZE / LONG_BIT) * 8)
#define VMBUS_EVTFLAG_LEN	LONG_BIT
#define VMBUS_EVTFLAG_MASK	(LONG_BIT - 1)

struct vmbus_evtflags {
	ulong		evt_flags[VMBUS_EVTFLAGS_MAX];
} __packed;

/*
 * Hyper-V Monitor Notification Facility
 */

struct vmbus_mon_trig {
	uint32_t	mt_pending;
	uint32_t	mt_armed;
} __packed;

#define VMBUS_MONTRIGS_MAX	4
#define VMBUS_MONTRIG_LEN	32

struct vmbus_mnf {
	uint32_t	mnf_state;
	uint32_t	mnf_rsvd1;

	struct vmbus_mon_trig
			mnf_trigs[VMBUS_MONTRIGS_MAX];
	uint8_t		mnf_rsvd2[536];

	uint16_t	mnf_lat[VMBUS_MONTRIGS_MAX][VMBUS_MONTRIG_LEN];
	uint8_t		mnf_rsvd3[256];

	struct hyperv_mon_param
			mnf_param[VMBUS_MONTRIGS_MAX][VMBUS_MONTRIG_LEN];
	uint8_t		mnf_rsvd4[1984];
} __packed;

/*
 * Buffer ring
 */
struct vmbus_bufring {
	/*
	 * If br_windex == br_rindex, this bufring is empty; this
	 * means we can _not_ write data to the bufring, if the
	 * write is going to make br_windex same as br_rindex.
	 */
	volatile uint32_t	br_windex;
	volatile uint32_t	br_rindex;

	/*
	 * Interrupt mask {0,1}
	 *
	 * For TX bufring, host set this to 1, when it is processing
	 * the TX bufring, so that we can safely skip the TX event
	 * notification to host.
	 *
	 * For RX bufring, once this is set to 1 by us, host will not
	 * further dispatch interrupts to us, even if there are data
	 * pending on the RX bufring.  This effectively disables the
	 * interrupt of the channel to which this RX bufring is attached.
	 */
	volatile uint32_t	br_imask;

	uint8_t			br_rsvd[4084];
	uint8_t			br_data[0];
} __packed;

/*
 * Channel
 */

#define VMBUS_CHAN_MAX_COMPAT	256
#define VMBUS_CHAN_MAX		(VMBUS_EVTFLAG_LEN * VMBUS_EVTFLAGS_MAX)

/*
 * Channel packets
 */

#define VMBUS_CHANPKT_SIZE_ALIGN	(1 << VMBUS_CHANPKT_SIZE_SHIFT)

#define VMBUS_CHANPKT_SETLEN(pktlen, len)		\
do {							\
	(pktlen) = (len) >> VMBUS_CHANPKT_SIZE_SHIFT;	\
} while (0)

struct vmbus_chanpkt {
	struct vmbus_chanpkt_hdr cp_hdr;
} __packed;

struct vmbus_chanpkt_sglist {
	struct vmbus_chanpkt_hdr cp_hdr;
	uint32_t	cp_rsvd;
	uint32_t	cp_gpa_cnt;
	struct vmbus_gpa cp_gpa[0];
} __packed;

struct vmbus_chanpkt_prplist {
	struct vmbus_chanpkt_hdr cp_hdr;
	uint32_t	cp_rsvd;
	uint32_t	cp_range_cnt;
	struct vmbus_gpa_range cp_range[0];
} __packed;

/*
 * Channel messages
 * - Embedded in vmbus_message.msg_data, e.g. response and notification.
 * - Embedded in hyperv_hypercall_postmsg_in.hc_data, e.g. request.
 */

#define VMBUS_CHANMSG_CHOFFER			1	/* NOTE */
#define VMBUS_CHANMSG_CHRESCIND			2	/* NOTE */
#define VMBUS_CHANMSG_CHREQUEST			3	/* REQ */
#define VMBUS_CHANMSG_CHOFFER_DONE		4	/* NOTE */
#define VMBUS_CHANMSG_CHOPEN			5	/* REQ */
#define VMBUS_CHANMSG_CHOPEN_RESP		6	/* RESP */
#define VMBUS_CHANMSG_CHCLOSE			7	/* REQ */
#define VMBUS_CHANMSG_GPADL_CONN		8	/* REQ */
#define VMBUS_CHANMSG_GPADL_SUBCONN		9	/* REQ */
#define VMBUS_CHANMSG_GPADL_CONNRESP		10	/* RESP */
#define VMBUS_CHANMSG_GPADL_DISCONN		11	/* REQ */
#define VMBUS_CHANMSG_GPADL_DISCONNRESP		12	/* RESP */
#define VMBUS_CHANMSG_CHFREE			13	/* REQ */
#define VMBUS_CHANMSG_CONNECT			14	/* REQ */
#define VMBUS_CHANMSG_CONNECT_RESP		15	/* RESP */
#define VMBUS_CHANMSG_DISCONNECT		16	/* REQ */
#define VMBUS_CHANMSG_COUNT			17
#define VMBUS_CHANMSG_MAX			22

struct vmbus_chanmsg_hdr {
	uint32_t	chm_type;	/* VMBUS_CHANMSG_* */
	uint32_t	chm_rsvd;
} __packed;

/* VMBUS_CHANMSG_CONNECT */
struct vmbus_chanmsg_connect {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_ver;
	uint32_t	chm_rsvd;
	uint64_t	chm_evtflags;
	uint64_t	chm_mnf1;
	uint64_t	chm_mnf2;
} __packed;

/* VMBUS_CHANMSG_CONNECT_RESP */
struct vmbus_chanmsg_connect_resp {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint8_t		chm_done;
} __packed;

/* VMBUS_CHANMSG_CHREQUEST */
struct vmbus_chanmsg_chrequest {
	struct vmbus_chanmsg_hdr chm_hdr;
} __packed;

/* VMBUS_CHANMSG_DISCONNECT */
struct vmbus_chanmsg_disconnect {
	struct vmbus_chanmsg_hdr chm_hdr;
} __packed;

/* VMBUS_CHANMSG_CHOPEN */
struct vmbus_chanmsg_chopen {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
	uint32_t	chm_openid;
	uint32_t	chm_gpadl;
	uint32_t	chm_vcpuid;
	uint32_t	chm_txbr_pgcnt;
#define VMBUS_CHANMSG_CHOPEN_UDATA_SIZE	120
	uint8_t		chm_udata[VMBUS_CHANMSG_CHOPEN_UDATA_SIZE];
} __packed;

/* VMBUS_CHANMSG_CHOPEN_RESP */
struct vmbus_chanmsg_chopen_resp {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
	uint32_t	chm_openid;
	uint32_t	chm_status;
} __packed;

/* VMBUS_CHANMSG_GPADL_CONN */
struct vmbus_chanmsg_gpadl_conn {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
	uint32_t	chm_gpadl;
	uint16_t	chm_range_len;
	uint16_t	chm_range_cnt;
	struct vmbus_gpa_range chm_range;
} __packed;

#define VMBUS_CHANMSG_GPADL_CONN_PGMAX		26

/* VMBUS_CHANMSG_GPADL_SUBCONN */
struct vmbus_chanmsg_gpadl_subconn {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_msgno;
	uint32_t	chm_gpadl;
	uint64_t	chm_gpa_page[0];
} __packed;

#define VMBUS_CHANMSG_GPADL_SUBCONN_PGMAX	28

/* VMBUS_CHANMSG_GPADL_CONNRESP */
struct vmbus_chanmsg_gpadl_connresp {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
	uint32_t	chm_gpadl;
	uint32_t	chm_status;
} __packed;

/* VMBUS_CHANMSG_CHCLOSE */
struct vmbus_chanmsg_chclose {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
} __packed;

/* VMBUS_CHANMSG_GPADL_DISCONN */
struct vmbus_chanmsg_gpadl_disconn {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
	uint32_t	chm_gpadl;
} __packed;

/* VMBUS_CHANMSG_CHFREE */
struct vmbus_chanmsg_chfree {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
} __packed;

/* VMBUS_CHANMSG_CHRESCIND */
struct vmbus_chanmsg_chrescind {
	struct vmbus_chanmsg_hdr chm_hdr;
	uint32_t	chm_chanid;
} __packed;

/* VMBUS_CHANMSG_CHOFFER */
struct vmbus_chanmsg_choffer {
	struct vmbus_chanmsg_hdr chm_hdr;
	struct hyperv_guid chm_chtype;
	struct hyperv_guid chm_chinst;
	uint64_t	chm_chlat;	/* unit: 100ns */
	uint32_t	chm_chrev;
	uint32_t	chm_svrctx_sz;
	uint16_t	chm_chflags;
	uint16_t	chm_mmio_sz;	/* unit: MB */
	uint8_t		chm_udata[120];
	uint16_t	chm_subidx;
	uint16_t	chm_rsvd;
	uint32_t	chm_chanid;
	uint8_t		chm_montrig;
	uint8_t		chm_flags1;	/* VMBUS_CHOFFER_FLAG1_ */
	uint16_t	chm_flags2;
	uint32_t	chm_connid;
} __packed;

#define VMBUS_CHOFFER_FLAG1_HASMNF	0x01

#endif	/* _KERNEL */

#endif	/* _HYPERVREG_H_ */
