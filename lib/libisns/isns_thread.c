/*	$NetBSD: isns_thread.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: isns_thread.c,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $");


/*
 * isns_thread.c
 */

#include <sys/types.h>

#include <unistd.h>

#include "isns.h"
#include "isns_config.h"
#include "isns_defs.h"

static struct iovec read_buf[2 + (ISNS_MAX_PDU_PAYLOAD / ISNS_BUF_SIZE) +
    ((ISNS_MAX_PDU_PAYLOAD % ISNS_BUF_SIZE) != 0)];

static struct isns_task_s *isns_get_next_task(struct isns_config_s *); 

/*
 * isns_control_thread()
 */ 
void *
isns_control_thread(void *arg)
{
	struct isns_config_s *cfg_p = (struct isns_config_s *)arg;
	struct kevent evt_chgs[5], *evt_p;

	int n, nevents;
	isns_kevent_handler *evt_handler_p;
	int run_thread;

	run_thread = 1;

	while (run_thread) {
		/* if no task outstanding, check queue here and send PDU */
		while ((cfg_p->curtask_p == NULL)
		    && ((cfg_p->curtask_p = isns_get_next_task(cfg_p)) != NULL))
			isns_run_task(cfg_p->curtask_p);

		nevents = kevent(cfg_p->kq, NULL, 0, evt_chgs,
		    ARRAY_ELEMS(evt_chgs), NULL);

		DBG("isns_control_thread: kevent() nevents=%d\n", nevents);

		for (n = 0, evt_p = evt_chgs; n < nevents; n++, evt_p++) {
			DBG("event[%d] - data=%d\n", n, (int)evt_p->data);
			evt_handler_p = (void *)evt_p->udata;
			run_thread = (evt_handler_p(evt_p, cfg_p) == 0);
		}
	}

	return 0;
}

/*
 * isns_get_next_task()
 */
static struct isns_task_s *
isns_get_next_task(struct isns_config_s *cfg_p)
{
	struct isns_task_s *task_p = NULL;


	DBG("isns_get_next_task: entered\n");

	task_p = isns_taskq_remove(cfg_p);

	if (cfg_p->sd_connected)
		return task_p;
	else {
		if (task_p == NULL)
			return NULL;
		else {
			if (task_p->task_type != ISNS_TASK_INIT_SOCKET_IO) {
				isns_taskq_insert_head(cfg_p, task_p);

				task_p = isns_new_task(cfg_p,
				    ISNS_TASK_RECONNECT_SERVER, 0);
				task_p->var.reconnect_server.ai_p = cfg_p->ai_p;
			}

			return task_p;
		}
	}
}

/*
 * isns_kevent_pipe()
 */ 
int
isns_kevent_pipe(struct kevent* evt_p, struct isns_config_s *cfg_p)
{
	uint8_t cmd_type;
	int force_isns_stop;
	uint16_t trans_id;
	ssize_t rbytes;
	int pipe_nbytes;

	force_isns_stop = 0;
	pipe_nbytes = (int)evt_p->data;

	while (pipe_nbytes > 0) {
		rbytes = read(cfg_p->pipe_fds[0], &cmd_type,
		    sizeof(cmd_type));
		if (rbytes < 0) {
			DBG("isns_kevent_pipe: error on wepe_sys_read\n");
			/*?? should we break here? */
			continue;
		}

		pipe_nbytes -= (int)rbytes;
		switch (cmd_type) {
		case ISNS_CMD_PROCESS_TASKQ:
			DBG("isns_kevent_pipe: ISNS_CMD_PROCESS_TASKQ\n");
			break;

		case ISNS_CMD_ABORT_TRANS:
			DBG("isns_kevent_pipe: ISNS_CMD_ABORT_TRANS\n");
			rbytes = read(cfg_p->pipe_fds[0], &trans_id,
			    sizeof(trans_id));
			if ((rbytes < 0) && (rbytes == sizeof(trans_id)))
				isns_abort_trans(cfg_p, trans_id);
			else
				DBG("isns_kevent_pipe: "
				    "error reading trans id\n");
			pipe_nbytes -= (int)rbytes;
			break;

		case ISNS_CMD_STOP:
			DBG("isns_kevent_pipe: ISNS_CMD_STOP\n");
			force_isns_stop = 1;
			pipe_nbytes = 0;
			break;

		default:
			DBG("isns_kevent_pipe: unknown command (cmd=%d)\n",
			    cmd_type);
			break;
		}
	}

	return (force_isns_stop ? 1 : 0);
}

