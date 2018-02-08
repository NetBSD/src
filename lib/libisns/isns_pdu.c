/*	$NetBSD: isns_pdu.c,v 1.4 2018/02/08 09:05:17 dholland Exp $	*/

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
 * isns_pdu.c
 */


#include <sys/cdefs.h>
__RCSID("$NetBSD: isns_pdu.c,v 1.4 2018/02/08 09:05:17 dholland Exp $");


#include <sys/types.h>
#include <sys/param.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "isns.h"
#include "isns_config.h"


/*
 * Local function prototypes.
 */
static struct isns_buffer_list_s *isns_lookup_buffer_list(int);

static struct isns_pdu_s *isns_init_pdu(struct isns_buffer_s *,
    struct isns_config_s *, uint16_t, uint16_t, uint16_t);
static int isns_add_pdu_payload_data(struct isns_trans_s *, const void *, int);
static void isns_get_tlv_info_advance(struct isns_get_tlv_info_s *);
static int isns_get_tlv_uint32(struct isns_get_tlv_info_s *, uint32_t *);
static int isns_get_tlv_data(struct isns_get_tlv_info_s *, int, void **);

static void isns_add_pdu_list(struct isns_pdu_s **, struct isns_pdu_s *);
static struct isns_buffer_s *isns_get_pdu_head_buffer(struct isns_pdu_s *);
#if 0
static struct isns_buffer_s *isns_get_pdu_tail_buffer(struct isns_pdu_s *);
#endif
static struct isns_buffer_s *isns_get_pdu_active_buffer(struct isns_pdu_s *);

static uint32_t isns_get_next_trans_id(void);

/*
 * Buffer pool structures and global (static) var.
 */
struct isns_buffer_list_s {
	int buf_size;
	int alloc_count;
	struct isns_buffer_s *head;
	struct isns_buffer_list_s *next;
};

struct isns_buffer_pool_s {
	int active;
	struct isns_buffer_list_s *list_p;
	pthread_mutex_t mutex;
};

static struct isns_buffer_pool_s G_buffer_pool;


/*
 * isns_init_buffer_pool - initialize buffer pool for use
 */
void
isns_init_buffer_pool(void)
{
	pthread_mutexattr_t mutexattr;

	DBG("isns_init_buffer_pool: entered\n");

	assert(!G_buffer_pool.active);

	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, ISNS_MUTEX_TYPE_NORMAL);
	pthread_mutex_init(&G_buffer_pool.mutex, &mutexattr);

	G_buffer_pool.active = 1;
}


/*
 * isns_lookup_buffer_list - locates the pool buffer list for the buf_size
 *			     specified.
 *
 * Returns: pointer to list in pool containing buf_size buffers
 *	    NULL if no list for size indicated exists
 */
static struct isns_buffer_list_s *
isns_lookup_buffer_list(int buf_size)
{
	struct isns_buffer_list_s *list_p;

	/*
	 * WARNING: G_buffer_pool.mutex MUST already be locked.
	 */

	list_p = G_buffer_pool.list_p;
	while (list_p != NULL) {
		if (list_p->buf_size == buf_size)
			break;
		list_p = list_p->next;
	}

	return list_p;
}


/*
 * isns_add_buffer_pool - allocates buffers of in pool
 *
 * If a list containing buf_size buffers already exists in pool, additional
 * buffers are added (allocated) to the list.
 */
int
isns_add_buffer_pool(int buf_size, int count)
{
	struct isns_buffer_list_s *list_p, *p, *p_next;
	struct isns_buffer_s *buf_p;
	int n;

	DBG("isns_add_buffer_pool: buf_size=%d, count=%d\n", buf_size, count);

	assert(G_buffer_pool.active);

	/* Make our buffer lengths always a multiple of 4. */
	buf_size = (buf_size + 0x03) & ~0x03;

	/*
	 * Lookup existing list for size specified.  If no exists, allocate
	 * a new list and initialize.
	 */
	pthread_mutex_lock(&G_buffer_pool.mutex);
	list_p = isns_lookup_buffer_list(buf_size);
	if (list_p == NULL) {
		pthread_mutex_unlock(&G_buffer_pool.mutex);
		list_p = (struct isns_buffer_list_s *)
		    isns_malloc(sizeof(struct isns_buffer_list_s));
		if (list_p == NULL) {
			DBG("isns_add_buffer_pool: error on isns_malloc()\n");
			return ENOMEM;
		}
		list_p->buf_size = buf_size;
		list_p->alloc_count = 0;
		list_p->head = NULL;
	}

	/* If this is a new list, insert into pool in buf_size order. */
	if (list_p->alloc_count == 0) {
		pthread_mutex_lock(&G_buffer_pool.mutex);
		if (G_buffer_pool.list_p == NULL) {
			G_buffer_pool.list_p = list_p;
			list_p->next = NULL;
		} else if (G_buffer_pool.list_p->buf_size > buf_size) {
			list_p->next = G_buffer_pool.list_p;
			G_buffer_pool.list_p = list_p;
		} else {
			p = G_buffer_pool.list_p;
			while (p->next != NULL) {
				p_next = p->next;
				if (p_next->buf_size > buf_size) {
					p->next = list_p;
					list_p->next = p_next;
					break;
				}
				p = p->next;
			}
			if (p->next == NULL) {
				p->next = list_p;
				list_p->next = NULL;
			}
		}
	}

	/* Allocate (possibly additional) buffers for list. */
	for (n = 0; n < count; n++) {
		buf_p = (struct isns_buffer_s *)
		    isns_malloc(buf_size + sizeof(struct isns_buffer_s));
		if (buf_p == NULL)
			break;
		buf_p->next = list_p->head;
		list_p->head = buf_p;
	}
	list_p->alloc_count += n;
	pthread_mutex_unlock(&G_buffer_pool.mutex);

	DBG("isns_init_buffer_pool: %d %d-byte buffers allocated\n",
	    n, buf_size);

	return (n > 0 ? 0 : ENOMEM);
}


