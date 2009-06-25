/*
 * IMPORTANT:  READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. 
 * By downloading, copying, installing or using the software you agree
 * to this license.  If you do not agree to this license, do not
 * download, install, copy or use the software.
 *
 * Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * -Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the
 *  distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL INTEL
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "config.h"

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
           

#include "iscsiprotocol.h"
#include "conffile.h"
#include "storage.h"
#include "target.h"
#include "device.h"
#include "iscsi-md5.h"
#include "parameters.h"
#include "iscsi.h"

enum {
	TARGET_SHUT_DOWN     = 0,
	TARGET_INITIALIZING  = 1,
	TARGET_INITIALIZED   = 2,
	TARGET_SHUTTING_DOWN = 3
};

/***********
 * Private *
 ***********/

static target_session_t	*g_session;
static iscsi_queue_t	g_session_q;
static iscsi_mutex_t	g_session_q_mutex;

/*********************
 * Private Functions *
 *********************/

static char *
get_iqn(target_session_t *sess, uint32_t t, char *buf, size_t size)
{
	targv_t	*targv;

	targv = sess->globals->lunv;
	if (targv->v[t].iqn != NULL) {
		(void) strlcpy(buf, targv->v[t].iqn, size);
		return buf;
	}
	(void) snprintf(buf, size, "%s:%s",
			iscsi_target_getvar(sess->globals, "iqn"),
			targv->v[t].target);
	return buf;
}

static int 
reject_t(target_session_t * sess, uint8_t *header, uint8_t reason)
{
	iscsi_reject_t  reject;
	uint8_t   rsp_header[ISCSI_HEADER_LEN];

	iscsi_trace_error(__FILE__, __LINE__, "reject %x\n", reason);
	reject.reason = reason;
	reject.length = ISCSI_HEADER_LEN;
	reject.StatSN = ++(sess->StatSN);
	reject.ExpCmdSN = sess->ExpCmdSN;
	reject.MaxCmdSN = sess->MaxCmdSN;
	reject.DataSN = 0;	/* SNACK not yet implemented */

	if (iscsi_reject_encap(rsp_header, &reject) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_reject_encap() failed\n");
		return -1;
	}
	if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, ISCSI_HEADER_LEN,
		     header, ISCSI_HEADER_LEN, 0) != 2 * ISCSI_HEADER_LEN) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
		return -1;
	}
	return 0;
}

static int 
scsi_command_t(target_session_t *sess, uint8_t *header)
{
	target_cmd_t    cmd;
	iscsi_scsi_cmd_args_t scsi_cmd;
	iscsi_scsi_rsp_t scsi_rsp;
	iscsi_read_data_t data;
	uint8_t   rsp_header[ISCSI_HEADER_LEN];
	uint32_t        DataSN = 0;

	(void) memset(&scsi_cmd, 0x0, sizeof(scsi_cmd));
	if (iscsi_scsi_cmd_decap(header, &scsi_cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_scsi_cmd_decap() failed\n");
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: SCSI Command (CmdSN %u, op %#x)\n", sess->id, scsi_cmd.CmdSN, scsi_cmd.cdb[0]);

	/* For Non-immediate commands, the CmdSN should be between ExpCmdSN  */
	/* and MaxCmdSN, inclusive of both.  Otherwise, ignore the command */
	if ((!scsi_cmd.immediate) &&
	    ((scsi_cmd.CmdSN < sess->ExpCmdSN) || (scsi_cmd.CmdSN > sess->MaxCmdSN))) {
		iscsi_trace_error(__FILE__, __LINE__, "CmdSN(%d) of SCSI Command not valid, ExpCmdSN(%d) MaxCmdSN(%d). Ignoring the command\n", scsi_cmd.CmdSN, sess->ExpCmdSN, sess->MaxCmdSN);
		return 0;
	}
	/* Arg check.   */
	scsi_cmd.attr = 0;	/* Temp fix FIXME */
	/*
	 * RETURN_NOT_EQUAL("ATTR (FIX ME)", scsi_cmd.attr, 0, NO_CLEANUP,
	 * -1);
	 */

	/* Check Numbering */

	if (scsi_cmd.CmdSN != sess->ExpCmdSN) {
		iscsi_trace_warning(__FILE__, __LINE__, "Expected CmdSN %d, got %d. (ignoring and resetting expectations)\n",
			      sess->ExpCmdSN, scsi_cmd.CmdSN);
		sess->ExpCmdSN = scsi_cmd.CmdSN;
	}
	/* Check Transfer Lengths */
	if (sess->sess_params.first_burst_length
	    && (scsi_cmd.length > sess->sess_params.first_burst_length)) {
		iscsi_trace_error(__FILE__, __LINE__, "scsi_cmd.length (%u) > FirstBurstLength (%u)\n",
		     scsi_cmd.length, sess->sess_params.first_burst_length);
		scsi_cmd.status = 0x02;
		scsi_cmd.length = 0;
		goto response;
	}
	if (sess->sess_params.max_data_seg_length
	    && (scsi_cmd.length > sess->sess_params.max_data_seg_length)) {
		iscsi_trace_error(__FILE__, __LINE__, "scsi_cmd.length (%u) > MaxRecvDataSegmentLength (%u)\n",
		    scsi_cmd.length, sess->sess_params.max_data_seg_length);
		return -1;
	}

#if 0
	/* commented out in original Intel reference code */
	if (scsi_cmd.final && scsi_cmd.output) {
		RETURN_NOT_EQUAL("Length", scsi_cmd.length, scsi_cmd.trans_len, NO_CLEANUP, -1);
	}
#endif

	/* Read AHS.  Need to optimize/clean this.   */
	/* We should not be calling malloc(). */
	/* We need to check for properly formated AHS segments. */

	if (scsi_cmd.ahs_len) {
		uint32_t        ahs_len;
		uint8_t  *ahs_ptr;
		uint8_t   ahs_type;

		scsi_cmd.ahs = NULL;
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "reading %u bytes AHS\n", scsi_cmd.ahs_len);
		if ((scsi_cmd.ahs = iscsi_malloc_atomic((unsigned)scsi_cmd.ahs_len)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
			return -1;
		}
#define AHS_CLEANUP do {						\
	if (scsi_cmd.ahs != NULL) {					\
		iscsi_free_atomic(scsi_cmd.ahs);			\
	}								\
} while (/* CONSTCOND */ 0)
		if (iscsi_sock_msg(sess->sock, 0, (unsigned)scsi_cmd.ahs_len, scsi_cmd.ahs, 0) != scsi_cmd.ahs_len) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			AHS_CLEANUP;
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "read %u bytes AHS\n", scsi_cmd.ahs_len);
		for (ahs_ptr = scsi_cmd.ahs; ahs_ptr < (scsi_cmd.ahs + scsi_cmd.ahs_len - 1) ; ahs_ptr += ahs_len) {
			ahs_len = ISCSI_NTOHS(*((uint16_t *) (void *)ahs_ptr));
			RETURN_EQUAL("AHS Length", ahs_len, 0, AHS_CLEANUP, -1);
			switch (ahs_type = *(ahs_ptr + 2)) {
			case ISCSI_AHS_EXTENDED_CDB:
				iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Got ExtendedCDB AHS (%u bytes extra CDB)\n", ahs_len - 1);
				scsi_cmd.ext_cdb = ahs_ptr + 4;
				break;
			case ISCSI_AHS_BIDI_READ:
				scsi_cmd.bidi_trans_len = ISCSI_NTOHL(*((uint32_t *) (void *) (ahs_ptr + 4)));
				*((uint32_t *) (void *) (ahs_ptr + 4)) = scsi_cmd.bidi_trans_len;
				iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Got Bidirectional Read AHS (expected read length %u)\n", scsi_cmd.bidi_trans_len);
				break;
			default:
				iscsi_trace_error(__FILE__, __LINE__, "unknown AHS type %x\n", ahs_type);
				AHS_CLEANUP;
				return -1;
			}
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "done parsing %u bytes AHS\n", scsi_cmd.ahs_len);
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "no AHS to read\n");
		scsi_cmd.ahs = NULL;
	}

	sess->ExpCmdSN++;
	sess->MaxCmdSN++;

	/* Execute cdb.  device_command() will set scsi_cmd.input if */
	/* there is input data and set the length of the input */
	/* to either scsi_cmd.trans_len or scsi_cmd.bidi_trans_len, depending  */
	/* on whether scsi_cmd.output was set. */

	if (scsi_cmd.input) {
		scsi_cmd.send_data = sess->buff;
	}
	scsi_cmd.input = 0;
	cmd.scsi_cmd = &scsi_cmd;
	cmd.callback = NULL;
	if (device_command(sess, &cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "device_command() failed\n");
		AHS_CLEANUP;
		return -1;
	}
	/* Send any input data */

	scsi_cmd.bytes_sent = 0;
	if (!scsi_cmd.status && scsi_cmd.input) {
		struct iovec    sg_singleton;
		struct iovec   *sg, *sg_orig, *sg_new = NULL;
		int             sg_len_orig, sg_len;
		uint32_t        offset, trans_len;
		int             fragment_flag = 0;
		int             offset_inc;
#define SG_CLEANUP do {							\
	if (fragment_flag) {						\
		iscsi_free_atomic(sg_new);				\
	}								\
} while (/* CONSTCOND */ 0)
		if (scsi_cmd.output) {
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending %u bytes bi-directional input data\n", scsi_cmd.bidi_trans_len);
			trans_len = scsi_cmd.bidi_trans_len;
		} else {
			trans_len = scsi_cmd.trans_len;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending %u bytes input data as separate PDUs\n", trans_len);

		if (scsi_cmd.send_sg_len) {
			sg_orig = (struct iovec *) (void *) scsi_cmd.send_data;
			sg_len_orig = scsi_cmd.send_sg_len;
		} else {
			sg_len_orig = 1;
			sg_singleton.iov_base = scsi_cmd.send_data;
			sg_singleton.iov_len = trans_len;
			sg_orig = &sg_singleton;
		}
		sg = sg_orig;
		sg_len = sg_len_orig;

		offset_inc = (sess->sess_params.max_data_seg_length) ? sess->sess_params.max_data_seg_length : trans_len;

		for (offset = 0; offset < trans_len; offset += offset_inc) {
			(void) memset(&data, 0x0, sizeof(data));

			data.length = (sess->sess_params.max_data_seg_length) ? MIN(trans_len - offset, sess->sess_params.max_data_seg_length) : trans_len - offset;
			if (data.length != trans_len) {
				if (!fragment_flag) {
					if ((sg_new = iscsi_malloc_atomic(sizeof(struct iovec) * sg_len_orig)) == NULL) {
						iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
						AHS_CLEANUP;
						return -1;
					}
					fragment_flag++;
				}
				sg = sg_new;
				sg_len = sg_len_orig;
				(void) memcpy(sg, sg_orig, sizeof(struct iovec) * sg_len_orig);
				if (modify_iov(&sg, &sg_len, offset, data.length) != 0) {
					iscsi_trace_error(__FILE__, __LINE__, "modify_iov() failed\n");
					SG_CLEANUP;
					AHS_CLEANUP;
					return -1;
				}
			}
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending read data PDU (offset %u, len %u)\n", offset, data.length);
			if (offset + data.length == trans_len) {
				data.final = 1;

				if (sess->UsePhaseCollapsedRead) {
					data.status = 1;
					data.status = scsi_cmd.status;
					data.StatSN = ++(sess->StatSN);
					iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "status %#x collapsed into last data PDU\n", data.status);
				} else {
					iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "NOT collapsing status with last data PDU\n");
				}
			} else if (offset + data.length > trans_len) {
				iscsi_trace_error(__FILE__, __LINE__, "offset+data.length > trans_len??\n");
				SG_CLEANUP;
				AHS_CLEANUP;
				return -1;
			}
			data.task_tag = scsi_cmd.tag;
			data.ExpCmdSN = sess->ExpCmdSN;
			data.MaxCmdSN = sess->MaxCmdSN;
			data.DataSN = DataSN++;
			data.offset = offset;
			if (iscsi_read_data_encap(rsp_header, &data) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_read_data_encap() failed\n");
				SG_CLEANUP;
				AHS_CLEANUP;
				return -1;
			}
			if ((uint32_t)iscsi_sock_send_header_and_data(sess->sock, rsp_header, ISCSI_HEADER_LEN, sg, data.length, sg_len)
			    != ISCSI_HEADER_LEN + data.length) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
				SG_CLEANUP;
				AHS_CLEANUP;
				return -1;
			}
			scsi_cmd.bytes_sent += data.length;
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sent read data PDU ok (offset %u, len %u)\n", data.offset, data.length);
		}
		SG_CLEANUP;
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "successfully sent %u bytes read data\n", trans_len);
	}
	/*
         * Send a response PDU if
         *
         * 1) we're not using phase collapsed input (and status was good)
         * 2) we are using phase collapsed input, but there was no input data (e.g., TEST UNIT READY)
         * 3) command had non-zero status and possible sense data
         */