/*
 * isns_is_trans_complete()
 */
static int
isns_is_trans_complete(struct isns_trans_s *trans_p)
{
	struct isns_pdu_s *pdu_p;
	uint16_t count;

	pdu_p = trans_p->pdu_rsp_list;
	count = 0;
	while (pdu_p->next != NULL) {
		if (pdu_p->hdr.seq_id != count++) return 0;
		pdu_p = pdu_p->next;
	}
	if ((pdu_p->hdr.seq_id != count) ||
	    !(pdu_p->hdr.flags & ISNS_FLAG_LAST_PDU))
		return 0;

	return 1;
}

/*
 * isns_is_valid_resp()
 */
static int
isns_is_valid_resp(struct isns_trans_s *trans_p, struct isns_pdu_s *pdu_p)
{
	struct isns_pdu_s *curpdu_p;

	if (pdu_p->hdr.trans_id != trans_p->id)
		return 0;
	if (pdu_p->hdr.func_id != (trans_p->func_id | 0x8000))
		return 0;
	curpdu_p = trans_p->pdu_rsp_list;
	while (curpdu_p != NULL) {
		if (curpdu_p->hdr.seq_id == pdu_p->hdr.seq_id) return 0;
		curpdu_p = curpdu_p->next;
	}

	return 1;
}

/*
 * isns_process_in_pdu()
 */
static void
isns_process_in_pdu(struct isns_config_s *cfg_p)
{
	struct isns_task_s *curtask_p;
	struct isns_trans_s *trans_p;
	
	DBG("isns_process_in_pdu: entered\n");

	if ((curtask_p = cfg_p->curtask_p) == NULL)
		isns_free_pdu(cfg_p->pdu_in_p);
	else if ((trans_p = curtask_p->var.send_pdu.trans_p) == NULL)
		isns_free_pdu(cfg_p->pdu_in_p);
	else if (!isns_is_valid_resp(trans_p, cfg_p->pdu_in_p))
		isns_free_pdu(cfg_p->pdu_in_p);
	else {
		isns_add_pdu_response(trans_p, cfg_p->pdu_in_p);

		if (isns_is_trans_complete(trans_p)) {
			isns_complete_trans(trans_p);
			isns_end_task(curtask_p);
		}
	}

	cfg_p->pdu_in_p = NULL;
}

/*
 * isns_kevent_socket()
 */