/*
 * isns_destroy_buffer_pool - destroys previously allocated buffer pool
 */
void
isns_destroy_buffer_pool(void)
{
	struct isns_buffer_list_s *list_p;
	struct isns_buffer_s *buf_p;
#ifdef ISNS_DEBUG
	char dbg_buffer[1024] = { 0 };
#endif

	DBG("isns_destroy_buffer_pool: entered\n");

	assert(G_buffer_pool.active);

	pthread_mutex_lock(&G_buffer_pool.mutex);
	while (G_buffer_pool.list_p != NULL) {
		list_p = G_buffer_pool.list_p;
		while (list_p->head != NULL) {
			buf_p = list_p->head;
			list_p->head = buf_p->next;
			list_p->alloc_count--;
			isns_free(buf_p);
		}
#ifdef ISNS_DEBUG
		if (list_p->alloc_count > 0) {
			snprintf(&dbg_buffer[(int) strlen(dbg_buffer)],
			    (sizeof(dbg_buffer) - strlen(dbg_buffer)),
			    "isns_destroy_buffer_pool: "
			    "%d %d-byte buffer(s) not freed\n",
			    list_p->alloc_count, list_p->buf_size);
		}
#endif
		G_buffer_pool.list_p = list_p->next;
		isns_free(list_p);
	}
	G_buffer_pool.active = 0;

	pthread_mutex_unlock(&G_buffer_pool.mutex);
	pthread_mutex_destroy(&G_buffer_pool.mutex);

	DBG(dbg_buffer);
}


/*
 * isns_new_buffer - allocates a new ISNS buffer
 *
 * Typically, the buffer is returned from the pool, but if no free buffers
 * are available in the pool, or a buf size larger than the largest pool buffer
 * size is requested, a normal malloc is used to allocate the buffer.  The
 * buffer type is recorded so that a subsequent isns_free_buffer will correctly
 * free the buffer or return it to the pool.
 */
struct isns_buffer_s *
isns_new_buffer(int buf_size)
{
	struct isns_buffer_list_s *list_p;
	struct isns_buffer_s *buf_p;
	int buf_type;

	if (buf_size == 0)
		buf_size = ISNS_BUF_SIZE;
	buf_p = NULL;

	pthread_mutex_lock(&G_buffer_pool.mutex);
	list_p = G_buffer_pool.list_p;
	while (list_p != NULL) {
		if ((list_p->head != NULL)
		    && (list_p->buf_size >= buf_size)) {
			buf_p = list_p->head;
			list_p->head = buf_p->next;
			buf_size = list_p->buf_size;
			buf_type = ISNS_BUF_POOL;
			break;
		}
		list_p = list_p->next;
	}
	pthread_mutex_unlock(&G_buffer_pool.mutex);

	if (buf_p == NULL) {
		buf_p = (struct isns_buffer_s *)isns_malloc(
		    buf_size + sizeof(struct isns_buffer_s));
		buf_type = ISNS_BUF_MALLOC;
	}

	if (buf_p != NULL)
		ISNS_INIT_BUFFER(buf_p, buf_size, buf_type);

	DBG("isns_new_buffer: %p (buf_size=%d, type=%d)\n", buf_p, buf_size,
	    buf_type);

	return buf_p;
}


/*
 * isns_free_buffer - free a ISNS buffer
 */
void
isns_free_buffer(struct isns_buffer_s *buf_p)
{
	struct isns_buffer_list_s *list_p;

	DBG("isns_free_buffer: %p (type=%d, alloc_len=%d)\n",
	    buf_p, (buf_p == NULL ? 0 : buf_p->buf_type),
	    (buf_p == NULL ? 0 : buf_p->alloc_len));

	if (buf_p != NULL) {
		switch (buf_p->buf_type) {
		case ISNS_BUF_POOL:
			/* Return buffer to proper pool list. */
			pthread_mutex_lock(&G_buffer_pool.mutex);
			list_p = isns_lookup_buffer_list((int)buf_p->alloc_len);
			if (list_p != NULL) {
				buf_p->next = list_p->head;
				list_p->head = buf_p;
			}
			pthread_mutex_unlock(&G_buffer_pool.mutex);
			break;
		case ISNS_BUF_MALLOC:
			/* Malloc allocated buf, so free normally. */
			isns_free(buf_p);
			break;
		case ISNS_BUF_STATIC:
			/* Static buf with no allocation, so do nothing here. */
			break;
		}
	}
}


/*
 * isns_new_trans - create a new ISNS transaction
 */