response:
	if (!sess->UsePhaseCollapsedRead || !scsi_cmd.length || scsi_cmd.status) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending SCSI response PDU\n");
		(void) memset(&scsi_rsp, 0x0, sizeof(scsi_rsp));
		scsi_rsp.length = scsi_cmd.status ? scsi_cmd.length : 0;
		scsi_rsp.tag = scsi_cmd.tag;
		/* If r2t send, then the StatSN is already incremented */
		if (sess->StatSN < scsi_cmd.ExpStatSN) {
			++sess->StatSN;
		}
		scsi_rsp.StatSN = sess->StatSN;
		scsi_rsp.ExpCmdSN = sess->ExpCmdSN;
		scsi_rsp.MaxCmdSN = sess->MaxCmdSN;
		scsi_rsp.ExpDataSN = (!scsi_cmd.status && scsi_cmd.input) ? DataSN : 0;
		scsi_rsp.response = 0x00;	/* iSCSI response */
		scsi_rsp.status = scsi_cmd.status;	/* SCSI status */
		if (iscsi_scsi_rsp_encap(rsp_header, &scsi_rsp) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_scsi_rsp_encap() failed\n");
			AHS_CLEANUP;
			return -1;
		}
		if ((uint32_t)iscsi_sock_send_header_and_data(sess->sock, rsp_header, ISCSI_HEADER_LEN,
		  scsi_cmd.send_data, scsi_rsp.length, scsi_cmd.send_sg_len)
		    != ISCSI_HEADER_LEN + scsi_rsp.length) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
			AHS_CLEANUP;
			return -1;
		}
		/* Make sure all data was transferred */

		if (scsi_cmd.output) {
			RETURN_NOT_EQUAL("scsi_cmd.bytes_recv", scsi_cmd.bytes_recv, scsi_cmd.trans_len, AHS_CLEANUP, -1);
			if (scsi_cmd.input) {
				RETURN_NOT_EQUAL("scsi_cmd.bytes_sent", scsi_cmd.bytes_sent, scsi_cmd.bidi_trans_len, AHS_CLEANUP, -1);
			}
		} else {
			if (scsi_cmd.input) {
				RETURN_NOT_EQUAL("scsi_cmd.bytes_sent", scsi_cmd.bytes_sent, scsi_cmd.trans_len, AHS_CLEANUP, -1);
			}
		}
	}
	/* Device callback after command has completed */

	if (cmd.callback) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "issuing device callback\n");
		if ((*cmd.callback)(cmd.callback_arg) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "device callback failed\n");
			AHS_CLEANUP;
			return -1;
		}
	}
	AHS_CLEANUP;
	return 0;
}

static int 
task_command_t(target_session_t * sess, uint8_t *header)
{
	iscsi_task_cmd_t cmd;
	iscsi_task_rsp_t rsp;
	uint8_t   rsp_header[ISCSI_HEADER_LEN];

	/* Get & check args */

	if (iscsi_task_cmd_decap(header, &cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_task_cmd_decap() failed\n");
		return -1;
	}
	if (cmd.CmdSN != sess->ExpCmdSN) {
		iscsi_trace_warning(__FILE__, __LINE__, "Expected CmdSN %d, got %d. (ignoring and resetting expectations)\n",
			      cmd.CmdSN, sess->ExpCmdSN);
		sess->ExpCmdSN = cmd.CmdSN;
	}
	sess->MaxCmdSN++;

	(void) memset(&rsp, 0x0, sizeof(rsp));
	rsp.response = ISCSI_TASK_RSP_FUNCTION_COMPLETE;

	switch (cmd.function) {
	case ISCSI_TASK_CMD_ABORT_TASK:
		printf("ISCSI_TASK_CMD_ABORT_TASK\n");
		break;
	case ISCSI_TASK_CMD_ABORT_TASK_SET:
		printf("ISCSI_TASK_CMD_ABORT_TASK_SET\n");
		break;
	case ISCSI_TASK_CMD_CLEAR_ACA:
		printf("ISCSI_TASK_CMD_CLEAR_ACA\n");
		break;
	case ISCSI_TASK_CMD_CLEAR_TASK_SET:
		printf("ISCSI_TASK_CMD_CLEAR_TASK_SET\n");
		break;
	case ISCSI_TASK_CMD_LOGICAL_UNIT_RESET:
		printf("ISCSI_TASK_CMD_LOGICAL_UNIT_RESET\n");
		break;
	case ISCSI_TASK_CMD_TARGET_WARM_RESET:
		printf("ISCSI_TASK_CMD_TARGET_WARM_RESET\n");
		break;
	case ISCSI_TASK_CMD_TARGET_COLD_RESET:
		printf("ISCSI_TASK_CMD_TARGET_COLD_RESET\n");
		break;
	case ISCSI_TASK_CMD_TARGET_REASSIGN:
		printf("ISCSI_TASK_CMD_TARGET_REASSIGN\n");
		break;
	default:
		iscsi_trace_error(__FILE__, __LINE__, "Unknown task function %d\n", cmd.function);
		rsp.response = ISCSI_TASK_RSP_REJECTED;
	}

	rsp.tag = cmd.tag;
	rsp.StatSN = ++(sess->StatSN);
	rsp.ExpCmdSN = sess->ExpCmdSN;
	rsp.MaxCmdSN = sess->MaxCmdSN;

	if (iscsi_task_rsp_encap(rsp_header, &rsp) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_task_cmd_decap() failed\n");
		return -1;
	}
	if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, rsp_header, 0) != ISCSI_HEADER_LEN) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
		return -1;

	}
	return 0;
}