int
isns_kevent_socket(struct kevent *evt_p, struct isns_config_s *cfg_p)
{
	struct iovec *iovp;
	struct isns_buffer_s *curbuf_p, *newbuf_p;
	struct isns_pdu_s *pdu_p;
	int64_t bavail; /* bytes available in socket buffer */
	uint32_t cur_len, buf_len, unread_len, rd_len, b_len;
	ssize_t rv;
	uint16_t payload_len;
	int iovcnt, more, transport_evt;


	DBG("isns_kevent_socket: entered\n");

	transport_evt = 0;
	bavail = evt_p->data;
	iovp = read_buf;

	more = (bavail > 0);
	while (more) {
		if (cfg_p->pdu_in_p == NULL) {
			/*
 	 		 * Try to form a valid pdu by starting with the hdr.
			 * If there isn't enough data in the socket buffer
			 * to form a full hdr, just return.
 	 		 *
 	 		 * Once we have read in our hdr, allocate all buffers
			 * needed.
 	 		 */

			if (bavail < (int64_t)sizeof(struct isns_pdu_hdr_s))
				return 0;

			/* Form a placeholder pdu */
			pdu_p = isns_new_pdu(cfg_p, 0, 0, 0);

			/* Read the header into our placeholder pdu */
			read_buf[0].iov_base = &(pdu_p->hdr);
			read_buf[0].iov_len = sizeof(struct isns_pdu_hdr_s);
			iovcnt = 1;

			iovp = read_buf;
			rv = isns_socket_readv(cfg_p->sd, iovp, iovcnt);
			if ((rv == 0) || (rv == -1)) {
				DBG("isns_kevent_socket: isns_socket_readv(1) "
				    "returned %d\n", rv);
				transport_evt = 1;
				break;
			}

			bavail -= sizeof(struct isns_pdu_hdr_s);
			/*
			 * ToDo: read until sizeof(struct isns_pdu_hdr_s) has
			 *       been read in. This statement should be
			 *
			 *       bavail -= rv;
			 */

			/* adjust byte order */
			pdu_p->hdr.isnsp_version = isns_ntohs(pdu_p->hdr.
			    isnsp_version);
			pdu_p->hdr.func_id = isns_ntohs(pdu_p->hdr.func_id);
			pdu_p->hdr.payload_len = isns_ntohs(pdu_p->hdr.
			    payload_len);
			pdu_p->hdr.flags = isns_ntohs(pdu_p->hdr.flags);
			pdu_p->hdr.trans_id = isns_ntohs(pdu_p->hdr.trans_id);
			pdu_p->hdr.seq_id = isns_ntohs(pdu_p->hdr.seq_id);
			pdu_p->byteorder_host = 1;

			/* Try to sense early whether we might have garbage */
			if (pdu_p->hdr.isnsp_version != ISNSP_VERSION) {
				DBG("isns_kevent_socket: pdu_p->hdr."
				    "isnsp_version != ISNSP_VERSION\n");
				isns_free_pdu(pdu_p);

				transport_evt = 1;
				break;
			}

			/* Allocate all the necessary payload buffers */ 
			payload_len = pdu_p->hdr.payload_len;
			curbuf_p = pdu_p->payload_p;
			buf_len = 0;
			while (buf_len + curbuf_p->alloc_len < payload_len) {
				buf_len += curbuf_p->alloc_len;
				newbuf_p = isns_new_buffer(0);
				curbuf_p->next = newbuf_p;
				curbuf_p = newbuf_p;
			}
			curbuf_p->next = NULL;

			/* Hold on to our placeholder pdu */
			cfg_p->pdu_in_p = pdu_p;
			more = (bavail > 0) ? 1 : 0;
		} else if (bavail > 0) {
			/*
 	 		 * Fill in the pdu payload data.
			 *
 	 		 * If we can fill it all in now
	 		 *     -AND- it corresponds to the active transaction
			 *           then add the pdu to the transaction's
			 *           pdu_rsp_list
	 		 *     -AND- it does not correspond to the active
			 *           transaction (or there is no active
			 *           transaction) then drop it on the floor.
			 * We may not be able to fill it all in now.
	 		 *     -EITHER WAY- fill in as much payload data now
			 *                  as we can.
 	 		 */

			/* Refer to our placeholder pdu */
			pdu_p = cfg_p->pdu_in_p;

			/* How much payload data has been filled in? */
			cur_len = 0;
			curbuf_p = pdu_p->payload_p;
			while (curbuf_p->cur_len == curbuf_p->alloc_len) {
				cur_len += curbuf_p->cur_len;
				curbuf_p = curbuf_p->next;
			}
			cur_len += curbuf_p->cur_len;

			/* How much payload data is left to be filled in? */
			unread_len = pdu_p->hdr.payload_len - cur_len;

			/* Read as much remaining payload data as possible */
			iovcnt = 0;
			while (curbuf_p->next != NULL) {
				read_buf[iovcnt].iov_base = isns_buffer_data(
			    	    curbuf_p, curbuf_p->cur_len); 
				read_buf[iovcnt].iov_len = curbuf_p->alloc_len -
			    	    curbuf_p->cur_len;
				iovcnt++;
				
				curbuf_p = curbuf_p->next;
			}
			read_buf[iovcnt].iov_base = isns_buffer_data(curbuf_p,
		    	    curbuf_p->cur_len);
			read_buf[iovcnt].iov_len = unread_len;
			iovcnt++;

			rv = isns_socket_readv(cfg_p->sd, iovp, iovcnt);
			if ((rv == 0) || (rv == -1)) {
				DBG("isns_kevent_socket: isns_socket_readv(2) "
			    	    "returned %d\n",rv);
				isns_free_pdu(cfg_p->pdu_in_p);
				cfg_p->pdu_in_p = NULL;

				transport_evt = 1;
				break;
			}

			/* Update cur_len in buffers that newly have data */
			curbuf_p = pdu_p->payload_p;
			while (curbuf_p->cur_len == curbuf_p->alloc_len)
				curbuf_p = curbuf_p->next;
			
			rd_len = (uint32_t)rv;
			do {
				b_len = curbuf_p->alloc_len - curbuf_p->cur_len;
				if (rd_len > b_len) {
					curbuf_p->cur_len = curbuf_p->alloc_len;
					rd_len -= b_len;
				} else {
					curbuf_p->cur_len += rd_len;
					break;
				}

				curbuf_p = curbuf_p->next;
			} while (curbuf_p != NULL);

			bavail -= rv;

			if (rv == (int)unread_len)
				isns_process_in_pdu(cfg_p);

			more = (bavail > (int64_t)sizeof(struct isns_pdu_hdr_s)) ? 1 : 0;
		}
	}

	transport_evt |= (evt_p->flags & EV_EOF);
	if (transport_evt) {
		DBG("isns_kevent_socket: processing transport event\n");

		isns_socket_close(cfg_p->sd);
		cfg_p->sd_connected = 0;

		if (cfg_p->curtask_p != NULL)
			isns_process_connection_loss(cfg_p);

		if (cfg_p->pdu_in_p != NULL) {
			isns_free_pdu(cfg_p->pdu_in_p);
			cfg_p->pdu_in_p = NULL;
		}
	}

	return 0;
}