ISNS_TRANS
isns_new_trans(ISNS_HANDLE isns_handle, uint16_t func_id, uint16_t pdu_flags)
{
	struct isns_trans_s *trans_p;
	struct isns_pdu_s *pdu_p;
	struct isns_buffer_s *buf_p;

	if (isns_handle == ISNS_INVALID_HANDLE) {
		DBG("isns_new_trans: error - handle=%p\n", isns_handle);
		return ISNS_INVALID_TRANS;
	}

	buf_p = isns_new_buffer((int)sizeof(struct isns_trans_s));
	if (buf_p == NULL) {
		DBG("isns_new_trans: error on isns_new_buffer()\n");
		return ISNS_INVALID_TRANS;
	}

	trans_p = (struct isns_trans_s *)isns_buffer_data(buf_p, 0);
	trans_p->id = isns_get_next_trans_id();
	trans_p->func_id = func_id;
	trans_p->flags = 0;
	trans_p->cfg_p = (struct isns_config_s *)isns_handle;
	trans_p->pdu_req_list = NULL;
	trans_p->pdu_rsp_list = NULL;
	trans_p->disconnect_cnt = 0;

	trans_p->get_tlv_info.pdu_p = NULL;
	trans_p->get_tlv_info.buf_p = NULL;
	trans_p->get_tlv_info.extra_buf_list = NULL;
	trans_p->get_tlv_info.buf_ofs = 0;

	buf_p->cur_len = sizeof(struct isns_trans_s);

	/*
	 * Mask off all but the AUTH and possibly REPLACE_REG pdu flags.  Then,
	 * set the appropriate server/client sender flag.  The first/last PDU
	 * flags will be set when the PDU is sent.
	 */
	if (func_id == isnsp_DevAttrReg)
		pdu_flags &= (ISNS_FLAG_AUTH | ISNS_FLAG_REPLACE_REG);
	else
		pdu_flags &= ISNS_FLAG_AUTH;

	if (trans_p->cfg_p->is_server)
		pdu_flags |= ISNS_FLAG_SND_SERVER;
	else
		pdu_flags |= ISNS_FLAG_SND_CLIENT;

	pdu_p = isns_new_pdu(trans_p->cfg_p, trans_p->id, func_id, pdu_flags);
	if (pdu_p == NULL) {
		DBG("isns_new_trans: error on isns_new_pdu()\n");
		isns_free_buffer(buf_p);
		return ISNS_INVALID_TRANS;
	}

	isns_add_pdu_request((ISNS_TRANS)trans_p, pdu_p);

	DBG("isns_new_trans: %p\n", trans_p);

	return (ISNS_TRANS)trans_p;
}


/*
 * isns_free_trans - free ISNS transaction created with isns_new_trans
 */
void
isns_free_trans(ISNS_TRANS trans)
{
	struct isns_trans_s *trans_p;
	struct isns_pdu_s *pdu_p;
	struct isns_buffer_s *buf_p, *free_buf_p;
	uint32_t trans_flags;

	DBG("isns_free_trans: %p\n", trans);

	if (trans != ISNS_INVALID_TRANS) {
		trans_p = (struct isns_trans_s *)trans;

		trans_flags = isns_set_trans_flags(trans_p,
		    ISNS_TRANSF_FREE_WHEN_COMPLETE);

		if ((trans_flags & ISNS_TRANSF_COMPLETE) == 0) {
			DBG("isns_free_trans: deferred - trans not complete\n");
			return;
		}

		DBG("isns_free_trans: pdu_req_list=%p\n",
		    trans_p->pdu_req_list);
		while ((pdu_p = trans_p->pdu_req_list) != NULL) {
			trans_p->pdu_req_list = pdu_p->next;
			isns_free_pdu(pdu_p);
		}
		DBG("isns_free_trans: pdu_rsp_list=%p\n",
		    trans_p->pdu_rsp_list);
		while ((pdu_p = trans_p->pdu_rsp_list) != NULL) {
			trans_p->pdu_rsp_list = pdu_p->next;
			isns_free_pdu(pdu_p);
		}
		DBG("isns_free_trans: extra_buf_list=%p\n",
		    trans_p->get_tlv_info.extra_buf_list);
		buf_p = trans_p->get_tlv_info.extra_buf_list;
		while (buf_p != NULL) {
			free_buf_p = buf_p;
			buf_p = buf_p->next;
			isns_free_buffer(free_buf_p);
		}

		DBG("isns_free_trans: freeing base trans buffer\n");
		buf_p = ((struct isns_buffer_s *)(void *)(trans_p))-1;
		isns_free_buffer(buf_p);
	}
}


/*
 * isns_send_trans - send ISNS transaction PDU(s) and optionally wait
 *
 * If a successful wait occurs (i.e., the transaction completes without
 * a timeout), then the response PDU status is place in *status_p.  For
 * all other cases, the data returned in *status_p is undefined.
 *
 */
int
isns_send_trans(ISNS_TRANS trans, const struct timespec *timeout_p,
    uint32_t *status_p)
{
	struct isns_trans_s *trans_p;
	struct isns_pdu_s *pdu_p;
	int rval;

	trans_p = (struct isns_trans_s *)trans;

	DBG("isns_send_trans: trans_p=%p, timeout_p=%p\n", trans_p, timeout_p);

	if (status_p != NULL)
		*status_p = 0;

	if (!isns_is_socket_init_done(trans_p->cfg_p)) {
		DBG("isns_send_trans: socket not initialized\n");
		isns_complete_trans(trans_p);
		return EINVAL;
	}

	if ((pdu_p = isns_get_pdu_request(trans)) == NULL) {
		DBG("isns_send_trans: no request PDU\n");
		return EINVAL;
	}

	/* Set the FIRST_PDU flag in the first PDU. */
	pdu_p->hdr.flags |= ISNS_FLAG_FIRST_PDU;

	/* Set our PDU sequence numbers for the PDU chain. */
	while (pdu_p->next != NULL) {
		pdu_p->next->hdr.seq_id = pdu_p->hdr.seq_id + 1;
		pdu_p = pdu_p->next;
	}

	/* Set the LAST_PDU flag in the last PDU. */
	pdu_p->hdr.flags |= ISNS_FLAG_LAST_PDU;

	rval = isns_send_pdu(trans, isns_get_pdu_request(trans), timeout_p);
	if ((rval == 0) && (status_p != NULL))
		isns_get_pdu_response_status(trans, status_p);

	return rval;
}