static int 
nop_out_t(target_session_t * sess, uint8_t *header)
{
	iscsi_nop_out_args_t nop_out;
	char           *ping_data = NULL;

	if (iscsi_nop_out_decap(header, &nop_out) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_nop_out_decap() failed\n");
		return -1;
	}
	if (nop_out.CmdSN != sess->ExpCmdSN) {
		iscsi_trace_warning(__FILE__, __LINE__, "Expected CmdSN %d, got %d. (ignoring and resetting expectations)\n",
			      nop_out.CmdSN, sess->ExpCmdSN);
		sess->ExpCmdSN = nop_out.CmdSN;
	}
	/* TODO Clarify whether we need to update the CmdSN */
	/* sess->ExpCmdSN++;  */
	/* sess->MaxCmdSN++;  */

	if (nop_out.length) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "reading %u bytes ping data\n", nop_out.length);
		if ((ping_data = iscsi_malloc(nop_out.length)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
			return -1;
		}
		if ((uint32_t)iscsi_sock_msg(sess->sock, 0, nop_out.length, ping_data, 0) != nop_out.length) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			if (ping_data) {
				iscsi_free(ping_data);
			}
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "successfully read %u bytes ping data:\n", nop_out.length);
		iscsi_print_buffer(ping_data, nop_out.length);
	}
	if (nop_out.tag != 0xffffffff) {
		iscsi_nop_in_args_t  nop_in;
		uint8_t   rsp_header[ISCSI_HEADER_LEN];

		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending %u bytes ping response\n", nop_out.length);
		(void) memset(&nop_in, 0x0, sizeof(nop_in));
		nop_in.length = nop_out.length;
		nop_in.lun = nop_out.lun;
		nop_in.tag = nop_out.tag;
		nop_in.transfer_tag = 0xffffffff;
		nop_in.StatSN = ++(sess->StatSN);
		nop_in.ExpCmdSN = sess->ExpCmdSN;
		nop_in.MaxCmdSN = sess->MaxCmdSN;

		if (iscsi_nop_in_encap(rsp_header, &nop_in) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_nop_in_encap() failed\n");
			if (ping_data) {
				iscsi_free(ping_data);
			}
			return -1;
		}
		if ((uint32_t)iscsi_sock_send_header_and_data(sess->sock, rsp_header, ISCSI_HEADER_LEN,
						    ping_data, nop_in.length, 0) != ISCSI_HEADER_LEN + nop_in.length) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
			if (ping_data) {
				iscsi_free(ping_data);
			}
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "successfully sent %u bytes ping response\n", nop_out.length);
	}
	if (ping_data) {
		iscsi_free(ping_data);
	}
	return 0;
}

/*
 * text_command_t
 */

static int 
text_command_t(target_session_t * sess, uint8_t *header)
{
	iscsi_text_cmd_args_t	 text_cmd;
	iscsi_text_rsp_args_t	 text_rsp;
	unsigned		 len_in;
	uint32_t		 i;
	uint8_t   		 rsp_header[ISCSI_HEADER_LEN];
	targv_t			*targv;
	char           		*text_in = NULL;
	char           		*text_out = NULL;
	char			 buf[BUFSIZ];
	int			 len_out = 0;

#define TC_CLEANUP do {							\
	if (text_in != NULL) {						\
		iscsi_free_atomic(text_in);				\
	}								\
	if (text_out != NULL) {						\
		iscsi_free_atomic(text_out);				\
	}								\
} while (/* CONSTCOND */ 0)
#define TC_ERROR {							\
	TC_CLEANUP;							\
	return -1;							\
}
	/* Get text args */

	if (iscsi_text_cmd_decap(header, &text_cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_text_cmd_decap() failed\n");
		return -1;
	}
	/* Check args & update numbering */
	RETURN_NOT_EQUAL("Continue", text_cmd.cont, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("CmdSN", text_cmd.CmdSN, sess->ExpCmdSN, NO_CLEANUP, -1);

	sess->ExpCmdSN++;
	sess->MaxCmdSN++;

	if ((text_out = iscsi_malloc_atomic(2048)) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
	/* Read text parameters */

	if ((len_in = text_cmd.length) != 0) {
		iscsi_parameter_t *ptr;

		if ((text_in = iscsi_malloc_atomic(len_in + 1)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
			TC_CLEANUP;
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "reading %u bytes text parameters\n", len_in);
		if ((unsigned)iscsi_sock_msg(sess->sock, 0, len_in, text_in, 0) != len_in) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			TC_CLEANUP;
			return -1;
		}
		text_in[len_in] = 0x0;
		PARAM_TEXT_PARSE(sess->params, &sess->sess_params.cred, text_in, (int) len_in, text_out, (int *)(void *)&len_out, 2048, 0, TC_ERROR);

		/*
		 * Handle exceptional cases not covered by parameters.c
		 * (e.g., SendTargets)
		 */

		if ((ptr = param_get(sess->params, "SendTargets")) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "param_get() failed\n");
			TC_CLEANUP;
			return -1;
		}
		if (ptr->rx_offer) {
			if (ptr->offer_rx && strcmp(ptr->offer_rx, "All") == 0 && !param_equiv(sess->params, "SessionType", "Discovery")) {
				iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Rejecting SendTargets=All in a non Discovery session\n");
				PARAM_TEXT_ADD(sess->params, "SendTargets", "Reject", text_out, &len_out, 2048, 0, TC_ERROR);
			} else {
				targv = sess->globals->lunv;
				for (i = 0 ; i < targv->c ; i++) {
					if (sess->address_family == 6 ||
					    (sess->address_family == 4 && allow_netmask(targv->v[i].mask, sess->initiator))) {
						(void) get_iqn(sess, i, buf, sizeof(buf));
						PARAM_TEXT_ADD(sess->params, "TargetName", buf, text_out, &len_out, 2048, 0, TC_ERROR);
						PARAM_TEXT_ADD(sess->params, "TargetAddress", iscsi_target_getvar(sess->globals, "target address"), text_out, &len_out, 2048, 0, TC_ERROR);
					} else {
#ifdef HAVE_SYSLOG_H
						syslog(LOG_INFO, "WARNING: attempt to discover targets from %s (not allowed by %s) has been rejected", sess->initiator, targv->v[0].mask);
#endif
					}
				}
			}
			ptr->rx_offer = 0;
		}
		/* Parse outgoing offer */

		if (len_out) {
			PARAM_TEXT_PARSE(sess->params, &sess->sess_params.cred, text_out, len_out, NULL, NULL, 2048, 1, TC_ERROR);
		}
	}
	if (sess->IsFullFeature) {
		set_session_parameters(sess->params, &sess->sess_params);
	}
	/* Send response */

	text_rsp.final = text_cmd.final;
	text_rsp.cont = 0;
	text_rsp.length = len_out;
	text_rsp.lun = text_cmd.lun;
	text_rsp.tag = text_cmd.tag;
	text_rsp.transfer_tag = (text_rsp.final) ? 0xffffffff : 0x1234;
	text_rsp.StatSN = ++(sess->StatSN);
	text_rsp.ExpCmdSN = sess->ExpCmdSN;
	text_rsp.MaxCmdSN = sess->MaxCmdSN;
	if (iscsi_text_rsp_encap(rsp_header, &text_rsp) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_text_rsp_encap() failed\n");
		TC_CLEANUP;
		return -1;
	}
	if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, rsp_header, 0) != ISCSI_HEADER_LEN) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
		TC_CLEANUP;
		return -1;
	}
	if (len_out) {
		if (iscsi_sock_msg(sess->sock, 1, (unsigned) len_out, text_out, 0) != len_out) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			TC_CLEANUP;
			return -1;
		}
	}
	TC_CLEANUP;
	return 0;
}