/* ARGSUSED */
/*
 * isns_kevent_timer_recon()
 */
int
isns_kevent_timer_recon(struct kevent *evt_p, struct isns_config_s *cfg_p)
{
	int rv;


	DBG("isns_kevent_timer_recon: entered\n");

	rv = isns_socket_create(&(cfg_p->sd), cfg_p->ai_p->ai_family,
		cfg_p->ai_p->ai_socktype);
	if (rv != 0)
		return 0;

	rv = isns_socket_connect(cfg_p->sd, cfg_p->ai_p->ai_addr,
	    cfg_p->ai_p->ai_addrlen);
	if (rv == 0) {
		/* Remove ISNS_EVT_TIMER_RECON from kqueue */
		rv = isns_change_kevent_list(cfg_p,
		    (uintptr_t)ISNS_EVT_TIMER_RECON, EVFILT_TIMER, EV_DELETE,
		    (int64_t)0, (intptr_t)0);
		if (rv == -1)
			DBG("isns_kevent_timer_recon: error on "
			    "isns_change_kevent_list(1)\n");

		cfg_p->sd_connected = 1;

		/* Add cfg_p->sd to kqueue */
		rv = isns_change_kevent_list(cfg_p, (uintptr_t)cfg_p->sd,
		    EVFILT_READ, EV_ADD | EV_CLEAR, (int64_t)0,
		    (intptr_t)isns_kevent_socket);
		if (rv == -1)
			DBG("isns_kevent_timer_recon: error on "
			    "isns_change_kevent_list(2)\n");

		isns_end_task(cfg_p->curtask_p);
	}

	return 0;
}


/* ARGSUSED */
/*
 * isns_kevent_timer_refresh
 */
int
isns_kevent_timer_refresh(struct kevent* evt_p, struct isns_config_s *cfg_p)
{
	struct isns_refresh_s *ref_p;
	ISNS_TRANS trans;
	uint32_t status;
	int rval;

	DBG("isns_kevent_timer_refresh: entered\n");

	/* If refresh info pointer NULL, or no name assigned, just return. */
	ref_p = cfg_p->refresh_p;
	if ((ref_p == NULL) || (ref_p->node[0] == '\0'))
	    	return 0;

	if (ref_p->trans_p != NULL) {
		/* If the previous refresh trans is not complete, return. */
		rval = isns_get_pdu_response_status(ref_p->trans_p, &status);
		if (rval == EPERM) {
			DBG("isns_kevent_timer_refresh: "
			    "prev refresh trans not complete\n");
			return 0;
		}
		/* Free previous refresh trans. */
		isns_free_trans(ref_p->trans_p);
		ref_p->trans_p = NULL;
	}

	/* Build new refresh transaction and send it. */
	trans = isns_new_trans((ISNS_HANDLE)cfg_p, isnsp_DevAttrQry, 0);
	if (trans == ISNS_INVALID_TRANS) {
		DBG("isns_kevent_timer_refresh: error on isns_new_trans()\n");
		return 0;
	}

	ref_p->trans_p = (struct isns_trans_s *)trans;
	/* First we add our source attribute */
	isns_add_string(trans, isnst_iSCSIName, ref_p->node);
	/* Now add our message attribute */
	isns_add_string(trans, isnst_iSCSIName, ref_p->node);
	isns_add_tlv(trans, isnst_Delimiter, 0, NULL);
	/* and finally the operating attributes */
	isns_add_tlv(trans, isnst_EID, 0, NULL);
	isns_send_trans(trans, NULL, NULL);

	return 0;
}