void
isns_complete_trans(struct isns_trans_s *trans_p)
{
	uint32_t flags;

	DBG("isns_complete_trans: trans_p=%p\n", trans_p);

	flags = isns_set_trans_flags(trans_p, ISNS_TRANSF_COMPLETE);

	if ((flags & ISNS_TRANSF_FREE_WHEN_COMPLETE) != 0)
		isns_free_trans(trans_p);
}


int
isns_abort_trans(struct isns_config_s *cfg_p, uint16_t trans_id)
{
	struct isns_task_s *task_p;

	/* First, look at current task. */
	if (((task_p = cfg_p->curtask_p) != NULL)
	    && (task_p->task_type == ISNS_TASK_SEND_PDU)
	    && (task_p->var.send_pdu.trans_p->id == trans_id)) {
		isns_complete_trans(task_p->var.send_pdu.trans_p);
		isns_end_task(task_p);
		return 0;
	}

	/* If not current task, look in task queue. */
	task_p = isns_taskq_remove_trans(cfg_p, trans_id);
	if (task_p) {
		isns_complete_trans(task_p->var.send_pdu.trans_p);
		isns_end_task(task_p);
		return 0;
	}

	return EINVAL;
}

/*
 * isns_add_string - add a TLV which is a C string
 *
 * Wrapper around isns_add_tlv()
 */
int
isns_add_string(ISNS_TRANS trans, uint32_t tag, const char *s)
{
	/* Add string, including required NULL. */
	return isns_add_tlv(trans, tag, (uint32_t)strlen(s) + 1, s);
}


/*
 * isns_add_tlv - adds a TLV to an existing transaction
 */
int
isns_add_tlv(ISNS_TRANS trans, uint32_t tag, uint32_t data_len,
    const void *data_p)
{
	struct isns_trans_s *trans_p;
	uint8_t tlv_buf[ISNS_TLV_HDR_SIZE];
	int rval;

	DBG("isns_add_tlv: trans=%p, tag=%d, data_len=%d, data_p=%p\n",
	    trans, tag, data_len, data_p);

	if (trans == ISNS_INVALID_TRANS) {
		DBG("isns_add_tlv: error - trans=%p\n", trans);
		return EINVAL;
	}

	if ((data_len > 0) && (data_p == NULL)) {
		DBG("isns_add_tlv: error data_len=%d, data_p=%p\n",
		    data_len, data_p);
		return EINVAL;
	}

	/* Set tag, length in header buffer and add to PDU payload data. */
	trans_p = (struct isns_trans_s *)trans;
	ISNS_TLV_SET_TAG(tlv_buf, tag);
	ISNS_TLV_SET_LEN(tlv_buf, ISNS_PAD4_LEN(data_len));
	rval = isns_add_pdu_payload_data(trans_p, tlv_buf, ISNS_TLV_HDR_SIZE);

	/* If header added okay, add value portion to PDU payload data. */
	if ((rval == 0) && (data_len > 0))
		rval = isns_add_pdu_payload_data(trans_p, data_p, data_len);

	return rval;
}


/*
 * isns_get_tlv - get TLV value from response PDU for transaction
 *
 * returns:
 *   0 - success
 *   ENOENT - no (more) TLVs in this transaction
 *   EINVAL - invalid arg
 *   EPERM  - operation not permitted - transaction not complete
 *   ENOMEM - could not allocate storage for spanning TLV data
 */
int
isns_get_tlv(ISNS_TRANS trans, int which_tlv, uint32_t *tag_p,
    uint32_t *data_len_p, void **data_pp)
{
	struct isns_trans_s *trans_p;
	struct isns_get_tlv_info_s *info_p;
	struct isns_pdu_s *pdu_p;
	int rval;

	if (trans == ISNS_INVALID_TRANS) {
		DBG("isns_get_tlv: error - trans=%p\n", trans);
		return EINVAL;
	}

	trans_p = (struct isns_trans_s *)trans;
	if ((isns_get_trans_flags(trans_p) & ISNS_TRANSF_COMPLETE) == 0) {
		DBG("isns_get_tlv: error - trans not complete\n");
		return EPERM;
	}

	/* Get response PDU for this transaction. */
	pdu_p = isns_get_pdu_response(trans_p);
	if (pdu_p == NULL) {
		DBG("isns_get_tlv: error - no response PDU in transaction\n");
		return EINVAL;
	}

	if (pdu_p->payload_p->cur_len == 0) {
		DBG("isns_get_tlv: error - zero length PDU payload\n");
		return EINVAL;
	}

	/* If get_tlv_info unset, treat ISNS_TLV_NEXT as ISNS_TLV_FIRST. */
	info_p = &trans_p->get_tlv_info;
	if ((which_tlv == ISNS_TLV_NEXT) && (info_p->pdu_p == NULL))
		which_tlv = ISNS_TLV_FIRST;

	/*!!! make sure PDU uses TLVs here */

	switch (which_tlv) {
	case ISNS_TLV_NEXT:
		/* For next TLV, nothing to do here - use get_tlv_info as-is. */
		break;

	case ISNS_TLV_FIRST:
		/* For first TLV, reset get_tlv_info. */
		info_p->pdu_p = pdu_p;
		info_p->buf_p = isns_get_pdu_head_buffer(pdu_p);
		info_p->buf_ofs = 4;
		break;

	default:
		DBG("isns_get_tlv: invalid arg (which_tlv=%d)\n", which_tlv);
		return EINVAL;
	}

	/*
	 * Get the type, length, and data (value) for the TLV.  The get calls
	 * below will advance the pointers in get_tlv_info_s *info_p.  Note that
	 * if the get of the TAG type fails, ENOENT is returned indicating that
	 * no more TLVs exist for this request PDU.
	 */
	if ((rval = isns_get_tlv_uint32(info_p, tag_p)) != 0) {
		DBG("isns_get_tlv: error on isns_get_tlv_uint32() tag\n");
		return ENOENT;
	}

	if ((rval = isns_get_tlv_uint32(info_p, (uint32_t *)data_len_p)) != 0) {
		DBG("isns_get_tlv: error on isns_get_tlv_uint32() data len\n");
		return rval;
	}

	rval = isns_get_tlv_data(info_p, *data_len_p, data_pp);
	if (rval != 0) {
		DBG("isns_get_tlv: error on isns_get_tlv_data()\n");
		return rval;
	}

	return 0;
}