/* given a target's iqn, find the relevant target that we're exporting */
int
find_target_iqn(target_session_t *sess)
{
	uint32_t	 i;
	targv_t		*targv;
	char		 buf[BUFSIZ];

	targv = sess->globals->lunv;
	for (i = 0 ; i < targv->c ; i++) {
		if (param_equiv(sess->params, "TargetName",
				get_iqn(sess, i, buf, sizeof(buf)))) {
			return sess->d = i;
		}
	}
	return -1;
}

/* given a tsih, find the relevant target that we're exporting */
int
find_target_tsih(iscsi_target_t *globals, int tsih)
{
	uint32_t	 i;
	targv_t		*targv;

	targv = globals->lunv;
	for (i = 0 ; i < targv->c ; i++) {
		if (targv->v[i].tsih == tsih) {
			return i;
		}
	}
	return -1;
}

/*
 * login_command_t() handles login requests and replies.
 */

static int 
login_command_t(target_session_t * sess, uint8_t *header)
{
	iscsi_login_cmd_args_t cmd;
	iscsi_login_rsp_args_t rsp;
	uint8_t		rsp_header[ISCSI_HEADER_LEN];
	targv_t		*targv;
	char           *text_in = NULL;
	char           *text_out = NULL;
	char		logbuf[BUFSIZ];
	int             len_in = 0;
	int             len_out = 0;
	int             status = 0;
	int		i;

	/* Initialize response */

#define LC_CLEANUP do {							\
	if (text_in != NULL) {						\
		iscsi_free_atomic(text_in);				\
	}								\
	if (text_out != NULL) {						\
		iscsi_free_atomic(text_out);				\
	}								\
} while (/* CONSTCOND */ 0)
#define LC_ERROR {							\
	TC_CLEANUP;							\
	return -1;							\
}

	(void) memset(&rsp, 0x0, sizeof(rsp));
	rsp.status_class = ISCSI_LOGIN_STATUS_INITIATOR_ERROR;

	/* Get login args & check preconditions */

	if (iscsi_login_cmd_decap(header, &cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_login_cmd_decap() failed\n");
		goto response;
	}
	if (sess->IsLoggedIn) {
		iscsi_trace_error(__FILE__, __LINE__, "duplicate login attempt on sess %d\n", sess->id);
		goto response;
	}
	if ((cmd.cont != 0) && (cmd.transit != 0)) {
		iscsi_trace_error(__FILE__, __LINE__, "Bad cmd.continue.  Expected 0.\n");
		goto response;
	} else if ((cmd.version_max < ISCSI_VERSION) || (cmd.version_min > ISCSI_VERSION)) {
		iscsi_trace_error(__FILE__, __LINE__, "Target iscsi version (%u) not supported by initiator [Max Ver (%u) and Min Ver (%u)]\n", ISCSI_VERSION, cmd.version_max, cmd.version_min);
		rsp.status_class = ISCSI_LOGIN_STATUS_INITIATOR_ERROR;
		rsp.status_detail = ISCSI_LOGIN_DETAIL_VERSION_NOT_SUPPORTED;
		rsp.version_max = ISCSI_VERSION;
		rsp.version_active = ISCSI_VERSION;
		goto response;
	} else if (cmd.tsih != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "Bad cmd.tsih (%u). Expected 0.\n", cmd.tsih);
		goto response;
	}
	/* Parse text parameters and build response */

	if ((text_out = iscsi_malloc_atomic(2048)) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
	if ((len_in = cmd.length) != 0) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "reading %d bytes text data\n", len_in);
		if ((text_in = iscsi_malloc_atomic((unsigned)(len_in + 1))) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
			LC_CLEANUP;
			return -1;
		}
		if (iscsi_sock_msg(sess->sock, 0, (unsigned) len_in, text_in, 0) != len_in) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			LC_CLEANUP;
			return -1;
		}
		text_in[len_in] = 0x0;
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "successfully read %d bytes text data\n", len_in);

		/*
		 * Parse incoming parameters (text_out will contain the
		 * response we need
		 */

		/* to send back to the initiator */


		if ((status = param_text_parse(sess->params, &sess->sess_params.cred, text_in, len_in, text_out, &len_out, 2048, 0)) != 0) {
			switch (status) {
			case ISCSI_PARAM_STATUS_FAILED:
				rsp.status_detail = ISCSI_LOGIN_DETAIL_SUCCESS;
				break;
			case ISCSI_PARAM_STATUS_AUTH_FAILED:
				rsp.status_detail = ISCSI_LOGIN_DETAIL_INIT_AUTH_FAILURE;
				break;
			default:
				/*
		                 * We will need to set the detail field based on more detailed error
		                 * cases. Will need to fix this if compliciance test break
		                 * (status_detail field).
		                 */
				break;
			}
			goto response;
		}
		/* Parse the outgoing offer */
		if (!sess->LoginStarted) {
			PARAM_TEXT_ADD(sess->params, "TargetPortalGroupTag", "1", text_out, &len_out, 2048, 0, LC_ERROR);
		}
		if (len_out) {
			PARAM_TEXT_PARSE(sess->params, &sess->sess_params.cred, text_out, len_out, NULL, NULL, 2048, 1, LC_ERROR;
			);
		}
	}
	if (!sess->LoginStarted) {
		sess->LoginStarted = 1;
	}
	/*
	 * For now, we accept what ever the initiators' current and next
	 * states are. And le are always
	 */
	/* ready to transitition to that state. */

	rsp.csg = cmd.csg;
	rsp.nsg = cmd.nsg;
	rsp.transit = cmd.transit;

	if (cmd.csg == ISCSI_LOGIN_STAGE_SECURITY) {
		if (param_equiv(sess->params, "AuthResult", "No")) {
			rsp.transit = 0;
		} else if (param_equiv(sess->params, "AuthResult", "Fail")) {
			rsp.status_class = rsp.status_detail = ISCSI_LOGIN_DETAIL_INIT_AUTH_FAILURE;
			goto response;
		}
	}
	if (cmd.transit && cmd.nsg == ISCSI_LOGIN_STAGE_FULL_FEATURE) {

		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "transitioning to ISCSI_LOGIN_STAGE_FULL_FEATURE\n");

		/* Check post conditions */

		if (param_equiv(sess->params, "InitiatorName", "")) {
			iscsi_trace_error(__FILE__, __LINE__, "InitiatorName not specified\n");
			goto response;
		}
		if (param_equiv(sess->params, "SessionType", "Normal")) {
			if (param_equiv(sess->params, "TargetName", "")) {
				iscsi_trace_error(__FILE__, __LINE__, "TargetName not specified\n");
				goto response;
			}
			if ((i = find_target_iqn(sess)) < 0) { 
				iscsi_trace_error(__FILE__, __LINE__, "Bad TargetName \"%s\"\n", param_val(sess->params, "TargetName"));
				goto response;
			}
			if (cmd.tsih != 0 && find_target_tsih(sess->globals, cmd.tsih) != i) {
				targv = sess->globals->lunv;
				iscsi_trace_error(__FILE__, __LINE__, "target tsih expected %d, cmd.tsih %d, i %d\n", targv->v[i].tsih, cmd.tsih, i);
			}
			sess->d = i;
		} else if ((i = find_target_tsih(sess->globals, cmd.tsih)) < 0) {
			iscsi_trace_error(__FILE__, __LINE__, "Abnormal SessionType cmd.tsih %d not found\n", cmd.tsih);
			i = sess->d;
		}
		if (param_equiv(sess->params, "SessionType", "")) {
			iscsi_trace_error(__FILE__, __LINE__, "SessionType not specified\n");
			goto response;
		}
		sess->ExpCmdSN = sess->MaxCmdSN = cmd.CmdSN;
		sess->cid = cmd.cid;
		sess->isid = cmd.isid;

		targv = sess->globals->lunv;
		targv->v[i].tsih = sess->tsih = ++sess->globals->last_tsih;
		sess->IsFullFeature = 1;

		sess->IsLoggedIn = 1;
		if (!param_equiv(sess->params, "SessionType", "Discovery")) {
			(void) strlcpy(param_val(sess->params, "MaxConnections")
					, "1", 2);
		}
		set_session_parameters(sess->params, &sess->sess_params);
	} else {
		if ((i = find_target_tsih(sess->globals, cmd.tsih)) < 0) {
			iscsi_trace_error(__FILE__, __LINE__,
				"cmd.tsih %d not found\n", cmd.tsih);
		}
	}

	/* No errors */
	rsp.status_class = rsp.status_detail = ISCSI_LOGIN_DETAIL_SUCCESS;
	rsp.length = len_out;

	/* Send login response */
