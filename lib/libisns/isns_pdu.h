/*	$NetBSD: isns_pdu.h,v 1.5 2022/04/19 20:32:16 rillig Exp $	*/

/*-
 * Copyright (c) 2004,2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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

/*
 * isns_pdu.h
 */

#ifndef _ISNS_PDU_H_
#define _ISNS_PDU_H_

#include <pthread.h>
#include <string.h>

#include "isns_defs.h"
#include "isns_util.h"

#define ISNSP_VERSION		(0x0001)

/*
 * Max PDU payload length (MUST be <= 65532 and a multiple of 4).
 */
#define ISNS_MAX_PDU_PAYLOAD	65532

/*
 * ISNS buffer pool defines.
 */
#define ISNS_BUF_SIZE		1024
#define ISNS_BUF_COUNT		32

#define ISNS_SMALL_BUF_SIZE	\
    (/* CONSTCOND */MAX(sizeof(struct isns_task_s), sizeof(struct isns_trans_s)))
#define ISNS_SMALL_BUF_COUNT	32

#define ISNS_BUF_POOL		1
#define ISNS_BUF_MALLOC		2
#define ISNS_BUF_STATIC		3

/*
 * ISNS_INIT_BUFFER - initialize buffer (pointer) provided as an isns_buffer_s,
 *		      setting the length and type specified, and initializing
 *		      other members to appropriate values.
 */
#define ISNS_INIT_BUFFER(_bufp, _len, _type)					\
	do if ((_bufp) != NULL) {						\
		((struct isns_buffer_s *)(_bufp))->cur_len = 0;			\
		((struct isns_buffer_s *)(_bufp))->alloc_len = (_len);		\
		((struct isns_buffer_s *)(_bufp))->alloc_len &= ~0x03;		\
		((struct isns_buffer_s *)(_bufp))->buf_type = (_type);		\
		((struct isns_buffer_s *)(_bufp))->next = NULL;			\
	} while (0)


/*
 * ISNS buffer struct - used for trans, pdu, payload allocations in libisns.
 */
struct isns_buffer_s {
	uint32_t cur_len;
	uint32_t alloc_len;
	int buf_type;

	struct isns_buffer_s *next;
};

#define isns_buffer_data(_bufp, _ofs)					\
	(void *)(((uint8_t *)(void *)(_bufp+1))+(_ofs))

void isns_init_buffer_pool(void);
int isns_add_buffer_pool(int, int);
void isns_destroy_buffer_pool(void);
struct isns_buffer_s *isns_new_buffer(int);
void isns_free_buffer(struct isns_buffer_s *);



/*
 * TLV buffer access/manipulation-related macros.
 */
#define ISNS_TLV_HDR_SIZE	(sizeof(uint32_t) * 2)

#define ISNS_PAD4_LEN(n)	(uint32_t)(((n)+3) & ~0x03)
#define ISNS_PAD4_BYTES(n)	((4 - ((n) & 0x03)) & 0x03)

static inline uint32_t ISNS_TLV_GET_TAG(const void *buf) {
	uint32_t tag;
	memcpy(&tag, buf, sizeof(tag));
	return isns_ntohl(tag);
}

static inline void ISNS_TLV_SET_TAG(void *buf, uint32_t tag) {
	tag = isns_htonl(tag);
	memcpy(buf, &tag, sizeof(tag));
}

static inline uint32_t ISNS_TLV_GET_LEN(const void *buf) {
	uint32_t len;
	memcpy(&len, (const uint8_t *)buf + sizeof(len), sizeof(len));
	return isns_ntohl(len);
}

static inline void ISNS_TLV_SET_LEN(void *buf, uint32_t len) {
	len = isns_htonl(len);
	memcpy((uint8_t *)buf + sizeof(len), &len, sizeof(len));
}

static inline void *ISNS_TLV_DATA_PTR(void *buf) {
	return (uint8_t *)buf + ISNS_TLV_HDR_SIZE;
}

/*
 * ISNS transaction and PDU structs.
 */

#define ISNS_TRANSF_COMPLETE			0x000000001
#define ISNS_TRANSF_FREE_WHEN_COMPLETE		0x000000002


struct isns_refresh_s {
	char node[225];
	int interval;

	struct isns_trans_s *trans_p;
};

struct isns_get_tlv_info_s {
	struct isns_pdu_s *pdu_p;
	struct isns_buffer_s *buf_p;
	struct isns_buffer_s *extra_buf_list;
	int buf_ofs;
};

struct isns_trans_s {
	uint16_t id;
	uint16_t func_id;
	uint32_t flags;
	struct isns_config_s *cfg_p;

	struct isns_get_tlv_info_s get_tlv_info;

	struct isns_pdu_s *pdu_req_list;
	struct isns_pdu_s *pdu_rsp_list;

	uint16_t disconnect_cnt;
};

struct isns_pdu_hdr_s {
	uint16_t isnsp_version;
	uint16_t func_id;
	uint16_t payload_len;
	uint16_t flags;
	uint16_t trans_id;
	uint16_t seq_id;
};

struct isns_pdu_s {
	struct isns_config_s *cfg_p;
	struct isns_pdu_hdr_s hdr;
	int byteorder_host;
	struct isns_buffer_s *payload_p;
	struct isns_pdu_s *next;
};


#define isns_get_pdu_request(_trans)					\
	(((_trans) == ISNS_INVALID_TRANS)				\
	    ? NULL : ((struct isns_trans_s *)(_trans))->pdu_req_list)

#define isns_get_pdu_response(_trans)					\
	(((_trans) == ISNS_INVALID_TRANS)				\
	    ? NULL : ((struct isns_trans_s *)(_trans))->pdu_rsp_list)

#define isns_get_next_pdu(_pdu)						\
	(((_pdu) == NULL) ? NULL : ((struct isns_pdu_s *)(_pdu))->next)

#define isns_get_trans_flags(_trans)					\
	isns_set_trans_flags((_trans), 0)


void isns_complete_trans(struct isns_trans_s *);
int isns_abort_trans(struct isns_config_s *, uint16_t);

uint32_t isns_set_trans_flags(struct isns_trans_s *, uint32_t);
void isns_add_pdu_request(struct isns_trans_s *, struct isns_pdu_s *);
void isns_add_pdu_response(struct isns_trans_s *, struct isns_pdu_s *);
struct isns_pdu_s *isns_get_pdu_request_tail(struct isns_trans_s *);
int isns_get_pdu_response_status(struct isns_trans_s *, uint32_t *);

struct isns_pdu_s *isns_new_pdu(struct isns_config_s *, uint16_t, uint16_t,
    uint16_t);
void isns_free_pdu(struct isns_pdu_s *);
int isns_send_pdu(ISNS_TRANS, struct isns_pdu_s *, const struct timespec *);

#ifdef ISNS_DEBUG
#define DUMP_PDU(_pdu)	isns_dump_pdu(_pdu)
void isns_dump_pdu(struct isns_pdu_s *);
#else
#define DUMP_PDU(_pdu)
#endif


#endif /* !_ISNS_PDU_H_ */