/*
 * isns_set_trans_flags - sets flag bit(s) in transaction flags member
 */
uint32_t
isns_set_trans_flags(struct isns_trans_s *trans_p, uint32_t flags)
{
	pthread_mutex_lock(&trans_p->cfg_p->trans_mutex);
	trans_p->flags |= flags;
	flags = trans_p->flags;
	pthread_mutex_unlock(&trans_p->cfg_p->trans_mutex);

	return flags;
}


/*
 * isns_add_pdu_request - adds PDU to transaction request PDU list
 */
void
isns_add_pdu_request(struct isns_trans_s *trans_p, struct isns_pdu_s *pdu_p)
{
	DBG("isns_add_pdu_request: trans_p=%p, pdu_p=%p\n", trans_p, pdu_p);

	isns_add_pdu_list(&trans_p->pdu_req_list, pdu_p);
}


/*
 * isns_add_pdu_response - adds PDU to transaction response PDU list
 */
void
isns_add_pdu_response(struct isns_trans_s *trans_p, struct isns_pdu_s *pdu_p)
{
	DBG("isns_add_pdu_response: trans_p=%p, pdu_p=%p\n", trans_p, pdu_p);

	isns_add_pdu_list(&trans_p->pdu_rsp_list, pdu_p);
}


/*
 * isns_get_pdu_request_tail - returns last PDU in request PDU chain
 */
struct isns_pdu_s *
isns_get_pdu_request_tail(struct isns_trans_s *trans_p)
{
	struct isns_pdu_s *pdu_p;

	if ((pdu_p = isns_get_pdu_request(trans_p)) != NULL) {
		while (pdu_p->next != NULL)
			pdu_p = pdu_p->next;
	}

	return pdu_p;
}


/*
 * isns_new_pdu - allocates a new PDU and assigns funtion ID and flags
 */
struct isns_pdu_s *
isns_new_pdu(struct isns_config_s *cfg_p, uint16_t trans_id, uint16_t func_id,
    uint16_t flags)
{
	struct isns_buffer_s *buf_p;

	/*
	 * Allocate a buffer at least large enough for our isns_pdu_s struct
	 * and the embedded isns_buffer_s struct for the PDU payload data and 4
	 * bytes of actual payload data.
	 */
	buf_p = isns_new_buffer(/* CONSTCOND */(int)MAX(ISNS_BUF_SIZE,
	    sizeof(struct isns_pdu_s) + sizeof(struct isns_buffer_s) + 4));
	if (buf_p == NULL) {
		DBG("isns_new_pdu: error on isns_new_buffer()\n");
		return NULL;
	}

	return isns_init_pdu(buf_p, cfg_p, trans_id, func_id, flags);
}


/*
 * isns_free_pdu - frees a PDU and all associated buffers
 */
void
isns_free_pdu(struct isns_pdu_s *pdu_p)
{
	struct isns_buffer_s *buf_p, *free_buf_p;

	DBG("isns_free_pdu: %p\n", pdu_p);

	if (pdu_p != NULL) {
		/* Free all payload buffers. */
		buf_p = pdu_p->payload_p;
		while (buf_p != NULL) {
			free_buf_p = buf_p;
			buf_p = buf_p->next;
			isns_free_buffer(free_buf_p);
		}
		/*
		 * Get a pointer to the ISNS buffer in which this PDU is
		 * contained, and then free it.
		 */
		buf_p = ((struct isns_buffer_s *)(void *)(pdu_p))-1 ;
		isns_free_buffer(buf_p);
	}
}


/*
 * isns_send_pdu - initiates the send PDU task
 */