response:
	sess->ExpCmdSN = sess->MaxCmdSN = cmd.CmdSN;
	rsp.isid = cmd.isid;
	rsp.StatSN = cmd.ExpStatSN;	/* debug  */
	rsp.tag = cmd.tag;
	rsp.cont = cmd.cont;
	rsp.ExpCmdSN = sess->ExpCmdSN;
	rsp.MaxCmdSN = sess->MaxCmdSN;
	if (!rsp.status_class) {
		if (rsp.transit && (rsp.nsg == ISCSI_LOGIN_STAGE_FULL_FEATURE)) {
			rsp.version_max = ISCSI_VERSION;
			rsp.version_active = ISCSI_VERSION;
			rsp.StatSN = ++(sess->StatSN);
			rsp.tsih = sess->tsih;
		}
	}
	if (iscsi_login_rsp_encap(rsp_header, &rsp) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_login_rsp_encap() failed\n");
		LC_CLEANUP;
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending login response\n");
	if ((uint32_t)iscsi_sock_send_header_and_data(sess->sock, rsp_header, ISCSI_HEADER_LEN,
		text_out, rsp.length, 0) != ISCSI_HEADER_LEN + rsp.length) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
		LC_CLEANUP;
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sent login response ok\n");
	if (rsp.status_class != 0) {
		LC_CLEANUP;
		return -1;
	}
	if (cmd.transit && cmd.nsg == ISCSI_LOGIN_STAGE_FULL_FEATURE) {

		/* log information to stdout */
		(void) snprintf(logbuf, sizeof(logbuf),
			"> iSCSI %s login  successful from %s on %s disk %d, ISID %" PRIu64 ", TSIH %u",
			param_val(sess->params, "SessionType"),
			param_val(sess->params, "InitiatorName"),
			sess->initiator,
			sess->d,
			sess->isid,
			sess->tsih);
		printf("%s\n", logbuf);
#ifdef HAVE_SYSLOG_H
		/* log information to syslog */
		syslog(LOG_INFO, "%s", logbuf);
#endif

		/* Buffer for data xfers to/from the scsi device */
		if (!param_equiv(sess->params, "MaxRecvDataSegmentLength", "0")) {
			if ((sess->buff = iscsi_malloc((unsigned)(param_atoi(sess->params, "MaxRecvDataSegmentLength")))) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
				LC_CLEANUP;
				return -1;
			}
		} else {
			iscsi_trace_error(__FILE__, __LINE__, "MaxRecvDataSegmentLength of 0 not supported\n");
			LC_CLEANUP;
			return -1;
		}
	}
	LC_CLEANUP;
	return 0;
}

static int 
logout_command_t(target_session_t * sess, uint8_t *header)
{
	iscsi_logout_cmd_args_t	cmd;
	iscsi_logout_rsp_args_t	rsp;
	targv_t			*targv;
	uint8_t			rsp_header[ISCSI_HEADER_LEN];
	char			logbuf[BUFSIZ];
	int			i;

	(void) memset(&rsp, 0x0, sizeof(rsp));
	if (iscsi_logout_cmd_decap(header, &cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_logout_cmd_decap() failed\n");
		return -1;
	}
	sess->StatSN = cmd.ExpStatSN;
	if ((cmd.reason == ISCSI_LOGOUT_CLOSE_RECOVERY) && (param_equiv(sess->params, "ErrorRecoveryLevel", "0"))) {
		rsp.response = ISCSI_LOGOUT_STATUS_NO_RECOVERY;
	}
	RETURN_NOT_EQUAL("CmdSN", cmd.CmdSN, sess->ExpCmdSN, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("ExpStatSN", cmd.ExpStatSN, sess->StatSN, NO_CLEANUP, -1);

	rsp.tag = cmd.tag;
	rsp.StatSN = sess->StatSN;
	rsp.ExpCmdSN = ++sess->ExpCmdSN;
	rsp.MaxCmdSN = sess->MaxCmdSN;
	if (iscsi_logout_rsp_encap(rsp_header, &rsp) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_logout_rsp_encap() failed\n");
		return -1;
	}
	if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, rsp_header, 0) != ISCSI_HEADER_LEN) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sent logout response OK\n");

	/* log information to stdout */
	(void) snprintf(logbuf, sizeof(logbuf), 
		"< iSCSI %s logout successful from %s on %s disk %d, ISID %" PRIu64 ", TSIH %u",
		param_val(sess->params, "SessionType"),
		param_val(sess->params, "InitiatorName"),
		sess->initiator,
		sess->d,
		sess->isid,
		sess->tsih);
	printf("%s\n", logbuf);
#ifdef HAVE_SYSLOG
	/* log information to syslog */
	syslog(LOG_INFO, "%s", logbuf);
#endif

	sess->IsLoggedIn = 0;

	if (sess->sess_params.cred.user) {
		free(sess->sess_params.cred.user);
		sess->sess_params.cred.user = NULL;
	}

	if ((i = find_target_tsih(sess->globals, sess->tsih)) < 0) {
		iscsi_trace_error(__FILE__, __LINE__, "logout sess->tsih %d not found\n", sess->tsih);
	} else {
		targv = sess->globals->lunv;
		targv->v[i].tsih = 0;
	}
	sess->tsih = 0;

	return 0;
}

static int 
verify_cmd_t(target_session_t * sess, uint8_t *header)
{
	int             op = ISCSI_OPCODE(header);

	if ((!sess->LoginStarted) && (op != ISCSI_LOGIN_CMD)) {
		/* Terminate the connection */
		iscsi_trace_error(__FILE__, __LINE__, "session %d: iSCSI op %#x attempted before LOGIN PHASE\n",
			    sess->id, op);
		return -1;
	}
	if (!sess->IsFullFeature && ((op != ISCSI_LOGIN_CMD) && (op != ISCSI_LOGOUT_CMD))) {
		iscsi_login_rsp_args_t rsp;
		uint8_t   rsp_header[ISCSI_HEADER_LEN];
		iscsi_trace_error(__FILE__, __LINE__, "session %d: iSCSI op %#x attempted before FULL FEATURE\n",
			    sess->id, op);
		/* Create Login Reject response */
		(void) memset(&rsp, 0x0, sizeof(rsp));
		rsp.status_class = ISCSI_LOGIN_STATUS_INITIATOR_ERROR;
		rsp.status_detail = ISCSI_LOGIN_DETAIL_NOT_LOGGED_IN;
		rsp.version_max = ISCSI_VERSION;
		rsp.version_active = ISCSI_VERSION;

		if (iscsi_login_rsp_encap(rsp_header, &rsp) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_login_rsp_encap() failed\n");
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending login response\n");
		if ((uint32_t)iscsi_sock_send_header_and_data(sess->sock, rsp_header,
			 ISCSI_HEADER_LEN, NULL, 0, 0) != ISCSI_HEADER_LEN +
		    rsp.length) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sent login response ok\n");
		return -1;
	}
	return 0;
}

/*
 * this function looks at the opcode in the received header for the session,
 * and does a switch on the opcode to call the required function.
 */
static int 
execute_t(target_session_t *sess, uint8_t *header)
{
	int             op = ISCSI_OPCODE(header);

	if (verify_cmd_t(sess, header) != 0) {
		return -1;
	}
	switch (op) {
	case ISCSI_TASK_CMD:
		iscsi_trace(TRACE_ISCSI_CMD, __FILE__, __LINE__, "session %d: Task Command\n", sess->id);
		if (task_command_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "task_command_t() failed\n");
			return -1;
		}
		break;

	case ISCSI_NOP_OUT:
		iscsi_trace(TRACE_ISCSI_CMD, __FILE__, __LINE__, "session %d: NOP-Out\n", sess->id);
		if (nop_out_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "nop_out_t() failed\n");
			return -1;
		}
		break;

	case ISCSI_LOGIN_CMD:
		iscsi_trace(TRACE_ISCSI_CMD, __FILE__, __LINE__, "session %d: Login Command\n", sess->id);
		if (login_command_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "login_command_t() failed\n");
			return -1;
		}
		break;

	case ISCSI_TEXT_CMD:
		iscsi_trace(TRACE_ISCSI_CMD, __FILE__, __LINE__, "session %d: Text Command\n", sess->id);
		if (text_command_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "text_command_t() failed\n");
			return -1;
		}
		break;

	case ISCSI_LOGOUT_CMD:
		iscsi_trace(TRACE_ISCSI_CMD, __FILE__, __LINE__, "session %d: Logout Command\n", sess->id);
		if (logout_command_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "logout_command_t() failed\n");
			return -1;
		}
		break;

	case ISCSI_SCSI_CMD:
		iscsi_trace(TRACE_ISCSI_CMD, __FILE__, __LINE__, "session %d: SCSI Command\n", sess->id);
		if (scsi_command_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "scsi_command_t() failed\n");
			return -1;
		}
		break;

	default:
		iscsi_trace_error(__FILE__, __LINE__, "Unknown Opcode %#x\n", ISCSI_OPCODE(header));
		if (reject_t(sess, header, 0x04) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "reject_t() failed\n");
			return -1;
		}
		break;
	}
	return 0;
}

/*
 * Currently one thread per session, used for both Rx and Tx.
 */

static int 
worker_proc_t(void *arg)
{
	target_session_t *sess = (target_session_t *) arg;
	uint8_t   header[ISCSI_HEADER_LEN];
	iscsi_parameter_t **l = &sess->params;

	ISCSI_THREAD_START("worker_thread");
	sess->worker.pid = getpid();
	sess->worker.state |= ISCSI_WORKER_STATE_STARTED;
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: started\n", sess->id);

	/*
         * ISCSI_PARAM_TYPE_LIST format:        <type> <key> <dflt> <valid list values>
         * ISCSI_PARAM_TYPE_BINARY format:      <type> <key> <dflt> <valid binary values>
         * ISCSI_PARAM_TYPE_NUMERICAL format:   <type> <key> <dflt> <max>
         * ISCSI_PARAM_TYPE_DECLARATIVE format: <type> <key> <dflt> ""
         */

	sess->params = NULL;
	l = &sess->params;

	/* CHAP Parameters */
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "AuthMethod", "CHAP", "CHAP,None", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "CHAP_A", "None", "5", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_N", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_R", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_I", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_C", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "TargetPortalGroupTag", "1", "1", return -1);
	/* CHAP Parameters */
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "HeaderDigest", "None", "None", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "DataDigest", "None", "None", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "MaxConnections", "1", "1", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "SendTargets", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "TargetName", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "InitiatorName", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "TargetAlias", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "InitiatorAlias", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "TargetAddress", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_OR, "InitialR2T", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_AND, "OFMarker", "No", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_AND, "IFMarker", "No", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "OFMarkInt", "1", "65536", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "IFMarkInt", "1", "65536", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_AND, "ImmediateData", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "MaxRecvDataSegmentLength", "8192", "16777215", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "MaxBurstLength", "262144", "16777215", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "FirstBurstLength", "65536", "16777215", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "DefaultTime2Wait", "2", "2", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "DefaultTime2Retain", "20", "20", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "MaxOutstandingR2T", "1", "1", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_OR, "DataPDUInOrder", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_OR, "DataSequenceInOrder", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "ErrorRecoveryLevel", "0", "0", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "SessionType", "Normal", "Normal,Discovery", return -1);
	/*
	 * Auth Result is not in specs, we use this key to pass
	 * authentication result
	 */
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "AuthResult", "No", "Yes,No,Fail", return -1);

	/* Set remaining session parameters  */

	sess->UsePhaseCollapsedRead = ISCSI_USE_PHASE_COLLAPSED_READ_DFLT;

	/* Loop for commands */

	while (sess->globals->state != TARGET_SHUT_DOWN) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: reading header\n", sess->id);
		if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: iscsi_sock_msg() failed\n", sess->id);
			break;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: iscsi op %#x\n", sess->id, ISCSI_OPCODE(header));
		if (execute_t(sess, header) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "execute_t() failed\n");
			break;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: iscsi op %#x complete\n", sess->id, ISCSI_OPCODE(header));
		if (ISCSI_OPCODE(header) == ISCSI_LOGOUT_CMD) {
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: logout received, ending session\n", sess->id);
			break;
		}
	}

	/* Clean up */

	iscsi_free(sess->buff);
	if (param_list_destroy(sess->params) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "param_list_destroy() failed\n");
		return -1;
	}
	/* Terminate connection */

	if (iscsi_sock_close(sess->sock) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_close() failed\n");
	}
	/* Make session available */

	ISCSI_LOCK(&g_session_q_mutex, return -1);
	(void) memset(sess, 0x0, sizeof(*sess));
	if (iscsi_queue_insert(&g_session_q, sess) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
		return -1;
	}
	ISCSI_UNLOCK(&g_session_q_mutex, return -1);
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "session %d: ended\n", sess->id);

	return 0;
}

static int 
read_data_pdu(target_session_t * sess,
	      iscsi_write_data_t * data,
	      iscsi_scsi_cmd_args_t * args)
{
	uint8_t   header[ISCSI_HEADER_LEN];
	int             ret_val = -1;

	if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
		return -1;
	}
	if ((ret_val = iscsi_write_data_decap(header, data)) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_write_data_decap() failed\n");
		return ret_val;
	}
	/* Check args */
	if (sess->sess_params.max_data_seg_length) {
		if (data->length > sess->sess_params.max_data_seg_length) {
			args->status = 0x02;
			return -1;
		}
	}
	if ((args->bytes_recv + data->length) > args->trans_len) {
		args->status = 0x02;
		return -1;
	}
	if (data->tag != args->tag) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Data ITT (%d) does not match with command ITT (%d)\n", data->tag, args->tag);
		if (data->final) {
			args->status = 0x02;
			return -1;
		} else {
			/* Send a reject PDU */
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Sending Reject PDU\n");
			if (reject_t(sess, header, 0x09) != 0) {	/* Invalid PDU Field */
				iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Sending Reject PDU failed\n");
				return 1;
			}
		}
	}
	return 0;
}

int 
target_transfer_data(target_session_t * sess, iscsi_scsi_cmd_args_t * args, struct iovec * sg, int sg_len)
{
	iscsi_write_data_t data;
	struct iovec   *iov, *iov_ptr = NULL;
	int             iov_len;

#define TTD_CLEANUP do {						\
	if (iov_ptr != NULL) {						\
		iscsi_free_atomic(iov_ptr);				\
	}								\
} while (/* CONSTCOND */ 0)

	args->bytes_recv = 0;
	if ((!sess->sess_params.immediate_data) && args->length) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Cannot accept any Immediate data\n");
		args->status = 0x02;
		return -1;
	}
	/* Make a copy of the iovec */

	if ((iov_ptr = iscsi_malloc_atomic(sizeof(struct iovec) * sg_len)) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
	iov = iov_ptr;
	(void) memcpy(iov, sg, sizeof(struct iovec) * sg_len);
	iov_len = sg_len;

	/*
         * Read any immediate data.
         */

	if (sess->sess_params.immediate_data && args->length) {
		if (sess->sess_params.max_data_seg_length) {
			RETURN_GREATER("args->length", args->length, sess->sess_params.max_data_seg_length, TTD_CLEANUP, -1);
		}
		/* Modify iov to include just immediate data */

		if (modify_iov(&iov, &iov_len, 0, args->length) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "modify_iov() failed\n");
			TTD_CLEANUP;
			return -1;
		}
		iscsi_trace(TRACE_SCSI_DATA, __FILE__, __LINE__, "reading %u bytes immediate write data\n", args->length);
		if ((uint32_t)iscsi_sock_msg(sess->sock, 0, args->length, iov, iov_len) != args->length) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			TTD_CLEANUP;
			return -1;
		}
		iscsi_trace(TRACE_SCSI_DATA, __FILE__, __LINE__, "successfully read %u bytes immediate write data\n", args->length);
		args->bytes_recv += args->length;
	}
	/*
         * Read iSCSI data PDUs
         */

	if (args->bytes_recv < args->trans_len) {
		int             r2t_flag = 0;
		int             read_status = 0;
		iscsi_r2t_t     r2t;
		int             desired_xfer_len = MIN(sess->sess_params.first_burst_length,
				      args->trans_len) - args->bytes_recv;

		(void) memset(&r2t, 0x0, sizeof(r2t));
		do {

			/*
			 * Send R2T if we're either operating in solicted
			 * mode or we're operating in unsolicted
			 */
			/* mode and have reached the first burst */
			if (!r2t_flag && (sess->sess_params.initial_r2t ||
				    (sess->sess_params.first_burst_length &&
				     (args->bytes_recv >= sess->sess_params.first_burst_length)))) {
				uint8_t   header[ISCSI_HEADER_LEN];

				desired_xfer_len = MIN((args->trans_len - args->bytes_recv),
					sess->sess_params.max_burst_length);
				iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending R2T for %u bytes data\n", desired_xfer_len);
				r2t.tag = args->tag;

				r2t.transfer_tag = 0x1234;

				r2t.ExpCmdSN = sess->ExpCmdSN;
				r2t.MaxCmdSN = sess->MaxCmdSN;
				r2t.StatSN = ++(sess->StatSN);
				r2t.length = desired_xfer_len;
				r2t.offset = args->bytes_recv;
				if (iscsi_r2t_encap(header, &r2t) != 0) {
					iscsi_trace_error(__FILE__, __LINE__, "r2t_encap() failed\n");
					TTD_CLEANUP;
					return -1;
				}
				iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "sending R2T tag %u transfer tag %u len %u offset %u\n",
				      r2t.tag, r2t.transfer_tag, r2t.length, r2t.offset);
				if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
					iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
					TTD_CLEANUP;
					return -1;
				}
				r2t_flag = 1;
				r2t.R2TSN += 1;
			}
			/* Read iSCSI data PDU */

			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "reading data pdu\n");
			if ((read_status = read_data_pdu(sess, &data, args)) != 0) {
				if (read_status == 1) {
					iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "Unknown PDU received and ignored.  Expecting Data PDU\n");
					continue;
				} else {
					iscsi_trace_error(__FILE__, __LINE__, "read_data_pdu() failed\n");
					args->status = 0x02;
					TTD_CLEANUP;
					return -1;
				}
			}
			WARN_NOT_EQUAL("ExpStatSN", data.ExpStatSN, sess->StatSN);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "read data pdu OK (offset %u, length %u)\n", data.offset, data.length);

			/* Modify iov with offset and length. */

			iov = iov_ptr;
			(void) memcpy(iov, sg, sizeof(struct iovec) * sg_len);
			iov_len = sg_len;
			if (modify_iov(&iov, &iov_len, data.offset, data.length) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "modify_iov() failed\n");
				TTD_CLEANUP;
				return -1;
			}
			/* Scatter into destination buffers */

			if ((uint32_t)iscsi_sock_msg(sess->sock, 0, data.length, iov, iov_len) != data.length) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
				TTD_CLEANUP;
				return -1;
			}
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "successfully scattered %u bytes\n", data.length);
			args->bytes_recv += data.length;
			desired_xfer_len -= data.length;
			if ((!r2t_flag) && (args->bytes_recv > sess->sess_params.first_burst_length)) {
				iscsi_trace_error(__FILE__, __LINE__, "Received unsolicited data (%u) more than first_burst_length (%u)\n", args->bytes_recv, sess->sess_params.first_burst_length);
				args->status = 0x02;
				TTD_CLEANUP;
				return -1;
			}
			if ((desired_xfer_len != 0) && data.final) {
				iscsi_trace_error(__FILE__, __LINE__, "Expecting more data (%d) from initiator for this sequence\n", desired_xfer_len);
				args->status = 0x02;
				TTD_CLEANUP;
				return -1;
			}
			if ((desired_xfer_len == 0) && !data.final) {
				iscsi_trace_error(__FILE__, __LINE__, "Final bit not set on the last data PDU of this sequence\n");
				args->status = 0x02;
				TTD_CLEANUP;
				return -1;
			}
			if ((desired_xfer_len == 0) && (args->bytes_recv < args->trans_len)) {
				r2t_flag = 0;
			}
		} while (args->bytes_recv < args->trans_len);
		RETURN_NOT_EQUAL("Final bit", data.final, 1, TTD_CLEANUP, -1);
	} else {
		RETURN_NOT_EQUAL("Final bit", args->final, 1, TTD_CLEANUP, -1);
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "successfully transferred %u bytes write data\n", args->trans_len);
	TTD_CLEANUP;
	return 0;
}