int
isns_send_pdu(ISNS_TRANS trans, struct isns_pdu_s *pdu_p,
    const struct timespec *timeout_p)
{
	struct isns_trans_s *trans_p;
	struct isns_task_s* task_p;
	int rval;

	if (trans == ISNS_INVALID_TRANS) {
		DBG("isns_send_pdu: error - trans=%p\n", trans);
		return EINVAL;
	}

	if (pdu_p == NULL) {
		DBG("isns_send_pdu: error - pdu_p=%p\n", pdu_p);
		return EINVAL;
	}

	trans_p = (struct isns_trans_s *)trans;

	/* Build SEND_PDU task, insert on queue and issue command. */
	task_p = isns_new_task(pdu_p->cfg_p, ISNS_TASK_SEND_PDU,
	    (timeout_p != NULL));
	task_p->var.send_pdu.trans_p = trans_p;
	task_p->var.send_pdu.pdu_p = pdu_p;

	isns_taskq_insert_tail(pdu_p->cfg_p, task_p);

	isns_issue_cmd(pdu_p->cfg_p, ISNS_CMD_PROCESS_TASKQ);

	if (timeout_p == NULL)
		rval = 0;
	else {
		rval = isns_wait_task(task_p, timeout_p);
		if (rval == ETIMEDOUT) {
			DBG("isns_send_pdu: "
			     "timeout on isns_wait_task() trans_id=%d\n",
			     trans_p->id);

			isns_issue_cmd_with_data(task_p->cfg_p,
			    ISNS_CMD_ABORT_TRANS, (void *)&trans_p->id,
			    (int)sizeof(trans_p->id));
		}
	}

	return rval;
}


/*
 * isns_init_pdu - initialize ISNS buffer to be a PDU
 */
static struct isns_pdu_s *
isns_init_pdu(struct isns_buffer_s *buf_p, struct isns_config_s *cfg_p,
    uint16_t trans_id, uint16_t func_id, uint16_t flags)
{
	struct isns_pdu_s *pdu_p;

	/* The config and buffer pointers must be valid here. */
	assert(cfg_p != NULL);
	assert(buf_p != NULL);

	/* The PDU starts at offset 0 for the ISNS buffer data. */
	pdu_p = isns_buffer_data(buf_p, 0);
	buf_p->cur_len = sizeof(struct isns_pdu_s);

	/* Assign PDU members. */
	pdu_p->cfg_p = cfg_p;
	pdu_p->hdr.isnsp_version = ISNSP_VERSION;
	pdu_p->hdr.func_id = func_id;
	pdu_p->hdr.payload_len = 0;
	pdu_p->hdr.flags = flags;
	pdu_p->hdr.trans_id = trans_id;
	pdu_p->hdr.seq_id = 0;
	pdu_p->byteorder_host = 1;
	pdu_p->next = NULL;

	/*
	 * The PDU payload buffer starts after the PDU struct portion in the
	 * ISNS buffer passed in to this function.  So, assign the payload_p
	 * pointer accordingly, and then init the buffer with the proper length
	 * and ISNS_BUF_STATIC type.
	 */
	pdu_p->payload_p = (struct isns_buffer_s *)
	    isns_buffer_data(buf_p, buf_p->cur_len);
	ISNS_INIT_BUFFER(pdu_p->payload_p, (unsigned)(buf_p->alloc_len -
	    sizeof(struct isns_pdu_s) - sizeof(struct isns_buffer_s)),
	    ISNS_BUF_STATIC);

	DBG("isns_init_pdu: %p\n", pdu_p);

	return pdu_p;
}


/*
 * isns_add_pdu_payload_data - add data to PDU payload
 */
static int
isns_add_pdu_payload_data(struct isns_trans_s *trans_p, const void *data_p,
    int len)
{
	struct isns_pdu_s *pdu_p, *new_pdu_p;
	struct isns_buffer_s *buf_p, *new_buf_p;
	const uint8_t *src_p;
	uint8_t *dst_p;
	int pad_bytes;

	/* Get the request PDU for this transaction. */
	if ((pdu_p = isns_get_pdu_request_tail(trans_p)) == NULL) {
		DBG("isns_add_pdu_payload_data: no request PDU\n");
		return EINVAL;
	}
	/* Get the active buffer for this PDU (where data should be copied). */
	buf_p = isns_get_pdu_active_buffer(pdu_p);

	/* Set up source and destination pointers.  Calculate pad bytes. */
	src_p = data_p;
	dst_p = isns_buffer_data(buf_p, buf_p->cur_len);
	pad_bytes = ISNS_PAD4_BYTES(len);

	/*
	 * Move data from source to PDU buffer(s), allocated new pdus and
	 * buffers as necessary to accommodate the data.
	 */
	while (len--) {
		/* If at max for one PDU payload, add a new PDU. */
		if (pdu_p->hdr.payload_len == ISNS_MAX_PDU_PAYLOAD) {
			new_pdu_p = isns_new_pdu(trans_p->cfg_p,
			    trans_p->id, trans_p->func_id, pdu_p->hdr.flags);
			if (new_pdu_p == NULL)
				return ENOMEM;
			isns_add_pdu_request(trans_p, new_pdu_p);
			pdu_p = new_pdu_p;
			buf_p = pdu_p->payload_p;
			dst_p = isns_buffer_data(buf_p, 0);
		}
		/* If at end of current buffer, add a new buffer. */
		if (buf_p->cur_len == buf_p->alloc_len) {
			if (buf_p->next != NULL)
				buf_p = buf_p->next;
			else {
				new_buf_p = isns_new_buffer(0);
				if (new_buf_p == NULL)
					return ENOMEM;
				buf_p->next = new_buf_p;
				buf_p = new_buf_p;
			}
			dst_p = isns_buffer_data(buf_p, 0);
		}
		pdu_p->hdr.payload_len++;
		buf_p->cur_len++;
		*dst_p++ = *src_p++;
	}

	/*
	 * Since the buffer alloc length is always a multiple of 4, we are
	 * guaranteed to have enough room for the pad bytes.
	 */
	if (pad_bytes > 0) {
		pdu_p->hdr.payload_len += pad_bytes;
		buf_p->cur_len += pad_bytes;
		while (pad_bytes--)
			*dst_p++ = 0;
	}

	return 0;
}