/* check there's enough space in the arrays */
static void
size_arrays(iscsi_target_t *tgt, unsigned needed)
{
	if (tgt->size == 0) {
		/* only get here first time around */
		tgt->size = needed;
		tgt->name = calloc(sizeof(char *), needed);
		tgt->value = calloc(sizeof(char *), needed);
	} else if (tgt->c == tgt->size) {
		/* only uses 'needed' when filled array */
		tgt->size += needed;
		tgt->name = realloc(tgt->name, sizeof(char *) * needed);
		tgt->value = realloc(tgt->value, sizeof(char *) * needed);
	}
}

/* find the name in the array */
static int
findvar(iscsi_target_t *tgt, const char *name)
{
	unsigned	i;

	for (i = 0 ; i < tgt->c && strcmp(tgt->name[i], name) != 0; i++) {
	}
	return (i == tgt->c) ? -1 : (int)i;
}

/********************
 * Public Functions *
 ********************/

int
iscsi_target_set_defaults(iscsi_target_t *tgt)
{
	char	buf[32];

	/* set defaults */
	(void) memset(tgt, 0x0, sizeof(*tgt));
	iscsi_target_setvar(tgt, "iqn", DEFAULT_TARGET_NAME);
	(void) snprintf(buf, sizeof(buf), "%d", ISCSI_PORT);
	iscsi_target_setvar(tgt, "target port", buf);
	iscsi_target_setvar(tgt, "address family", "unspec");
	(void) snprintf(buf, sizeof(buf), "%d", DEFAULT_TARGET_MAX_SESSIONS);
	iscsi_target_setvar(tgt, "max sessions", buf);
	iscsi_target_setvar(tgt, "configfile", _PATH_ISCSI_TARGETS);
	iscsi_target_setvar(tgt, "blocklen", "512");
	return 1;
}

int 
iscsi_target_start(iscsi_target_t *gp)
{
	uint32_t	 j;
	targv_t		*lunv;
	char		*config;
	char		*dbg;
	int		 maxsessions;
	int              i;

	if ((dbg = iscsi_target_getvar(gp, "debug")) != NULL) {
		set_debug(dbg);
	}
	/* allocate space for disks, extents and targets */
	NEW(targv_t, gp->lunv, "iscsi_target_start 1", return -1);
	NEW(devv_t, gp->devv, "iscsi_target_start 2", return -1);
	NEW(extv_t, gp->extentv, "iscsi_target_start 3", return -1);
	/* read the configuration file */
	config = iscsi_target_getvar(gp, "configfile");
	if (!read_conf_file(config, gp->lunv, gp->devv, gp->extentv)) {
		(void) fprintf(stderr, "Error: can't open `%s'\n", config);
		return 0;
	}
	lunv = gp->lunv;
	if (lunv->c == 0) {
		(void) fprintf(stderr, "No targets to initialise\n");
		return -1;
	}
	maxsessions = atoi(iscsi_target_getvar(gp, "max sessions"));
	NEWARRAY(target_session_t, g_session, maxsessions, "iscsi_target_start",
			return -1);
	device_set_var("blocklen", iscsi_target_getvar(gp, "blocklen"));
	if (gp->state == TARGET_INITIALIZING ||
	    gp->state == TARGET_INITIALIZED) {
		iscsi_trace_error(__FILE__, __LINE__, "duplicate target initialization attempted\n");
		return -1;
	}
	gp->state = TARGET_INITIALIZING;
	if (iscsi_queue_init(&g_session_q, maxsessions) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		return -1;
	}
	gp->main_pid = getpid();
	for (i = 0; i < maxsessions; i++) {
		g_session[i].id = i;
		if (iscsi_queue_insert(&g_session_q, &g_session[i]) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
			return -1;
		}
	}
	for (j = 0 ; j < lunv->c ; j++) {
		if ((g_session[j].d  = device_init(gp, lunv, &lunv->v[j])) < 0) {
			iscsi_trace_error(__FILE__, __LINE__, "device_init() failed\n");
			return -1;
		}
	}
	ISCSI_MUTEX_INIT(&g_session_q_mutex, return -1);
	gp->listener_listening = 0;
	gp->listener_pid = -1;
	gp->state = TARGET_INITIALIZED;

	printf("TARGET: iSCSI Qualified Name (IQN) is %s\n",
			iscsi_target_getvar(gp, "iqn"));
	for (i = 0 ; i < gp->sockc ; i++) {
		printf("\tsocket %d listening on port %s\n", gp->sockv[i],
			iscsi_target_getvar(gp, "target port"));
	}

	return 0;
}

int 
iscsi_target_shutdown(iscsi_target_t *gp)
{
	target_session_t	*sess;
	int			 maxsessions;
	int			 i;

	if ((gp->state == TARGET_SHUTTING_DOWN) || (gp->state == TARGET_SHUT_DOWN)) {
		iscsi_trace_error(__FILE__, __LINE__, "duplicate target shutdown attempted\n");
		return -1;
	}
	gp->state = TARGET_SHUTTING_DOWN;
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "shutting down target\n");
	maxsessions = atoi(iscsi_target_getvar(gp, "max sessions"));
	for (i = 0; i < maxsessions; i++) {
		sess = &g_session[i];

		/* Need to replace with a call to session_destroy() */

		if (sess->IsLoggedIn) {
			printf("shutting down socket on sess %d\n", i);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "shutting down socket on sess %d\n", i);
			if (iscsi_sock_shutdown(sess->sock, 2) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_shutdown() failed\n");
				return -1;
			}
			printf("waiting for worker %d (pid %d, state %d)\n", i, sess->worker.pid, sess->worker.state);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "waiting for worker %d (pid %d, state %d)\n",
			      i, sess->worker.pid, sess->worker.state);
			while (sess->worker.state & ISCSI_WORKER_STATE_STARTED) {
				ISCSI_SPIN;
			}
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "worker %d has exited\n", i);
		}
		if (device_shutdown(sess) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "device_shutdown() failed\n");
			return -1;
		}
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "shutting down accept socket\n");
	if (iscsi_sock_shutdown(gp->sockv[0], 2) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_shutdown() failed\n");
		return -1;
	}
	if (gp->listener_pid != getpid()) {
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "waiting for listener thread\n");
		while (gp->listener_listening)
			ISCSI_SPIN;
		iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "listener thread has exited\n");
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "closing accept socket\n");
	if (iscsi_sock_close(gp->sockv[0]) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_close() failed\n");
		return -1;
	}
	ISCSI_MUTEX_DESTROY(&g_session_q_mutex, return -1);
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "target shutdown complete\n");
	gp->state = TARGET_SHUT_DOWN;

	return 0;
}

int 
iscsi_target_listen(iscsi_target_t *gp)
{
	struct sockaddr_in6	remoteAddrStorage6;
	struct sockaddr_in6	localAddrStorage6;
	struct sockaddr_in	remoteAddrStorage;
	struct sockaddr_in	localAddrStorage;
	target_session_t	*sess;
	socklen_t		remoteAddrLen;
	socklen_t		localAddrLen;
	char			targetaddress[1024];
	char			remote[1024];
	char			local[1024];
	char			*config;
	int			newconn;
	int			i;

	ISCSI_THREAD_START("listen_thread");
	gp->listener_pid = getpid();
	gp->listener_listening++;
	iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "listener thread started\n");

	if (!iscsi_socks_establish(gp->sockv, gp->famv, &gp->sockc,
			iscsi_target_getvar(gp, "address family"),
			atoi(iscsi_target_getvar(gp, "target port")))) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_establish() failed\n");
		goto done;
	}

	iscsi_trace(TRACE_NET_DEBUG, __FILE__, __LINE__, "create, bind, listen OK\n");

	/* Loop for connections: FIX ME with queue */

	while (gp->state != TARGET_SHUT_DOWN) {
		ISCSI_LOCK(&g_session_q_mutex, return -1);
		if ((sess = iscsi_queue_remove(&g_session_q)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__,
			"no free sessions: iscsi_queue_remove() failed\n");
			goto done;
		}
		ISCSI_UNLOCK(&g_session_q_mutex, return -1);
#if 0
		(void) memset(sess, 0x0, sizeof(*sess));
#endif

		sess->globals = gp;

		/* Accept connection, spawn session thread, and */
		/* clean up old threads */

		config = iscsi_target_getvar(gp, "configfile");
		i = iscsi_waitfor_connection(gp->sockv, gp->sockc, config, &newconn);

		iscsi_trace(TRACE_NET_DEBUG, __FILE__, __LINE__, "waiting for %s connection on port %s\n",
			iscsi_address_family(gp->famv[i]),
			iscsi_target_getvar(gp, "target port"));

		if (!iscsi_sock_accept(newconn, &sess->sock)) {
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "iscsi_sock_accept() failed\n");
			goto done;
		}

		switch (gp->famv[i]) {
		case AF_INET:
			sess->address_family = 4;
			(void) memset(&localAddrStorage, 0x0, localAddrLen = sizeof(localAddrStorage));
			if (getsockname(sess->sock, (struct sockaddr *) (void *) &localAddrStorage, &localAddrLen) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_getsockname() failed\n");
				goto done;
			}

			(void) memset(&remoteAddrStorage, 0x0, remoteAddrLen = sizeof(remoteAddrStorage));
			if (getpeername(sess->sock, (struct sockaddr *) (void *) &remoteAddrStorage, &remoteAddrLen) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_getpeername() failed\n");
				goto done;
			}

#ifdef HAVE_GETNAMEINFO
			if (getnameinfo((struct sockaddr *)(void *)&localAddrStorage,
				sizeof(localAddrStorage), local, sizeof(local), NULL, 0, NI_NUMERICHOST) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "getnameinfo local failed\n");
			}
			if (getnameinfo((struct sockaddr *)(void *)&remoteAddrStorage,
				sizeof(remoteAddrStorage), remote, sizeof(remote), NULL, 0, NI_NUMERICHOST) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "getnameinfo remote failed\n");
			}
			(void) strlcpy(sess->initiator, remote, sizeof(sess->initiator));
#else
			(void) strlcpy(local, inet_ntoa(localAddrStorage.sin_addr), sizeof(local));
			(void) strlcpy(sess->initiator, inet_ntoa(remoteAddrStorage.sin_addr), sizeof(sess->initiator));
#endif

			(void) snprintf(targetaddress, sizeof(targetaddress),
				"%s:%s,1", local,
				iscsi_target_getvar(gp, "target port"));
			iscsi_target_setvar(gp, "target address", targetaddress);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__,
"IPv4 connection accepted on port %s (local IP %s, remote IP %s)\n",
			      iscsi_target_getvar(gp, "target port"),
			      local, sess->initiator);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "TargetAddress = \"%s\"\n", targetaddress);
			break;

		case AF_INET6:
			sess->address_family = 6;
			(void) memset(&localAddrStorage6, 0x0, localAddrLen = sizeof(localAddrStorage6));
			if (getsockname(sess->sock, (struct sockaddr *) (void *) &localAddrStorage6, &localAddrLen) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "getsockname() failed\n");
				goto done;
			}

			(void) memset(&remoteAddrStorage6, 0x0, remoteAddrLen = sizeof(remoteAddrStorage6));
			if (getpeername(sess->sock, (struct sockaddr *) (void *) &remoteAddrStorage6, &remoteAddrLen) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_sock_getpeername() failed\n");
				goto done;
			}

			if (getnameinfo((struct sockaddr *)(void *)&localAddrStorage6, sizeof(localAddrStorage6), local, sizeof(local), NULL, 0, NI_NUMERICHOST) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "getnameinfo local failed\n");
			}
			if (getnameinfo((struct sockaddr *)(void *)&remoteAddrStorage6, sizeof(remoteAddrStorage6), remote, sizeof(remote), NULL, 0, NI_NUMERICHOST) < 0) {
				iscsi_trace_error(__FILE__, __LINE__, "getnameinfo remote failed\n");
			}
			(void) strlcpy(sess->initiator, remote, sizeof(sess->initiator));
			(void) snprintf(targetaddress, sizeof(targetaddress),
				"%s:%s,1", local,
				iscsi_target_getvar(gp, "target port"));
			iscsi_target_setvar(gp, "target address", targetaddress);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "IPv6 connection accepted on port %s (local IP %s, remote IP %s)\n",
			      iscsi_target_getvar(gp, "target port"),
			      local, sess->initiator);
			iscsi_trace(TRACE_ISCSI_DEBUG, __FILE__, __LINE__, "TargetAddress = \"%s\"\n", targetaddress);
			break;
		}

		if (iscsi_thread_create(&sess->worker.thread, (void *) worker_proc_t, sess) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "iscsi_thread_create() failed\n");
			goto done;
		}
	}
done:
	gp->listener_listening--;
	return 0;
}

/* write the pid to the pid file */
void
iscsi_target_write_pidfile(const char *f)
{
	FILE	*fp;

	if (f == NULL) {
		f = _PATH_ISCSI_PID_FILE;
	}
	if ((fp = fopen(f, "w")) == NULL) {
		(void) fprintf(stderr, "Couldn't create pid file \"%s\": %s",
				f, strerror(errno)); 
	} else {
		(void) fprintf(fp, "%ld\n", (long) getpid());
		(void) fclose(fp);
	}
}

/* set a variable */
int
iscsi_target_setvar(iscsi_target_t *tgt, const char *name, const char *value)
{
	int	i;

	if ((i = findvar(tgt, name)) < 0) {
		/* add the element to the array */
		size_arrays(tgt, tgt->size + 15);
		tgt->name[i = tgt->c++] = strdup(name);
	} else {
		/* replace the element in the array */
		if (tgt->value[i]) {
			(void) free(tgt->value[i]);
			tgt->value[i] = NULL;
		}
	}
	/* sanity checks for range of values would go here */
	tgt->value[i] = strdup(value);
	return 1;
}

/* get a variable's value (NULL if not set) */
char *
iscsi_target_getvar(iscsi_target_t *tgt, const char *name)
{
	int	i;

	return ((i = findvar(tgt, name)) < 0) ? NULL : tgt->value[i];
}