/*
 * isns_get_tlv_info_advance - advances pdu/buffer pointers if at end of
 *			       current buffer.
 */
static void
isns_get_tlv_info_advance(struct isns_get_tlv_info_s *info_p)
{
	if ((info_p->buf_p != NULL) &&
	    (info_p->buf_ofs == (int)info_p->buf_p->cur_len)) {
		info_p->buf_p = info_p->buf_p->next;
		info_p->buf_ofs = 0;
	}

	if ((info_p->buf_p == NULL) && (info_p->pdu_p->next != NULL)) {
		info_p->pdu_p = info_p->pdu_p->next;
		info_p->buf_p = isns_get_pdu_head_buffer(info_p->pdu_p);
		info_p->buf_ofs = 0;
	}
}


/*
 * isns_get_tlv_uint32 - retrieve host-ordered uint32_t from PDU buffer at
 *			 starting offset and adjusts isns_get_tlv_info
 *			 pointers accordingly.
 */
static int
isns_get_tlv_uint32(struct isns_get_tlv_info_s *info_p, uint32_t *uint32_p)
{
	/* Advance to next buffer/pdu (if necessary). */
	isns_get_tlv_info_advance(info_p);

	if ((info_p->buf_p == NULL) ||
	    ((info_p->buf_ofs + 4) > (int)info_p->buf_p->cur_len)) {
		DBG("isns_get_tlv_uint32: end of buffer reached\n");
		return EFAULT;
	}

	*uint32_p = ntohl(*(uint32_t *)isns_buffer_data(info_p->buf_p,
	    info_p->buf_ofs));
	info_p->buf_ofs += 4;

	return 0;
}


/*
 * isns_get_tlv_data - retrieves data from PDU buffer at starting offset
 *		       for TLV data contained in specified isns_get_tlv_info.
 */
static int
isns_get_tlv_data(struct isns_get_tlv_info_s *info_p, int data_len,
    void **data_pp)
{
	struct isns_buffer_s *extra_buf_p;
	struct isns_get_tlv_info_s gti;
	uint8_t *data_p, *extra_data_p;
	int bytes_remaining, cbytes;

	/* First, NULL return data pointer. */
	*data_pp = NULL;

	/* Advance to next buffer/pdu (if necessary). */
	isns_get_tlv_info_advance(info_p);

	/* Make sure we have a current get tlv info buffer. */
	if (info_p->buf_p == NULL) {
		DBG("isns_get_tlv_data: no next buffer\n");
		return EFAULT;
	}

	/* Get pointer into buffer where desired TLV resides. */
	data_p = isns_buffer_data(info_p->buf_p, info_p->buf_ofs);

	/* TLV data completely resides in current buffer. */
	if ((info_p->buf_ofs + data_len) <= (int)info_p->buf_p->cur_len) {
		info_p->buf_ofs += data_len;
		*data_pp = data_p;
		return 0;
	}

	/*
	 * TLV data extends into next buffer so an "extra" buffer is allocated
	 * that is large enough to hold the entire data value.  The extra buffer
	 * is added to the transaction's extra buffer list so we can free it
	 * when the transaction is freed.
	 */

	if ((extra_buf_p = isns_new_buffer(data_len)) == NULL) {
		DBG("isns_get_tlv_data: error on isns_new_buffer()\n");
		return ENOMEM;
	}
	if (info_p->extra_buf_list == NULL)
		info_p->extra_buf_list = extra_buf_p;
	else {
		extra_buf_p->next = info_p->extra_buf_list;
		info_p->extra_buf_list = extra_buf_p;
	}

	/* Setup to copy data bytes out to extra buffer. */
	gti = *info_p;
	extra_data_p = isns_buffer_data(extra_buf_p, 0);
	bytes_remaining = data_len;

	while (bytes_remaining > 0) {
		/*
		 * Advance to next buffer/pdu (if necessary), using local copy
		 * of the get_tlv_info structure.
		 */
		isns_get_tlv_info_advance(&gti);
		if (gti.buf_p == NULL) {
			DBG("isns_get_tlv_data: no next buffer\n");
			return EFAULT;
		}

		data_p = isns_buffer_data(gti.buf_p, gti.buf_ofs);

		cbytes = MIN(bytes_remaining, ((int)gti.buf_p->cur_len - gti.buf_ofs));
		bytes_remaining -= cbytes;
		gti.buf_ofs += cbytes;
		while (cbytes--)
			*extra_data_p++ = *data_p++;
	}

	/* Update isns_get_tlv_info with our local copy. */
	*info_p = gti;

	/* Assign return data pointer. */
	*data_pp = isns_buffer_data(extra_buf_p, 0);

	return 0;
}


/*
 * isns_get_pdu_response_status - returns status in PDU response
 *
 * Returns: 0 - success
 *	    EPERM - operation not permitted, trans not complete
 *	    EINVAL - invalid trans PDU response/payload
 */
int
isns_get_pdu_response_status(struct isns_trans_s *trans_p, uint32_t *status_p)
{
	struct isns_pdu_s *pdu_p;

	if ((isns_get_trans_flags(trans_p) & ISNS_TRANSF_COMPLETE) == 0)
		return EPERM;

	pdu_p = isns_get_pdu_response(trans_p);
	if ((pdu_p == NULL)
	    || (pdu_p->payload_p == NULL)
	    || (pdu_p->payload_p->cur_len < 4))
	    	return EINVAL;

	*status_p = htonl(*(uint32_t *)isns_buffer_data(pdu_p->payload_p, 0));

	return 0;
}


/*
 * isns_add_pdu_list - adds pdu to specified pdu list
 */
static void
isns_add_pdu_list(struct isns_pdu_s **list_pp, struct isns_pdu_s *pdu_p)
{
	struct isns_pdu_s *p, *p_prev;


	if (*list_pp == NULL) {
		*list_pp = pdu_p;
		pdu_p->next = NULL;
		return;
	}

	p = *list_pp;
	while (p != NULL) {
		if (pdu_p->hdr.seq_id < p->hdr.seq_id) {
			if (p == *list_pp) {
				*list_pp = pdu_p;
				pdu_p->next = p;
			} else {
				p_prev = *list_pp;
				while (p_prev->next != p)
					p_prev = p_prev->next;
				p_prev->next = pdu_p;
				pdu_p->next = p;
			}

			return;
		}
		p = p->next;
	}

	/* pdu_p->hdr.seq_id > hdr.seq_id of all list elements */
	p = *list_pp;
	while (p->next != NULL)
		p = p->next;
	p->next = pdu_p;
	pdu_p->next = NULL;
}


/*
 * isns_get_pdu_head_buffer - returns PDU payload head buffer
 */
static struct isns_buffer_s *
isns_get_pdu_head_buffer(struct isns_pdu_s *pdu_p)
{
	return pdu_p->payload_p;
}


#if 0
/*
 * isns_get_pdu_tail_buffer - returns PDU payload tail buffer
 */
static struct isns_buffer_s *
isns_get_pdu_tail_buffer(struct isns_pdu_s *pdu_p)
{
	struct isns_buffer_s *buf_p;

	buf_p = pdu_p->payload_p;
	if (buf_p != NULL)
		while (buf_p->next != NULL) buf_p = buf_p->next;

	return buf_p;
}
#endif


/*
 * isns_get_pdu_active_buffer - returns PDU payload "active buffer where the
 *				next TLV/data should be written
 */
static struct isns_buffer_s *
isns_get_pdu_active_buffer(struct isns_pdu_s *pdu_p)
{
	struct isns_buffer_s *buf_p;

	buf_p = pdu_p->payload_p;
	while ((buf_p->next != NULL) && (buf_p->cur_len == buf_p->alloc_len)) {
		buf_p = buf_p->next;
	}

	return buf_p;
}


/*
 * isns_get_next_trans_id - returns next ISNS transaction ID to use
 */
static uint32_t
isns_get_next_trans_id(void)
{
	static int	trans_id = 1;

	return trans_id++;
}


#ifdef ISNS_DEBUG
/*
 * isns_dump_pdu - dumps PDU contents
 */
void
isns_dump_pdu(struct isns_pdu_s *pdu_p)
{
	int n, pos;
	struct isns_buffer_s *buf_p;
	uint8_t *p;
	char text[17];

	if (pdu_p == NULL) {
		DBG("isns_dump_pdu: pdu_p is NULL\n");
		return;
	}

	DBG("pdu header:\n");
	if (pdu_p->byteorder_host) {
	    DBG("ver=0x%04X, func=%d(%s), len=%d, flags=0x%04X, trans=%d, "
	        "seq=%d\n",
		pdu_p->hdr.isnsp_version,
		pdu_p->hdr.func_id & ~0x8000,
		(pdu_p->hdr.func_id & 0x8000 ? "rsp" : "req"),
		pdu_p->hdr.payload_len,
		pdu_p->hdr.flags,
		pdu_p->hdr.trans_id,
		pdu_p->hdr.seq_id);
	} else {
	    DBG("ver=0x%04X, func=%d(%s), len=%d, flags=0x%04X, trans=%d, "
	        "seq=%d\n",
		isns_ntohs(pdu_p->hdr.isnsp_version),
		isns_ntohs(pdu_p->hdr.func_id) & ~0x8000,
		(pdu_p->hdr.func_id & 0x0080 ? "rsp" : "req"),
		isns_ntohs(pdu_p->hdr.payload_len),
		isns_ntohs(pdu_p->hdr.flags),
		isns_ntohs(pdu_p->hdr.trans_id),
		isns_ntohs(pdu_p->hdr.seq_id));
	}

	DBG("pdu buffers:\n");
	buf_p = pdu_p->payload_p;
	while (buf_p != NULL) {
		DBG("[%p]: alloc_len=%d, cur_len=%d\n",
		    buf_p, buf_p->alloc_len, buf_p->cur_len);
		buf_p = buf_p->next;
	}

	DBG("pdu payload:\n");
	buf_p = pdu_p->payload_p;
	if (buf_p == NULL) {
		DBG("<none>\n");
		return;
	}

	pos = 0;
	memset(text, 0, 17);
	while (buf_p != NULL) {
		p = isns_buffer_data(buf_p, 0);
		for (n = 0; n < buf_p->cur_len; n++) {
			DBG("%02X ", *p);
			text[pos] = (isprint(*p) ? *p : '.');
			pos++;
 			p++;

			if ((pos % 16) == 0) {
				DBG("   %s\n", text);
				memset(text, 0, 17);
				pos = 0;
			}
		}
		buf_p = buf_p->next;
	}

	if ((pos % 16) != 0)
		DBG("%*c   %s\n", (16 - (pos % 16)) * 3, ' ', text);
}
#endif /* ISNS_DEBUG */
