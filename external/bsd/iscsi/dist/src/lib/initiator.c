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

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
  
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
  
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
  
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
  
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
  
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
  
#include <stdio.h>
#include <stdlib.h>
   
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "iscsiprotocol.h"
#include "initiator.h"

#include "iscsi.h"

static initiator_target_t	g_target[CONFIG_INITIATOR_NUM_TARGETS];

/*
 * Globals
 */
static uint32_t g_tag;
static iscsi_spin_t g_tag_spin;
static hash_t   g_tag_hash;
static iscsi_worker_t g_enqueue_worker;
static iscsi_queue_t g_enqueue_q;
static iscsi_queue_t g_session_q;
static int      g_initiator_state;
static char           *gfilename;

/* Testing of initiator_abort */

static initiator_cmd_t *g_cmd = NULL;

/*
 * Enqueue worker functions. The enqueue worker is responsible for enqueing
 * all iSCSI commands to one of the Tx workers.  It is also the thread
 * responsible for initializing sessions, discovering targets and getting
 * each session into full feature phase.
 */

static int      enqueue_worker_proc(void *);
static int      login_phase_i(initiator_session_t *, char *, int);
static int      logout_phase_i(initiator_session_t *);

/*
 * Tx functions.  initiator_cmd_t pointers are enqueued to the Tx worker
 * for a given session by the enqueue worker.  The Tx worker will send out these
 * commands and wait for the Rx worker to process the response.  The pointer is
 * inserted into the hashtable g_tag_hash, keyed by the initiator tag of the iSCSI
 * commands.
 */

static int      tx_worker_proc_i(void *);
static int      text_command_i(initiator_cmd_t *);
static int      login_command_i(initiator_cmd_t *);
static int      logout_command_i(initiator_cmd_t *);
static int      scsi_command_i(initiator_cmd_t *);
static int      nop_out_i(initiator_cmd_t *);


/*
 * Rx functions. Upon receipt of an incoming PDU, the Rx worker will first
 * extract the tag (if it exists for the PDU) and then the associated
 * initiator_cmd_t pointer stored in the hash table.  One of Rx functions
 * will be called to processs the PDU. The Rx worker will invoke the callback
 * function associated with the command once the command has been retired.
 */

static int      rx_worker_proc_i(void *);
static int      login_response_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      text_response_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      logout_response_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      scsi_response_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      scsi_read_data_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      scsi_r2t_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      nop_in_i(initiator_session_t *, initiator_cmd_t *, uint8_t *);
static int      reject_i(initiator_session_t *, uint8_t *);
static int      async_msg_i(initiator_session_t *, uint8_t *);


/*
 * Misc. Prototypes
 */


static int      session_init_i(initiator_session_t **, uint64_t );
static int      session_destroy_i(initiator_session_t *);
static int      wait_callback_i(void *);
static int      discovery_phase(int, strv_t *);

/*
 * Private Functions
 */

#if 0
static void
dump_session(initiator_session_t * sess)
{
        iscsi_parameter_value_t *vp;
        iscsi_parameter_t       *ip;

                for (ip = sess->params ; ip ; ip = ip->next) {
                        printf("Key: %s Type: %d\n",ip->key,ip->type);
                        for (vp = ip->value_l ; vp ; vp = vp->next) {
                                printf("Value: %s\n",vp->value);
                        }
        }
}
#endif

/* This function reads the target IP and target name information */
/* from the input configuration file, and populates the */
/* g_target data structure  fields. */
static int 
get_target_config(const char *hostname, int port)
{
	int i;

	for (i = 0 ; i < CONFIG_INITIATOR_NUM_TARGETS ; i++) {
		(void) strlcpy(g_target[i].name, hostname,
			sizeof(g_target[i].name));
		g_target[i].port = port;
	}
	return 0;
}

static int 
session_init_i(initiator_session_t ** sess, uint64_t isid)
{
	initiator_session_t	*s;
	iscsi_parameter_t	**l;
        char			*user;
        int			 auth_type;
        int			 mutual_auth;
	int			 one = 1;

	iscsi_trace(TRACE_ISCSI_DEBUG, "initializing session %" PRIu64 "\n", isid);

	/* Get free session */
	if ((*sess = iscsi_queue_remove(&g_session_q)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_remove() failed\n");
		return -1;
	}
	s = *sess;
	user = NULL;
        auth_type = s->sess_params.auth_type;
        if (s->sess_params.cred.user && auth_type != AuthNone) {
                user = s->sess_params.cred.user;
        }
        mutual_auth = s->sess_params.mutual_auth;
	(void) memset(s, 0x0, sizeof(*s));
	s->state = INITIATOR_SESSION_STATE_INITIALIZING;
	s->isid = s->tx_worker.id = s->rx_worker.id = (int)isid;
	s->cmds = NULL;
        s->sess_params.cred.user = user;
        s->sess_params.auth_type = auth_type;
        s->sess_params.mutual_auth = mutual_auth;

	iscsi_spin_init(&s->cmds_spin);
	g_target[(int)isid].has_session = 1;

	/* Create socket */
	if (!iscsi_sock_create(&s->sock)) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_create() failed\n");
		return -1;
	}
	if (!iscsi_sock_setsockopt(&s->sock, SOL_TCP, TCP_NODELAY, &one, sizeof(one))) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_setsockopt() failed\n");
		return -1;
	}

	/* Initialize wait queues */

	ISCSI_MUTEX_INIT(&s->tx_worker.work_mutex, return -1);
	ISCSI_COND_INIT(&s->tx_worker.work_cond, return -1);
	ISCSI_MUTEX_INIT(&s->tx_worker.exit_mutex, return -1);
	ISCSI_COND_INIT(&s->tx_worker.exit_cond, return -1);
	ISCSI_MUTEX_INIT(&s->rx_worker.work_mutex, return -1);
	ISCSI_COND_INIT(&s->rx_worker.work_cond, return -1);
	ISCSI_MUTEX_INIT(&s->rx_worker.exit_mutex, return -1);
	ISCSI_COND_INIT(&s->rx_worker.exit_cond, return -1);

	/* Build parameter list */

	/*
         * ISCSI_PARAM_TYPE_LIST format:        <type> <key> <dflt> <valid list values>
         * ISCSI_PARAM_TYPE_BINARY format:      <type> <key> <dflt> <valid binary values>
         * ISCSI_PARAM_TYPE_NUMERICAL format:   <type> <key> <dflt> <max>
         * ISCSI_PARAM_TYPE_DECLARATIVE format: <type> <key> <dflt> ""
         */

	s->params = NULL;
	l = &(s->params);
	/* CHAP Support Parameters */
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "AuthMethod", "None", "CHAP,None", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "CHAP_A", "None", "5", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_N", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_R", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_I", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "CHAP_C", "", "", return -1);
	/* CHAP Support Parameters */

	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "HeaderDigest", "None", "None", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_LIST, "DataDigest", "None", "None", return -1);

	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "MaxConnections", "1", "1", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "SendTargets", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARE_MULTI, "TargetName", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "InitiatorName", "iqn.1994-04.org.NetBSD:iscsi-initiator", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "TargetAlias", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "InitiatorAlias", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARE_MULTI, "TargetAddress", "", "", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_OR, "InitialR2T", "Yes", "Yes,No", return -1);

	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_AND, "ImmediateData", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "MaxRecvDataSegmentLength", "8192", "16777215", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "MaxBurstLength", "262144", "16777215", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, "FirstBurstLength", "65536", "16777215", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "TargetPortalGroupTag", "1", "65535", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "DefaultTime2Wait", "2", "2", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "DefaultTime2Retain", "20", "20", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "MaxOutstandingR2T", "1", "1", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_OR, "DataPDUInOrder", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_BINARY_OR, "DataSequenceInOrder", "Yes", "Yes,No", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_NUMERICAL, "ErrorRecoveryLevel", "0", "0", return -1);
	PARAM_LIST_ADD(l, ISCSI_PARAM_TYPE_DECLARATIVE, "SessionType", "Normal", "Normal,Discovery", return -1);

	/* Start Tx worker */

	iscsi_trace(TRACE_ISCSI_DEBUG, "starting Tx worker %" PRIu64 "\n", isid);
	if (iscsi_queue_init(&s->tx_queue, CONFIG_INITIATOR_QUEUE_DEPTH) == -1) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		return -1;
	}
	ISCSI_LOCK(&s->tx_worker.exit_mutex, return -1);
	if (iscsi_thread_create(&s->tx_worker.thread,
			(void *) tx_worker_proc_i, &s->tx_worker) != 0) {
		iscsi_err(__FILE__, __LINE__,
			"iscsi_threads_create() failed\n");
		return -1;
	}
	ISCSI_WAIT(&s->tx_worker.exit_cond, &s->tx_worker.exit_mutex,
			return -1);
	ISCSI_UNLOCK(&s->tx_worker.exit_mutex, return -1);
	if (s->state == INITIATOR_SESSION_STATE_DESTROYING) {
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"session %" PRIu64 " is being destroyed, exiting\n", isid);
		return -1;
	}
	if (s->tx_worker.state & ISCSI_WORKER_STATE_ERROR) {
		iscsi_err(__FILE__, __LINE__,
			"Tx worker %" PRIu64 " started with an error\n", isid);
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "got signal from Tx worker\n");
	s->state = INITIATOR_SESSION_STATE_INITIALIZED;

	return 0;
}

static int 
session_destroy_i(initiator_session_t * sess)
{
	initiator_cmd_t	*ptr;
	uint64_t	 isid = sess->isid;

	if (sess == NULL) {
		iscsi_err(__FILE__, __LINE__, "session pointer is NULL\n");
		return -1;
	}
	if (g_target[(int)sess->isid].has_session == 0) {
		iscsi_err(__FILE__, __LINE__,
			"g_target[%" PRIu64 "].has_session==0??\n", sess->isid);
		return -1;
	}
	sess->state = INITIATOR_SESSION_STATE_DESTROYING;

	/* Abort all outstanding commands */

	for (ptr = sess->cmds; ptr != NULL; ptr = ptr->next) {
		if (initiator_abort(ptr) != 0) {
			iscsi_err(__FILE__, __LINE__, "initiator_abort() failed\n");
			return -1;
		}
	}

	if (sess->tx_worker.state & ISCSI_WORKER_STATE_STARTED) {
		if (sess->tx_worker.state & ISCSI_WORKER_STATE_EXITING) {
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"Tx worker %" PRIu64 " already signalled for exit\n",
				sess->isid);
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"signaling Tx worker %" PRIu64 " into exiting state\n",
				sess->isid);
			ISCSI_LOCK(&sess->tx_worker.work_mutex, return -1);
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"signaling socket shutdown to Tx worker %" PRIu64 "\n",				sess->isid);
			if (iscsi_sock_shutdown(sess->sock, 1) != 0) {
				iscsi_err(__FILE__, __LINE__,
					"iscsi_sock_shutdown() failed\n");
			}
			ISCSI_SIGNAL(&sess->tx_worker.work_cond, return -1);
			ISCSI_UNLOCK(&sess->tx_worker.work_mutex, return -1);
		}
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"Checking exit condition of Tx worker\n");
		while ((sess->tx_worker.state & ISCSI_WORKER_STATE_EXITING) !=
				ISCSI_WORKER_STATE_EXITING) {
			ISCSI_SPIN;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "Tx worker %" PRIu64 " has exited\n",
			sess->isid);
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"Tx worker was not started. Nothing to signal\n");
	}

	/* Destroy Tx state */
	while ((ptr = iscsi_queue_remove(&sess->tx_queue)) != NULL) {
		ptr->status = -1;
		if (ptr->callback && ((*ptr->callback)(ptr) != 0)) {
			iscsi_err(__FILE__, __LINE__, "callback() failed\n");
		}
	}
	iscsi_queue_destroy(&sess->tx_queue);

	if (sess->rx_worker.state & ISCSI_WORKER_STATE_STARTED) {
		if (sess->rx_worker.state & ISCSI_WORKER_STATE_EXITING) {
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"Rx worker %" PRIu64 " already signalled for exit\n",
				sess->isid);
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"signaling Rx worker %" PRIu64 " into exiting state\n",				sess->isid);
			if (iscsi_sock_shutdown(sess->sock, 0) != 0) {
				iscsi_err(__FILE__, __LINE__,
					"iscsi_sock_shutdown() failed\n");
			}
		}
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"Checking exit condition of Rx worker\n");
		while ((sess->rx_worker.state & ISCSI_WORKER_STATE_EXITING) !=
				ISCSI_WORKER_STATE_EXITING) {
			ISCSI_SPIN;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "Rx worker %" PRIu64 " has exited\n",
				sess->isid);
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"Rx worker was not started. Nothing to signal\n");
	}

	/* Close socket */

	if (iscsi_sock_close(sess->sock) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_close() failed\n");
		return -1;
	}
	/* Destroy wait queues */

	ISCSI_MUTEX_DESTROY(&sess->tx_worker.work_mutex, return -1);
	ISCSI_COND_DESTROY(&sess->tx_worker.work_cond, return -1);
	ISCSI_MUTEX_DESTROY(&sess->tx_worker.exit_mutex, return -1);
	ISCSI_COND_DESTROY(&sess->tx_worker.exit_cond, return -1);
	ISCSI_MUTEX_DESTROY(&sess->rx_worker.work_mutex, return -1);
	ISCSI_COND_DESTROY(&sess->rx_worker.work_cond, return -1);
	ISCSI_MUTEX_DESTROY(&sess->rx_worker.exit_mutex, return -1);
	ISCSI_COND_DESTROY(&sess->rx_worker.exit_cond, return -1);

	/* Destroy param list */

	PARAM_LIST_DESTROY(sess->params, return -1);

	/* Enqueue session to free list */
	if (iscsi_queue_insert(&g_session_q, sess) == -1) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "session %p destroyed and requeued\n",
			sess);

	g_target[(int)isid].has_session = 0;

	return 0;
}

#define IS_DISCOVERY	1
#define IS_SECURITY	1

enum {
	SESS_TYPE_DISCOVERY = 1,
	SESS_TYPE_NORMAL = 2,
	SESS_TYPE_NONE = 3
};

static int 
params_out(initiator_session_t * sess, char *text, int *len, int textsize, int sess_type, int security)
{
	if (security == IS_SECURITY) {
		PARAM_TEXT_ADD(sess->params, "InitiatorName", "iqn.1994-04.org.NetBSD.iscsi-initiator:agc", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "InitiatorAlias", "NetBSD", text, len, textsize, 1, return -1);
		if (sess->sess_params.auth_type != AuthNone) {
			PARAM_TEXT_ADD(sess->params, "AuthMethod", "CHAP,None", text, len, textsize, 1, return -1);
		} else {
			PARAM_TEXT_ADD(sess->params, "AuthMethod", "None", text, len, textsize, 1, return -1);
		}
	} else {
		PARAM_TEXT_ADD(sess->params, "HeaderDigest", "None", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "DataDigest", "None", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "MaxConnections", "1", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "InitialR2T", "Yes", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "ImmediateData", "Yes", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "MaxRecvDataSegmentLength", "8192", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "FirstBurstLength", "65536", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "MaxBurstLength", "262144", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "DefaultTime2Wait", "2", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "DefaultTime2Retain", "20", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "MaxOutstandingR2T", "1", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "DataPDUInOrder", "No", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "DataSequenceInOrder", "No", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "ErrorRecoveryLevel", "0", text, len, textsize, 1, return -1);
	}
	switch (sess_type) {
	case SESS_TYPE_DISCOVERY:
		PARAM_TEXT_ADD(sess->params, "SessionType", "Discovery", text, len, textsize, 1, return -1);
		break;
	case SESS_TYPE_NORMAL:
		PARAM_TEXT_ADD(sess->params, "SessionType", "Normal", text, len, textsize, 1, return -1);
		PARAM_TEXT_ADD(sess->params, "TargetName", g_target[(int)sess->isid].TargetName, text, len, textsize, 1, return -1);
		break;
	default:
		break;
	}
	PARAM_TEXT_PARSE(sess->params, &sess->sess_params.cred, text, *len, NULL, NULL, 0, 1, return -1);
	return 0;
}

static int 
full_feature_negotiation_phase_i(initiator_session_t * sess, char *text,
				int text_len)
{
	initiator_cmd_t *cmd = NULL;
	iscsi_text_cmd_args_t *text_cmd = NULL;
	initiator_wait_t iwait;

	/* Allocate command pointers */

	if ((cmd = iscsi_malloc_atomic(sizeof(initiator_cmd_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	(void) memset(cmd, 0x0, sizeof(*cmd));
	text_cmd = iscsi_malloc_atomic(sizeof(iscsi_text_cmd_args_t));
	if (text_cmd == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (cmd != NULL)
			iscsi_free_atomic(cmd);	/* initiator command */
		return -1;
	}
#define FFN_ERROR {if (cmd != NULL) iscsi_free_atomic(cmd); if (text_cmd != NULL) iscsi_free_atomic(text_cmd); return -1;}
	(void) memset(text_cmd, 0x0, sizeof(*text_cmd));

	/*
         * Note that <final>, <length> and <text> are updated
         * by text_response_i when we receive offers from
         * the target.
         */
	text_cmd->text = text;
	text_cmd->length = text_len;

	do {

		/* Build text command */

		text_cmd->final = 1;
		text_cmd->cont = 0;
		ISCSI_SET_TAG(&text_cmd->tag);
		text_cmd->transfer_tag = 0xffffffff;

		/* Build wait for callback */

		ISCSI_MUTEX_INIT(&iwait.mutex, FFN_ERROR);
		ISCSI_COND_INIT(&iwait.cond, FFN_ERROR);

		/* Build initiator command */

		cmd->type = ISCSI_TEXT_CMD;
		cmd->ptr = text_cmd;
		cmd->callback = wait_callback_i;
		cmd->callback_arg = &iwait;
		cmd->isid = sess->isid;

		/* Enqueue initiator command to Tx worker */

		iscsi_trace(TRACE_ISCSI_DEBUG,
			"enqueing text command to tx worker %" PRIu64 "\n",
			sess->isid);
		ISCSI_LOCK(&iwait.mutex, FFN_ERROR);
		ISCSI_LOCK(&sess->tx_worker.work_mutex, FFN_ERROR);
		if (iscsi_queue_insert(&sess->tx_queue, cmd) == -1) {
			ISCSI_UNLOCK(&iwait.mutex, );
			iscsi_err(__FILE__, __LINE__,
				"iscsi_queue_insert() failed\n");
			FFN_ERROR;
		}
		ISCSI_SIGNAL(&sess->tx_worker.work_cond, FFN_ERROR);
		ISCSI_UNLOCK(&sess->tx_worker.work_mutex, FFN_ERROR);
		iscsi_trace(TRACE_ISCSI_DEBUG, "enqueued text command ok\n");

		/* Wait for callback  */

		iscsi_trace(TRACE_ISCSI_DEBUG, "waiting on text callback\n");
		ISCSI_WAIT(&iwait.cond, &iwait.mutex, FFN_ERROR);
		ISCSI_UNLOCK(&iwait.mutex, FFN_ERROR);
		ISCSI_COND_DESTROY(&iwait.cond, FFN_ERROR);
		ISCSI_MUTEX_DESTROY(&iwait.mutex, FFN_ERROR);
		iscsi_trace(TRACE_ISCSI_DEBUG, "received text callback ok\n");

		/*
		 * See if we're done.  text_response_i() overwrites
		 * text_cmd->final
		 */
		/* with the final bit in the text response from the target. */

		if (!text_cmd->final) {
			iscsi_trace(TRACE_ISCSI_PARAM,
				"more negotiation needed (sending %d bytes "
				"response parameters)\n",
				text_cmd->length);
		}
	} while (!text_cmd->final);

	/* Free command pointers */

	iscsi_free_atomic(cmd->ptr);	/* text command */
	iscsi_free_atomic(cmd);	/* initiator command */

	return 0;
}

#define DISCOVERY_PHASE_TEXT_LEN	1024
#define DP_CLEANUP {if (text != NULL) iscsi_free_atomic(text);}
#define DP_ERROR {DP_CLEANUP; return -1;}

int
initiator_set_target_name(int target, char *target_name)
{
	(void) strlcpy(g_target[target].iqnwanted, target_name,
			sizeof(g_target[target].iqnwanted));
	(void) strlcpy(g_target[target].TargetName, target_name,
			sizeof(g_target[target].TargetName));
	return 0;
}


int
iscsi_initiator_get_max_targets(void)
{
	return CONFIG_INITIATOR_NUM_TARGETS;
}

#if 0
int
iscsi_initiator_get_targets(int target, strv_t *svp)
{
        initiator_session_t	*sess = g_target[target].sess;
        iscsi_parameter_value_t	*vp;
        iscsi_parameter_t	*ip;
        char			*text = NULL;
        int			 text_len = 0;
        int			 pos = 0;

        if ((text = iscsi_malloc_atomic(DISCOVERY_PHASE_TEXT_LEN)) == NULL) {
                iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
                return -1;
        }

        text_len = 0;
        text[0] = 0x0;

        PARAM_TEXT_ADD(sess->params, "SendTargets", "All", text, &text_len,
			DISCOVERY_PHASE_TEXT_LEN, 1, DP_ERROR);
        PARAM_TEXT_PARSE(sess->params, &sess->sess_params.cred, text,
			text_len, NULL, NULL, DISCOVERY_PHASE_TEXT_LEN, 1,
			DP_ERROR);
        if (full_feature_negotiation_phase_i(sess, text, text_len) != 0) {
                iscsi_err(__FILE__, __LINE__,
			"full_feature_negotiation_phase_i() failed\n");
                DP_ERROR;
        }       
        for (ip = sess->params ; ip ; ip = ip->next) {
                if (strcmp(ip->key, "TargetName") == 0) {
                	pos = 0;
                        for (vp = ip->value_l ; vp ; vp = vp->next, pos++) {
                        	/*
                        	 * Skip items which have no name,
                        	 * these have been blocked by the target
                        	 */
                        	if (!strlen(vp->value))
                        		continue;
				
                                ALLOC(char *, svp->v, svp->size, svp->c, 10,
						10, "igt", return -1);
                                svp->v[svp->c++] = strdup(vp->value);
                                ALLOC(char *, svp->v, svp->size, svp->c, 10,
						10, "igt2", return -1);
                                svp->v[svp->c++] =
                                     strdup(param_val_which(sess->params,
                                     "TargetAddress", pos));
                        }
                }
        }

	return 1;
}
#else
/* SendTargets=All must be sent in discovery session. */
int
iscsi_initiator_get_targets(int target, strv_t *svp)
{
	initiator_session_t	*sess = g_target[target].sess;
	strv_t *tp = &g_target[target].all_targets;
	uint32_t i;

	if (sess == NULL)
		return -1;

	for (i = 0; i < tp->c; i++) {
		ALLOC(char *, svp->v, svp->size, svp->c, 10,
			10, "igt", return -1);
		svp->v[svp->c++] = strdup(tp->v[i]);
	}

	return 1;
}
#endif

static int 
discovery_phase(int target, strv_t *svp)
{
	initiator_session_t	*sess;
	iscsi_parameter_value_t	*vp;
	iscsi_parameter_t	*ip;
	unsigned		 i;
	char           		*ptr;
	char           		*colon_ptr;
	char           		*comma_ptr;
	char            	 port[64];
	char           		*text = NULL;
	int             	 text_len = 0;

	if (target >= CONFIG_INITIATOR_NUM_TARGETS) {
		iscsi_err(__FILE__, __LINE__,
			"target (%d) out of range [0..%d]\n", target,
			CONFIG_INITIATOR_NUM_TARGETS);
		return -1;
	}
	sess = g_target[target].sess;
	if ((text = iscsi_malloc_atomic(DISCOVERY_PHASE_TEXT_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
	/* Login to target */

	iscsi_trace(TRACE_ISCSI_DEBUG,
		"entering Discovery login phase with target %d (sock %#x)\n",
		target, (int) sess->sock);
	text[0] = 0x0;
	if (params_out(sess, text, &text_len, DISCOVERY_PHASE_TEXT_LEN,
			SESS_TYPE_DISCOVERY, IS_SECURITY) != 0) {
		iscsi_err(__FILE__, __LINE__, "params_out() failed\n");
		DP_ERROR;
	}
	if (login_phase_i(sess, text, text_len) != 0) {
		iscsi_err(__FILE__, __LINE__, "login_phase_i() failed\n");
		DP_ERROR;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"now full feature for Discovery with target %d\n", target);

	/* Full Feature Phase Negotiation (for SendTargets) */
	text_len = 0;
	text[0] = 0x0;
	PARAM_TEXT_ADD(sess->params, "SendTargets", "All", text, &text_len,
		DISCOVERY_PHASE_TEXT_LEN, 1, DP_ERROR);
	PARAM_TEXT_PARSE(sess->params, &sess->sess_params.cred, text,
		text_len, NULL, NULL, DISCOVERY_PHASE_TEXT_LEN, 1, DP_ERROR);
	if (full_feature_negotiation_phase_i(sess, text, text_len) != 0) {
		iscsi_err(__FILE__, __LINE__,
			"full_feature_negotiation_phase_i() failed\n");
		DP_ERROR;
	}

	/* fill in information on the targets from the TargetName values */
	(void) memset(svp, 0x0, sizeof(*svp));
	for (ip = sess->params ; ip ; ip = ip->next) {
		if (strcmp(ip->key, "TargetName") == 0) {
			for (vp = ip->value_l ; vp ; vp = vp->next) {
				ALLOC(char *, svp->v, svp->size, svp->c, 10,
					10, "discovery_phase", return -1);
				svp->v[svp->c++] = strdup(vp->value);
				ALLOC(char *, svp->v, svp->size, svp->c, 10,
					10, "discovery_phase2", return -1);
				svp->v[svp->c++] = strdup(param_val(
						sess->params, "TargetAddress"));
			}
		}
	}

	if (g_target[target].iqnwanted[0] == 0x0) {
		/*
		 * Use the first TargetName and TargetAddress sent to
		 * us (all others are currently ignored)
		 */
		if (param_val(sess->params, "TargetName") != NULL) {
			strlcpy(g_target[target].TargetName,
				param_val(sess->params, "TargetName"),
				sizeof(g_target[target].TargetName));
		} else {
			iscsi_err(__FILE__, __LINE__, "SendTargets failed\n");
			DP_ERROR;
		}
		if ((ptr = param_val(sess->params, "TargetAddress")) == NULL) {
			iscsi_err(__FILE__, __LINE__, "SendTargets failed\n");
			DP_ERROR;
		}
	} else {
		/* the user has asked for a specific target - find it */
		for (i = 0 ; i < svp->c ; i += 2) {
			if (strcmp(g_target[target].iqnwanted,
					svp->v[i]) == 0) {
				strlcpy(g_target[target].TargetName, svp->v[i],
					sizeof(g_target[target].TargetName));
				ptr = svp->v[i + 1];
				break;
			}
		}
		if (i >= svp->c) {
			iscsi_err(__FILE__, __LINE__,
				"SendTargets failed - target `%s' not found\n",
				g_target[target].iqnwanted);
			DP_ERROR;
		}
	}

	if (*ptr == 0x0) {
		iscsi_err(__FILE__, __LINE__,
			"Target is not allowing access\n");
		DP_ERROR;
	}
	colon_ptr = strchr(ptr, ':');
	if ((comma_ptr = strchr(ptr, ',')) == NULL) {
		iscsi_err(__FILE__, __LINE__,
			"portal group tag is missing in \"%s\"\n",
			param_val(sess->params, "TargetAddress"));
		DP_ERROR;
	}
	if (colon_ptr) {
		strncpy(g_target[target].ip, ptr, (size_t)(colon_ptr - ptr));
		strncpy(port, colon_ptr + 1,
			(size_t)(comma_ptr - colon_ptr - 1));
		port[comma_ptr - colon_ptr - 1] = 0x0;
		g_target[target].port = iscsi_atoi(port);
	} else {
		strncpy(g_target[target].ip, ptr, (size_t)(comma_ptr - ptr));
		g_target[target].port = ISCSI_PORT;
	}

	iscsi_trace(TRACE_ISCSI_DEBUG, "Discovered \"%s\" at \"%s:%u\"\n",
		g_target[target].TargetName, g_target[target].name,
		g_target[target].port);

	/* Logout from target */

	iscsi_trace(TRACE_ISCSI_DEBUG,
		"entering logout phase with target %d\n", target);
	if (logout_phase_i(sess) != 0) {
		iscsi_err(__FILE__, __LINE__, "logout_phase_i() failed\n");
		DP_ERROR;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "target %d logout phase complete\n",
		target);
	DP_CLEANUP;
	return 0;
}

#define FULL_FEATURE_PHASE_TEXT_LEN	1024

static int 
full_feature_phase(initiator_session_t * sess)
{
	char           *text;
	int             text_len;

	if ((text = iscsi_malloc_atomic(FULL_FEATURE_PHASE_TEXT_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
#define FFP_CLEANUP {if (text != NULL) iscsi_free_atomic(text);}
#define FFP_ERROR {FFP_CLEANUP; return -1;}
	/* Set text parameters */

	text[0] = 0x0;
	text_len = 0;
	if (params_out(sess, text, &text_len, FULL_FEATURE_PHASE_TEXT_LEN,
			SESS_TYPE_NORMAL, IS_SECURITY) != 0) {
		iscsi_err(__FILE__, __LINE__, "params_out() failed\n");
		FFP_ERROR;
	}
	/* Send login command */

	iscsi_trace(TRACE_ISCSI_DEBUG, "entering login phase\n");
	if (login_phase_i(sess, text, text_len) != 0) {
		iscsi_err(__FILE__, __LINE__, "login_phase_i() failed\n");
		FFP_ERROR;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "login phase successful\n");

	FFP_CLEANUP;
	return 0;
}

int 
iscsi_initiator_start(iscsi_initiator_t *ini)
{
	initiator_session_t	*sess = NULL;
	char			*dbg;
	char			*cp;
	int			 port;
	int			 i;

#define INIT_CLEANUP {if (sess != NULL) iscsi_free_atomic(sess);}
#define INIT_ERROR {INIT_CLEANUP; return -1;}

	if ((dbg = iscsi_initiator_getvar(ini, "debug")) != NULL) {
		set_debug(dbg);
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "initializing initiator\n");
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"target config filename to read from:%s\n", gfilename);
	port = atoi(iscsi_initiator_getvar(ini, "target port"));
	if (get_target_config(iscsi_initiator_getvar(ini,
				"target hostname"), port) != 0) {
		iscsi_err(__FILE__, __LINE__,
			"Error getting target configuration in config file\n");
		return -1;
	}
	g_initiator_state = 0;
	if (iscsi_queue_init(&g_session_q,
			CONFIG_INITIATOR_MAX_SESSIONS) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		return -1;
	}
	for (i = 0; i < CONFIG_INITIATOR_MAX_SESSIONS; i++) {
		sess = iscsi_malloc_atomic(sizeof(initiator_session_t));
		if (sess == NULL) {
			iscsi_err(__FILE__, __LINE__,
				"iscsi_malloc_atomic() failed\n");
			return -1;
		}
		if (iscsi_queue_insert(&g_session_q, sess) != 0) {
			iscsi_err(__FILE__, __LINE__,
				"iscsi_queue_init() failed\n");
			INIT_CLEANUP;
			return -1;
		}
		cp = iscsi_initiator_getvar(ini, "auth type");
		if (strcmp(cp, "none") == 0) {
			sess->sess_params.auth_type = AuthNone;
			sess->sess_params.cred.user = NULL;
		} else {
			sess->sess_params.cred.user =
				strdup(iscsi_initiator_getvar(ini, "user"));	
		}
		cp = iscsi_initiator_getvar(ini, "mutual auth");
		if (strcmp(cp, "none") == 0) {
			sess->sess_params.mutual_auth = 0;
		}
		cp = iscsi_initiator_getvar(ini, "digest type");
		if (strcmp(cp, "none") == 0) {
			sess->sess_params.digest_wanted = DigestNone;
		}
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "%d free sessions available\n",
			CONFIG_INITIATOR_MAX_SESSIONS);

	g_tag = 0xabc123;
	if (hash_init(&g_tag_hash, CONFIG_INITIATOR_QUEUE_DEPTH) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_init() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	iscsi_spin_init(&g_tag_spin);
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"tag hash table initialized with queue depth %d\n",
		CONFIG_INITIATOR_QUEUE_DEPTH);

	/*
	 * Start enqueue worker.  This thread accepts scsi commands from
	 * initiator_enqueue()
	 */
	/* and queues them onto one of the tx worker queues. */

	iscsi_trace(TRACE_ISCSI_DEBUG, "starting enqueue worker\n");
	if (iscsi_queue_init(&g_enqueue_q, CONFIG_INITIATOR_QUEUE_DEPTH) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "about to initialize mutex\n");
	ISCSI_MUTEX_INIT(&g_enqueue_worker.work_mutex, INIT_ERROR);
	ISCSI_COND_INIT(&g_enqueue_worker.work_cond, INIT_ERROR);
	ISCSI_MUTEX_INIT(&g_enqueue_worker.exit_mutex, INIT_ERROR);
	ISCSI_COND_INIT(&g_enqueue_worker.exit_cond, INIT_ERROR);
	ISCSI_LOCK(&g_enqueue_worker.exit_mutex, INIT_ERROR);

	iscsi_trace(TRACE_ISCSI_DEBUG, "spawning thread for enqueue worker\n");
	if (iscsi_thread_create(&g_enqueue_worker.thread,
		(void *) &enqueue_worker_proc, &g_enqueue_worker) != 0) {
		iscsi_err(__FILE__, __LINE__,
			"iscsi_threads_create() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "thread spawned, waiting for signal\n");
	ISCSI_WAIT(&g_enqueue_worker.exit_cond, &g_enqueue_worker.exit_mutex,
			INIT_ERROR);
	ISCSI_UNLOCK(&g_enqueue_worker.exit_mutex, INIT_ERROR);
	iscsi_trace(TRACE_ISCSI_DEBUG, "successfully started enqueue worker\n");

	iscsi_trace(TRACE_ISCSI_DEBUG, "initiator initialization complete\n");
	return 0;
}

int 
iscsi_initiator_shutdown(void)
{
	initiator_session_t	*sess;
	int			 i;

	iscsi_trace(TRACE_ISCSI_DEBUG, "shutting down initiator\n");
	for (i = 0; i < CONFIG_INITIATOR_NUM_TARGETS; i++) {
		if (g_target[i].has_session) {
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"entering logout phase for target %d\n", i);
			if (g_target[i].sess->rx_worker.state &
					ISCSI_WORKER_STATE_ERROR) {
				iscsi_warn(__FILE__, __LINE__,
					"rx worker exited abnormal, "
					"skipping logout phase\n");
			} else {
				if (logout_phase_i(g_target[i].sess) != 0) {
					iscsi_err(__FILE__, __LINE__,
					"logout_phase_i() failed "
					"for target %d\n", i);
				}
				iscsi_trace(TRACE_ISCSI_DEBUG,
					"logout phase complete for target "
					"%d (state %#x)\n",
					i, g_target[i].sess->state);
			}
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"destroying session for target %d\n", i);
			if (session_destroy_i(g_target[i].sess) != 0) {
				iscsi_err(__FILE__, __LINE__,
					"session_destroy_i() failed for "
					"target %d\n", i);
			}
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"session destroyed for target %d\n", i);
		}
	}

	g_initiator_state = INITIATOR_STATE_SHUTDOWN;
	if (g_enqueue_worker.state & ISCSI_WORKER_STATE_EXITING) {
		iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue already exiting\n");
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"signaling enqueue worker into exiting state\n");
		ISCSI_LOCK(&g_enqueue_worker.work_mutex, return -1);
		ISCSI_SIGNAL(&g_enqueue_worker.work_cond, return -1);
		ISCSI_UNLOCK(&g_enqueue_worker.work_mutex, return -1);
	}
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"Checking exit condition of enqueue worker\n");
	while ((g_enqueue_worker.state & ISCSI_WORKER_STATE_EXITING) !=
			ISCSI_WORKER_STATE_EXITING) {
		ISCSI_SPIN;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue worker has exited\n");

	iscsi_queue_destroy(&g_enqueue_q);
	ISCSI_MUTEX_DESTROY(&g_enqueue_worker.work_mutex, return -1);
	ISCSI_COND_DESTROY(&g_enqueue_worker.work_cond, return -1);
	ISCSI_MUTEX_DESTROY(&g_enqueue_worker.exit_mutex, return -1);
	ISCSI_COND_DESTROY(&g_enqueue_worker.exit_cond, return -1);

	while ((sess = iscsi_queue_remove(&g_session_q)) != NULL) {
		iscsi_free_atomic(sess);
	}
	iscsi_queue_destroy(&g_session_q);
	iscsi_spin_destroy(&g_tag_spin);
	hash_destroy(&g_tag_hash);
	iscsi_trace(TRACE_ISCSI_DEBUG, "initiator shutdown complete\n");
	return 0;
}

static int 
wait_callback_i(void *ptr)
{
	initiator_wait_t *iwait = (initiator_wait_t *) (((initiator_cmd_t *) ptr)->callback_arg);

	iwait = (initiator_wait_t *) (((initiator_cmd_t *) ptr)->callback_arg);
	ISCSI_LOCK(&iwait->mutex, return -1);
	ISCSI_SIGNAL(&iwait->cond, return -1);
	ISCSI_UNLOCK(&iwait->mutex, return -1);
	return 0;
}

int 
initiator_abort(initiator_cmd_t * cmd)
{
	initiator_cmd_t *ptr, *prev;
	initiator_session_t *sess;

	iscsi_err(__FILE__, __LINE__, "aborting iSCSI cmd 0x%p (type %d, isid %" PRIu64 ")\n",
		    cmd, cmd->type, cmd->isid);

	hash_remove(&g_tag_hash, cmd->key);
	if (g_target[(int)cmd->isid].has_session) {
		sess = g_target[(int)cmd->isid].sess;
		iscsi_spin_lock(&sess->cmds_spin);
		prev = ptr = sess->cmds;
		while (ptr != NULL) {
			prev = ptr;
			if (ptr == cmd)
				break;
			ptr = ptr->next;
		}
		if (ptr != NULL) {
			if (prev == sess->cmds) {
				sess->cmds = cmd->next;
			} else {
				prev->next = cmd->next;
			}
		}
		iscsi_spin_unlock(&sess->cmds_spin);
	} else {
		iscsi_err(__FILE__, __LINE__, "cmd 0x%p has no session\n", cmd);
	}
	cmd->status = -1;
	if (cmd->callback) {
		if ((*cmd->callback)(cmd) != 0) {
			iscsi_err(__FILE__, __LINE__, "cmd->callback() failed\n");
			return -1;
		}
	}
	iscsi_err(__FILE__, __LINE__, "successfully aborted iSCSI cmd 0x%p (type %d, isid %" PRIu64 ")\n",
		    cmd, cmd->type, cmd->isid);
	return 0;
}

int 
initiator_command(initiator_cmd_t * cmd)
{
	initiator_wait_t iwait;

	ISCSI_MUTEX_INIT(&iwait.mutex, return -1);
	ISCSI_COND_INIT(&iwait.cond, return -1);
	ISCSI_LOCK(&iwait.mutex, return -1);
	cmd->callback = wait_callback_i;
	cmd->callback_arg = &iwait;
	cmd->status = -1;
	if (initiator_enqueue(cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_enqueue() failed\n");
		return -1;
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG, "command (type %d) enqueued, waiting on condition\n", cmd->type);
		ISCSI_WAIT(&iwait.cond, &iwait.mutex, return -1);
		iscsi_trace(TRACE_ISCSI_DEBUG, "condition signaled\n");
	}
	ISCSI_UNLOCK(&iwait.mutex, return -1);
	ISCSI_COND_DESTROY(&iwait.cond, return -1);
	ISCSI_MUTEX_DESTROY(&iwait.mutex, return -1);

	return cmd->status;
}

/*
 * initiator_enqueue() may be called from within interrupt context within
 * the midlayer.  This function cannot block or be scheduled within.
 * All we do is enqueue the args ptr to g_enqueue_q.  The thread in
 * enqueue_worker_proc will enqueue the ptr onto one of the tx queues.
 */

int 
initiator_enqueue(initiator_cmd_t * cmd)
{
	initiator_session_t *sess;
	iscsi_scsi_cmd_args_t *scsi_cmd;
	iscsi_nop_out_args_t *nop_out;
	uint64_t target;
	uint32_t        tag;

	if ((target = cmd->isid) >= CONFIG_INITIATOR_NUM_TARGETS) {
		iscsi_err(__FILE__, __LINE__, "target (%" PRIu64 ") out of range [0..%d]\n", target, CONFIG_INITIATOR_NUM_TARGETS);
		return -1;
	}
	sess = g_target[(int)target].sess;
	if (g_target[(int)target].has_session &&
	    sess->state == INITIATOR_SESSION_STATE_LOGGED_IN_NORMAL) {

		/* Give command directly to tx worker */

		ISCSI_SET_TAG_IN_INTR(&tag);
		target = cmd->isid;

		switch (cmd->type) {
		case ISCSI_SCSI_CMD:
			scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr;
			scsi_cmd->tag = tag;
			break;
		case ISCSI_NOP_OUT:
			nop_out = (iscsi_nop_out_args_t *) cmd->ptr;
			if (nop_out->tag != 0xffffffff) {
				nop_out->tag = tag;
			}
			break;
		default:
			iscsi_err(__FILE__, __LINE__, "enqueue_worker: unknown command type %d\n", cmd->type);
			return -1;
		}
		if (iscsi_queue_insert(&sess->tx_queue, cmd) == -1) {
			iscsi_err(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
			return -1;
		}
		ISCSI_LOCK(&sess->tx_worker.work_mutex, return -1);
		ISCSI_SIGNAL(&sess->tx_worker.work_cond, return -1);
		ISCSI_UNLOCK(&sess->tx_worker.work_mutex, return -1);
		iscsi_trace(TRACE_ISCSI_DEBUG, "initiator_cmd_t 0x%p given to tx_worker[%" PRIu64 "]\n", cmd, cmd->isid);
	} else {

		/*
		 * Give command to enqueue worker to get us into full feature
		 * and then issue the command
		 */
		/* to one of the tx workers. */

		if (iscsi_queue_insert(&g_enqueue_q, cmd) == -1) {
			iscsi_err(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
			return -1;
		}
		ISCSI_LOCK(&g_enqueue_worker.work_mutex, return -1);
		ISCSI_SIGNAL(&g_enqueue_worker.work_cond, return -1);
		ISCSI_UNLOCK(&g_enqueue_worker.work_mutex, return -1);
		iscsi_trace(TRACE_ISCSI_DEBUG, "initiator_cmd_t 0x%p given to enqueue worker\n", cmd);
	}
	return 0;
}

static int 
enqueue_worker_proc(void *arg)
{
	initiator_session_t *sess;
	initiator_cmd_t *cmd;
	iscsi_scsi_cmd_args_t *scsi_cmd;
	iscsi_nop_out_args_t *nop_out;
	iscsi_worker_t *me = (iscsi_worker_t *) arg;
	uint64_t	target;
	uint32_t        tag;
	int             rc;


	ISCSI_THREAD_START("enqueue_worker");
	ISCSI_SET_THREAD(me)
		ISCSI_LOCK(&me->exit_mutex, goto done);

	me->pid = getpid();
	me->state = ISCSI_WORKER_STATE_STARTED;
	ISCSI_SIGNAL(&me->exit_cond, goto done);
	ISCSI_UNLOCK(&me->exit_mutex, goto done);
	iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: started\n");
	ISCSI_LOCK(&g_enqueue_worker.work_mutex, goto done);
	for (;;) {
		if (iscsi_queue_depth(&g_enqueue_q) || (g_initiator_state == INITIATOR_STATE_SHUTDOWN)) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "enqueu, start to work\n");
			ISCSI_UNLOCK(&g_enqueue_worker.work_mutex, goto done);
			if (g_initiator_state == INITIATOR_STATE_SHUTDOWN) {
				iscsi_trace(TRACE_ISCSI_DEBUG, "got shutdown signal\n");
				goto done;
			}
			if ((cmd = iscsi_queue_remove(&g_enqueue_q)) == NULL) {
				iscsi_err(__FILE__, __LINE__, "enqueue_worker: iscsi_queue_remove() failed\n");
				goto done;
			}
			ISCSI_SET_TAG(&tag);
			target = cmd->isid;
			iscsi_trace(TRACE_ISCSI_CMD, "enqueue_worker: dequeued initiator_cmd_t 0x%p (type %d, target %" PRIu64 ")\n", cmd, cmd->type, target);
			switch (cmd->type) {
			case ISCSI_SCSI_CMD:
				scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr;
				scsi_cmd->tag = tag;
				break;
			case ISCSI_NOP_OUT:
				nop_out = (iscsi_nop_out_args_t *) cmd->ptr;
				if (nop_out->tag != 0xffffffff) {
					nop_out->tag = tag;
				}
				break;
			default:
				iscsi_err(__FILE__, __LINE__, "enqueue_worker: unknown command type %d\n", cmd->type);
				goto done;
			}

			/* Initialize session (if not already) */
initialize:
			if (!g_target[(int)target].has_session) {
				iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: initializing target %" PRIu64 " session\n", target);
				if (session_init_i(&g_target[(int)target].sess, target) != 0) {
					iscsi_err(__FILE__, __LINE__, "session_init_i() failed (ignoring command)\n");
					goto next;
				}
				iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: target %" PRIu64 " session initialized\n", target);
			} else {
				iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: target %" PRIu64 " session already initialized\n", target);
			}
			sess = g_target[(int)target].sess;
			iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: session 0x%p\n", sess);

			/* Discovery login if TargetName is zero length */

			if (strlen(g_target[(int)target].TargetName) == 0) {
				iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: entering Discovery phase with target %" PRIu64 "\n", target);
				rc = discovery_phase((int)target, &g_target[(int)target].all_targets);
				iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: Discovery phase complete\n");

				/* Destroy session */

				if (sess->state != INITIATOR_SESSION_STATE_DESTROYING) {
					if (g_target[(int)target].has_session) {
						if (session_destroy_i(g_target[(int)target].sess) != 0) {
							iscsi_err(__FILE__, __LINE__, "enqueue_worker: session_destroy_i() failed\n");
							goto done;
						}
					}
				}

				/*
				 * If the Discovery phase was
				 * successful, we re-initialize the
				 * session, enter full feature phase
				 * and then execute the command.
				 */

				if (rc == 0) {
					iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: discovery_phase() succeeded, entering full feature\n");
					goto initialize;
				} else {
					iscsi_err(__FILE__, __LINE__, "enqueue_worker: discovery_phase() failed (ignoring command)\n");
					goto next;
				}
			}
			/* Get into full feature if we're not already */

			if (sess->state != INITIATOR_SESSION_STATE_LOGGED_IN_NORMAL) {
				iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: entering full feature with target %" PRIu64 " (sock %#x)\n", target, (int) sess->sock);
				if (full_feature_phase(sess) != 0) {
					iscsi_err(__FILE__, __LINE__, "enqueue_worker: full_feature_phase() failed (ignoring command)\n");
					goto next;
				} else {
					iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: now full feature with target %" PRIu64 "\n", target);
				}
			}
			/*
			 * Now we are in FPP, so set the mostly
			 * accessed parameters for easy retrieval
			 * during data transfer
			 */
			set_session_parameters(sess->params, &sess->sess_params);

			/* Add command to tx work queue and signal worker */

			ISCSI_LOCK(&sess->tx_worker.work_mutex, goto done);
			if (iscsi_queue_insert(&sess->tx_queue, cmd) == -1) {
				ISCSI_UNLOCK(&sess->tx_worker.work_mutex, goto done);
				iscsi_err(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
				goto done;
			}
			ISCSI_SIGNAL(&sess->tx_worker.work_cond, goto done);
			ISCSI_UNLOCK(&sess->tx_worker.work_mutex, goto done);
			iscsi_trace(TRACE_ISCSI_CMD, "enqueue_worker: gave initiator_cmd_t 0x%p to tx_worker[%" PRIu64 "]\n", cmd, cmd->isid);
next:
			ISCSI_LOCK(&g_enqueue_worker.work_mutex, goto done);
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: queue empty, awaiting condition\n");
			ISCSI_WAIT(&g_enqueue_worker.work_cond, &g_enqueue_worker.work_mutex, goto done);
			iscsi_trace(TRACE_ISCSI_DEBUG, "enqueue_worker: condition signaled\n");
		}
	}
done:
	ISCSI_WORKER_EXIT(me);
	return 0;
}


/***********
 * Private *
 ***********/


/*
 * Tx Worker (one per connection)
 */

static int 
tx_worker_proc_i(void *arg)
{
	iscsi_worker_t *me = (iscsi_worker_t *) arg;
	initiator_cmd_t *cmd, *ptr;
	initiator_session_t *sess = g_target[me->id].sess;

	ISCSI_THREAD_START("tx_worker");

	ISCSI_SET_THREAD(me)
		me->pid = getpid();
	me->state = ISCSI_WORKER_STATE_STARTED;

	/* Connect to target */
	iscsi_trace(TRACE_ISCSI_DEBUG, "tx_worker[%d]: connecting to %s:%d\n",
	      me->id, g_target[me->id].name, g_target[me->id].port);
	sess->state = INITIATOR_SESSION_STATE_CONNECTING;
	if (iscsi_sock_connect(sess->sock, g_target[me->id].name,
			g_target[me->id].port) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_connect() failed\n");

		ISCSI_LOCK(&me->exit_mutex, return -1);
		me->state |= ISCSI_WORKER_STATE_ERROR;
		ISCSI_SIGNAL(&me->exit_cond, return -1);
		ISCSI_UNLOCK(&me->exit_mutex, return -1);
		goto done;

	}
	sess->state = INITIATOR_SESSION_STATE_CONNECTED;
	iscsi_trace(TRACE_ISCSI_DEBUG, "tx_worker[%d]: connected to %s:%d\n",
	      me->id, g_target[me->id].name, g_target[me->id].port);

	/* Start Rx worker */

	iscsi_trace(TRACE_ISCSI_DEBUG, "tx_worker[%d]: starting Rx worker\n",
		me->id);
	ISCSI_LOCK(&sess->rx_worker.exit_mutex, return -1);
	if (iscsi_thread_create(&sess->rx_worker.thread,
		(void *) rx_worker_proc_i, sess) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_thread_create() failed\n");
		goto done;
	}
	ISCSI_WAIT(&sess->rx_worker.exit_cond, &sess->rx_worker.exit_mutex,
		return -1);
	ISCSI_UNLOCK(&sess->rx_worker.exit_mutex, return -1);
	iscsi_trace(TRACE_ISCSI_DEBUG, "tx_worker[%d]: Rx worker started\n",
		me->id);

	/* Signal that we've started */
	ISCSI_LOCK(&me->exit_mutex, return -1);
	ISCSI_SIGNAL(&me->exit_cond, return -1);
	ISCSI_UNLOCK(&me->exit_mutex, return -1);
	iscsi_trace(TRACE_ISCSI_DEBUG, "tx_worker[%d]: successfully started\n",
		me->id);

	/* This Tx loop will exit when both the g_tx_queue is empty and  */
	/* sess->state != INITIATOR_SESSION_STATE_DESTROYING */

	ISCSI_LOCK(&me->work_mutex, return -1);
	for (;;) {

		if (iscsi_queue_depth(&g_target[me->id].sess->tx_queue) ||
		    sess->state == INITIATOR_SESSION_STATE_DESTROYING) {

			if (sess->state == INITIATOR_SESSION_STATE_DESTROYING) {
				iscsi_trace(TRACE_ISCSI_DEBUG,
					"tx_worker[%d]: session is being "
					"destroyed, exiting\n", me->id);
				ISCSI_UNLOCK(&me->work_mutex, return -1);
				goto done;
			}
			/* Get initiator command */

			cmd = iscsi_queue_remove(
					&g_target[me->id].sess->tx_queue);
			if (cmd == NULL) {
				iscsi_err(__FILE__, __LINE__,
					"tx_worker[%d]: iscsi_queue_remove "
					"failed\n", me->id);
				ISCSI_UNLOCK(&me->work_mutex, return -1);
				goto done;
			}
			ISCSI_UNLOCK(&me->work_mutex, return -1);
			iscsi_trace(TRACE_ISCSI_CMD,
				"tx_worker[%d]: dequeued initiator_cmd_t 0x%p "
				"(type %d, target %" PRIu64 ")\n",
				me->id, cmd, cmd->type, cmd->isid);

			/* Make sure we've got the right command */
			if (cmd->isid != (unsigned)me->id) {
				iscsi_err(__FILE__, __LINE__,
					"got command %#x for target %" PRIu64 ", "
					"expected %d\n", cmd->type,
					cmd->isid, me->id);
				goto done;
			}
			/*
			 * Add to list of oustanding commands in session
			 * (unless NOP_OUT without ping)
			 */
			if (!((cmd->type == ISCSI_NOP_OUT) &&
			    (((iscsi_nop_out_args_t *)(cmd->ptr))->tag ==
			    				0xffffffff))) {
				cmd->next = NULL;
				iscsi_spin_lock(&sess->cmds_spin);
				for (ptr = sess->cmds;
				     ptr && ptr->next != NULL;
				     ptr = ptr->next) {
				}
				if (ptr) {
					ptr->next = cmd;
				} else {
					sess->cmds = cmd;
				}
				iscsi_spin_unlock(&sess->cmds_spin);
			}
			cmd->tx_done = 0;
			switch (cmd->type) {
			case ISCSI_LOGIN_CMD:
				iscsi_trace(TRACE_ISCSI_CMD,
					"tx_worker[%d]: ISCSI_LOGIN_CMD\n",
					me->id);
				if (login_command_i(cmd) != 0) {
					iscsi_err(__FILE__, __LINE__,
						"tx_worker[%d]: "
						"login_command_i() failed\n",
						me->id);
					goto done;
				}
				break;
			case ISCSI_TEXT_CMD:
				iscsi_trace(TRACE_ISCSI_CMD,
					"tx_worker[%d]: ISCSI_TEXT_CMD\n",
					me->id);
				if (text_command_i(cmd) != 0) {
					iscsi_err(__FILE__, __LINE__,
						"tx_worker[%d]: text_command_i "
						"failed\n", me->id);
					goto done;
				}
				break;
			case ISCSI_SCSI_CMD:
				iscsi_trace(TRACE_ISCSI_CMD,
					"tx_worker[%d]: ISCSI_SCSI_CMD\n",
					me->id);
				if (scsi_command_i(cmd) != 0) {
					iscsi_err(__FILE__, __LINE__,
						"tx_worker[%d]: scsi_command_i"
						" failed\n", me->id);
					goto done;
				}
				break;
			case ISCSI_NOP_OUT:
				iscsi_trace(TRACE_ISCSI_CMD,
					"tx_worker[%d]: ISCSI_NOP_OUT\n",
					me->id);
				if (nop_out_i(cmd) != 0) {
					iscsi_err(__FILE__, __LINE__,
						"tx_worker[%d]: nop_out_i "
						"failed\n", me->id);
					goto done;
				}
				break;
			case ISCSI_LOGOUT_CMD:
				iscsi_trace(TRACE_ISCSI_CMD,
					"tx_worker[%d]: ISCSI_LOGOUT_CMD\n",
					me->id);
				if (logout_command_i(cmd) != 0) {
					iscsi_err(__FILE__, __LINE__,
						"tx_worker[%d]: "
						"logout_command_i() failed\n",
						me->id);
					goto done;
				}
				break;
			default:
				iscsi_err(__FILE__, __LINE__,
					"tx_worker[%d]: unknown iSCSI command"
					" %#x\n", me->id, cmd->type);
				cmd->status = -1;
				break;
			}

			/* Get lock for next iteration */

			ISCSI_LOCK(&me->work_mutex, return -1);

			/*
			 * The Rx thread will receive a response for
			 * the command and execute the callback.  We
			 * need to make sure the callback function is
			 * not executed before the Tx thread has
			 * completed sending the command.  This is
			 * what tx_done is used for.  The last step is
			 * to set tx_done and signal the Rx thread,
			 * which may be block on the condition. 
			 * NOP_OUT (without ping) will have no
			 * response for the Rx thread to process - so
			 * we execute the callback directly.  */

			if ((cmd->type == ISCSI_NOP_OUT) &&
			    (((iscsi_nop_out_args_t *)(cmd->ptr))->tag ==
			    				0xffffffff)) {
				iscsi_trace(TRACE_ISCSI_DEBUG,
					"executing callback() function "
					"directly for NOP_OUT (no NOP_IN)\n");
				if (cmd->callback(cmd) != 0) {
					iscsi_err(__FILE__, __LINE__,
						"cmd->callback() failed\n");
					return -1;
				}
			} else {
				ISCSI_LOCK(&sess->rx_worker.work_mutex,
					return -1);
				cmd->tx_done = 1;
				ISCSI_SIGNAL(&sess->rx_worker.work_cond,
					return -1);
				ISCSI_UNLOCK(&sess->rx_worker.work_mutex,
					return -1);
			}
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"tx_worker[%d]: awaiting condition\n", me->id);
			ISCSI_WAIT(&me->work_cond, &me->work_mutex, return -1);
			iscsi_trace(TRACE_ISCSI_DEBUG,
				"tx_worker[%d]: condition signaled\n", me->id);
		}
	}
done:
	if (sess->state != INITIATOR_SESSION_STATE_DESTROYING) {
		iscsi_err(__FILE__, __LINE__,
			"tx_worker[%d]: session exited prematurely "
			"(state %#x)\n", me->id, sess->state);
		me->state |= ISCSI_WORKER_STATE_ERROR;
	}
	ISCSI_WORKER_EXIT(me);
	return 0;
}

/*
 * There is one Rx worker per connection.
 */

static int 
rx_worker_proc_i(void *arg)
{
	uint8_t   header[ISCSI_HEADER_LEN];
	initiator_session_t *sess = (initiator_session_t *) arg;
	iscsi_worker_t *me = &sess->rx_worker;
	initiator_cmd_t *cmd = NULL;
	initiator_cmd_t *prev, *ptr;
	uint32_t        tag;

	ISCSI_THREAD_START("rx_worker");
	ISCSI_SET_THREAD(me)
		me->state = ISCSI_WORKER_STATE_STARTED;
	me->pid = getpid();
	ISCSI_LOCK(&me->exit_mutex, return -1);
	ISCSI_SIGNAL(&me->exit_cond, return -1);
	ISCSI_UNLOCK(&me->exit_mutex, return -1);

	iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: started (sess %p)\n", me->id, sess);

	for (;;) {
		iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: reading iscsi header (sock %#x) \n",
		      me->id, (int) sess->sock);
		if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: iscsi_sock_msg() failed\n", me->id);
			goto done;
		}
		if (sess->state == INITIATOR_SESSION_STATE_DESTROYING) {
			iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: session is being destroyed\n", me->id);
			goto done;
		}
		/* Get cmd ptr from hash table */

		if ((ISCSI_OPCODE(header) != ISCSI_REJECT) && (ISCSI_OPCODE(header) != ISCSI_ASYNC)) {
			(void) memcpy(&tag, header + 16, sizeof(tag));
			tag = ISCSI_NTOHL(tag);
			if (tag != 0xffffffff) {

				/*
				 * remove command from g_tag_hash, cmd is
				 * local so we only need to lock the queue
				 * remove operation
				 */

				if ((cmd = hash_remove(&g_tag_hash, tag)) == NULL) {
					iscsi_err(__FILE__, __LINE__, "hash_remove() failed\n");
					iscsi_trace(TRACE_ISCSI_DEBUG, "no cmd ptr associated with tag %#x\n", tag);
				} else {
					iscsi_trace(TRACE_ISCSI_DEBUG, "cmd ptr %p associated with tag %#x\n", cmd, tag);

					ISCSI_LOCK(&sess->rx_worker.work_mutex, return -1);
					if (!cmd->tx_done) {
						ISCSI_WAIT(&sess->rx_worker.work_cond, &sess->rx_worker.work_mutex, return -1);
					}
					ISCSI_UNLOCK(&sess->rx_worker.work_mutex, return -1);
				}
			} else {
				iscsi_trace(TRACE_ISCSI_DEBUG, "no command associated with tag %#x\n", tag);
			}
		}
		/* Remove cmd ptr from outstanding list */
		iscsi_spin_lock(&sess->cmds_spin);
		prev = ptr = sess->cmds;
		while (ptr != NULL) {
			prev = ptr;
			if (ptr == cmd)
				break;
			ptr = ptr->next;
		}
		if (ptr != NULL) {
			if (prev == sess->cmds) {
				sess->cmds = cmd->next;
			} else {
				prev->next = cmd->next;
			}
		}
		iscsi_spin_unlock(&sess->cmds_spin);
		switch (ISCSI_OPCODE(header)) {
		case ISCSI_SCSI_RSP:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_SCSI_RSP\n", me->id);
			if (scsi_response_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: scsi_response_i() failed\n", me->id);
				goto done;
			}
			break;
		case ISCSI_READ_DATA:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_READ_DATA\n", me->id);
			if (scsi_read_data_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: scsi_read_data_i() failed\n", me->id);
				goto done;
			}
			break;
		case ISCSI_R2T:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_R2T\n", me->id);
			if (scsi_r2t_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: scsi_r2t_i() failed\n", me->id);
				goto done;
			}
			break;
		case ISCSI_NOP_IN:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_NOP_IN\n", me->id);
			if (nop_in_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "nop_in_i() failed\n");
				return -1;
			}
			break;
		case ISCSI_LOGIN_RSP:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_LOGIN_RSP\n", me->id);
			if (login_response_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: login_response_i() failed\n", me->id);
				goto done;
			}
			break;
		case ISCSI_TEXT_RSP:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_TEXT_RSP\n", me->id);
			if (text_response_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: text_response_i() failed\n", me->id);
				goto done;
			}
			break;
		case ISCSI_LOGOUT_RSP:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_LOGOUT_RSP\n", me->id);
			if (logout_response_i(sess, cmd, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: logout_response_i() failed\n", me->id);
				goto done;
			}
			break;
		case ISCSI_REJECT:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_REJECT\n", me->id);
			if (reject_i(sess, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "reject_i() failed\n");
				return -1;
			}
			break;
		case ISCSI_ASYNC:
			iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: ISCSI_ASYNC\n", me->id);
			if (async_msg_i(sess, header) != 0) {
				iscsi_err(__FILE__, __LINE__, "async_msg_i() failed\n");
				goto done;
			}
			break;
		default:
			iscsi_err(__FILE__, __LINE__, "rx_worker[%d]: unexpected iSCSI op %#x\n", me->id, ISCSI_OPCODE(header));
			goto done;
		}
	}
done:
	if (sess->state != INITIATOR_SESSION_STATE_DESTROYING) {

		iscsi_trace(TRACE_ISCSI_DEBUG, "rx_worker[%d]: session exited prematurely (state %#x)\n", me->id, sess->state);
		me->state |= ISCSI_WORKER_STATE_ERROR;
	}
	ISCSI_WORKER_EXIT(me);
	return 0;
}

static int 
text_command_i(initiator_cmd_t * cmd)
{
	iscsi_text_cmd_args_t	*text_cmd;
	initiator_session_t	*sess;
	uint8_t			 header[ISCSI_HEADER_LEN];

	text_cmd = (iscsi_text_cmd_args_t *) cmd->ptr;
	sess = g_target[(int)cmd->isid].sess;
	/*
	 * Insert cmd into the hash table, keyed by the tag. The Rx thread
	 * will
	 */
	/* retrieve the cmd ptr using the tag from the response PDU. */

	if (hash_insert(&g_tag_hash, cmd, text_cmd->tag) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_insert() failed\n");
		return -1;
	}
	/* Send text command PDU */

	text_cmd->ExpStatSN = sess->ExpStatSN;
	text_cmd->CmdSN = sess->CmdSN++;
	iscsi_trace(TRACE_ISCSI_DEBUG, "sending text command\n");
	if (iscsi_text_cmd_encap(header, text_cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "(iscsi_text_cmd_encap() failed\n");
		return -1;
	}
	if ((unsigned)iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, text_cmd->text, text_cmd->length, 0)
	    != ISCSI_HEADER_LEN + text_cmd->length) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed.\n");
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "text command sent ok\n");

	return 0;
}

static int 
login_command_i(initiator_cmd_t * cmd)
{
	iscsi_login_cmd_args_t	*login_cmd;
	initiator_session_t	*sess;
	uint8_t			 header[ISCSI_HEADER_LEN];

	login_cmd = (iscsi_login_cmd_args_t *) cmd->ptr;
	sess = g_target[(int)cmd->isid].sess;
	/*
	 * Insert cmd into the hash table, keyed by the tag. The Rx thread
	 * will
	 */
	/* retrieve the cmd ptr using the tag from the response PDU. */

	if (hash_insert(&g_tag_hash, cmd, login_cmd->tag) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_insert() failed\n");
		return -1;
	}
	/* Send login command PDU */
	login_cmd->ExpStatSN = sess->ExpStatSN;
	iscsi_trace(TRACE_ISCSI_DEBUG, "sending login command\n");
	if (iscsi_login_cmd_encap(header, login_cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "(iscsi_login_cmd_encap() failed\n");
		return -1;
	}
	if ((unsigned)iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, login_cmd->text, login_cmd->length, 0)
	    != ISCSI_HEADER_LEN + login_cmd->length) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed.\n");
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "login command sent ok\n");

	return 0;
}

static int 
logout_phase_i(initiator_session_t * sess)
{
	initiator_cmd_t *cmd = NULL;
	iscsi_logout_cmd_args_t *logout_cmd = NULL;
	initiator_wait_t iwait;

	sess->state = INITIATOR_SESSION_STATE_LOGGING_OUT;

	/* Allocate command pointers */

	if ((cmd = iscsi_malloc_atomic(sizeof(initiator_cmd_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	(void) memset(cmd, 0x0, sizeof(*cmd));
	if ((logout_cmd = iscsi_malloc_atomic(sizeof(iscsi_logout_cmd_args_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (cmd != NULL)
			iscsi_free_atomic(cmd);
		return -1;
	}
#define LO_CLEANUP {if (cmd != NULL) iscsi_free_atomic(cmd); if (logout_cmd != NULL) iscsi_free_atomic(logout_cmd); }
#define LO_ERROR {LO_CLEANUP; return -1;}
	(void) memset(logout_cmd, 0x0, sizeof(*logout_cmd));

	/* Build logout command */

	logout_cmd->cid = sess->cid;
	logout_cmd->reason = ISCSI_LOGOUT_CLOSE_SESSION;
	ISCSI_SET_TAG(&logout_cmd->tag);
	logout_cmd->ExpStatSN = sess->ExpStatSN;
	logout_cmd->CmdSN = sess->CmdSN++;

	/* Build wait for callback */

	ISCSI_MUTEX_INIT(&iwait.mutex, LO_ERROR);
	ISCSI_COND_INIT(&iwait.cond, LO_ERROR);

	/* Build initiator command */

	cmd->type = ISCSI_LOGOUT_CMD;
	cmd->ptr = logout_cmd;
	cmd->callback = wait_callback_i;
	cmd->callback_arg = &iwait;
	cmd->isid = sess->isid;

	/* Enqueue to Tx worker */

	iscsi_trace(TRACE_ISCSI_DEBUG, "enqueing logout command to tx worker %" PRIu64 "\n", sess->isid);
	ISCSI_LOCK(&iwait.mutex, LO_ERROR);
	ISCSI_LOCK(&sess->tx_worker.work_mutex, LO_ERROR);
	if (iscsi_queue_insert(&sess->tx_queue, cmd) == -1) {
		ISCSI_UNLOCK(&sess->tx_worker.work_mutex, LO_ERROR);
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
		LO_ERROR;
	}
	ISCSI_SIGNAL(&sess->tx_worker.work_cond, LO_ERROR);
	ISCSI_UNLOCK(&sess->tx_worker.work_mutex, LO_ERROR);
	iscsi_trace(TRACE_ISCSI_DEBUG, "enqueued logout command ok\n");

	/* Wait for callback */

	iscsi_trace(TRACE_ISCSI_DEBUG, "waiting on logout callback\n");
	ISCSI_WAIT(&iwait.cond, &iwait.mutex, LO_ERROR);
	ISCSI_UNLOCK(&iwait.mutex, LO_ERROR);
	ISCSI_COND_DESTROY(&iwait.cond, LO_ERROR);
	ISCSI_MUTEX_DESTROY(&iwait.mutex, LO_ERROR);
	iscsi_trace(TRACE_ISCSI_DEBUG, "received logout callback ok\n");

	sess->state = INITIATOR_SESSION_STATE_LOGGED_OUT;

	LO_CLEANUP;
	return 0;
}

static void 
alarm_handler(int arg)
{
	USE_ARG(arg);
	iscsi_err(__FILE__, __LINE__, "***aborting cmd 0x%p***\n", g_cmd);
	if (initiator_abort(g_cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_abort() failed\n");
	}
}

static int 
login_phase_i(initiator_session_t * sess, char *text, int text_len)
{
	initiator_cmd_t *cmd = NULL;
	initiator_wait_t iwait;
	iscsi_login_cmd_args_t *login_cmd = NULL;
	struct sigaction act;

	sess->state = INITIATOR_SESSION_STATE_LOGGING_IN;

	/* Allocate command pointers */

	if ((cmd = iscsi_malloc_atomic(sizeof(initiator_cmd_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	(void) memset(cmd, 0x0, sizeof(*cmd));
	if ((login_cmd = iscsi_malloc_atomic(sizeof(iscsi_login_cmd_args_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (cmd != NULL)
			iscsi_free_atomic(cmd);
		return -1;
	}
#define LI_CLEANUP {if (cmd != NULL) iscsi_free_atomic(cmd); if (login_cmd != NULL) iscsi_free_atomic(login_cmd); }
#define LI_ERROR {LI_CLEANUP; return -1;}
	(void) memset(login_cmd, 0x0, sizeof(*login_cmd));

	/* This is the length of our original offer. */

	login_cmd->text = text;
	login_cmd->length = text_len;
	login_cmd->transit = 1;
	login_cmd->csg = ISCSI_LOGIN_STAGE_SECURITY;
	login_cmd->nsg = ISCSI_LOGIN_STAGE_NEGOTIATE;
	ISCSI_SET_TAG(&login_cmd->tag);
	login_cmd->CmdSN = sess->CmdSN = 0;

	do {

		/*
		 * Build login command.  Note that the <length> and
		 * <text> fields may get updated by login_response_i. 
		 * Such is the case when we receive offers from the
		 * target.  The new <length> and <text> fields will
		 * represent the response that we need to send to the
		 * target on the next login.
		 */

		login_cmd->cont = 0;
		login_cmd->version_min = ISCSI_VERSION;
		login_cmd->version_max = ISCSI_VERSION;
		login_cmd->cid = sess->cid = (int)sess->isid;
		login_cmd->isid = sess->isid = sess->isid;
		login_cmd->tsih = 0;

		/* Build wait for callback */

		ISCSI_MUTEX_INIT(&iwait.mutex, LI_ERROR);
		ISCSI_COND_INIT(&iwait.cond, LI_ERROR);

		/* Build initiator command */

		cmd->type = ISCSI_LOGIN_CMD;
		cmd->ptr = login_cmd;
		cmd->callback = wait_callback_i;
		cmd->callback_arg = &iwait;
		cmd->isid = sess->isid;

		/* Set Alarm */

		g_cmd = cmd;
		act.sa_handler = alarm_handler;
		sigaction(SIGALRM, &act, NULL);
		alarm(5);

		/* Enqueue initiator command to Tx worker */

		iscsi_trace(TRACE_ISCSI_DEBUG, "enqueing login command to tx worker %" PRIu64 "\n", sess->isid);
		ISCSI_LOCK(&iwait.mutex, LI_ERROR);
		ISCSI_LOCK(&sess->tx_worker.work_mutex, LI_ERROR);
		if (iscsi_queue_insert(&sess->tx_queue, cmd) == -1) {
			ISCSI_UNLOCK(&sess->tx_worker.work_mutex, LI_ERROR);
			iscsi_err(__FILE__, __LINE__, "iscsi_queue_insert() failed\n");
			LI_ERROR;

		}
		ISCSI_SIGNAL(&sess->tx_worker.work_cond, LI_ERROR);
		ISCSI_UNLOCK(&sess->tx_worker.work_mutex, LI_ERROR);
		iscsi_trace(TRACE_ISCSI_DEBUG, "enqueued login command ok\n");

		/* Wait for callback  */

		iscsi_trace(TRACE_ISCSI_DEBUG, "waiting on login callback\n");
		ISCSI_WAIT(&iwait.cond, &iwait.mutex, LI_ERROR);
		ISCSI_UNLOCK(&iwait.mutex, LI_ERROR);
		ISCSI_COND_DESTROY(&iwait.cond, LI_ERROR);
		ISCSI_MUTEX_DESTROY(&iwait.mutex, LI_ERROR);
		iscsi_trace(TRACE_ISCSI_DEBUG, "received login callback ok\n");

		alarm(0);

		if (cmd->status != 0) {
			iscsi_err(__FILE__, __LINE__, "initiator_cmd_t failed\n");
			LI_ERROR;
		}
		if (sess->state == INITIATOR_SESSION_STATE_LOGGING_IN) {
			iscsi_trace(TRACE_ISCSI_PARAM, "more negotiation needed (sending %d bytes response parameters)\n",
			      login_cmd->length);
		}
	} while (sess->state == INITIATOR_SESSION_STATE_LOGGING_IN);
	iscsi_trace(TRACE_ISCSI_DEBUG, "login phase completed successfully\n");

	LI_CLEANUP;
	return 0;
}


#define TEXT_RESPONSE_TEXT_LEN	2048

static int 
text_response_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_text_cmd_args_t *text_cmd;
	iscsi_text_rsp_args_t text_rsp;
	iscsi_parameter_t *l = sess->params;
	char           *text_in = NULL;
	char           *text_out = NULL;
	int             len_in = 0;
	int             len_out = 0;
	int             ret = 0;

#define TI_CLEANUP {if (text_in != NULL) iscsi_free_atomic(text_in); if (text_out != NULL) iscsi_free_atomic(text_out);}
#define TI_ERROR {cmd->status=-1; goto callback;}
	if (cmd) {
		text_cmd = (iscsi_text_cmd_args_t *) cmd->ptr;
	} else {
		iscsi_err(__FILE__, __LINE__, "no initiator_cmd_t specified for iscsi_text_cmd_args_t??\n");
		return -1;
	}

	/* Check arguments & update numbering */

	if (iscsi_text_rsp_decap(header, &text_rsp) != 0) {
		iscsi_err(__FILE__, __LINE__, "text_response_decap() failed\n");
		TI_ERROR;
	}
	if (text_rsp.tag != text_cmd->tag) {
		iscsi_err(__FILE__, __LINE__,
				"Bad \"Tag\": %u != %u.\n",
				text_rsp.tag, text_cmd->tag);
		TI_ERROR;
	}
	if (text_rsp.transfer_tag != 0xffffffff) {
		iscsi_err(__FILE__, __LINE__,
				"Bad \"Transfer Tag\": %u != %u.\n",
				text_rsp.transfer_tag, 0xffffffff);
		TI_ERROR;
	}
	if (text_rsp.StatSN != sess->ExpStatSN) {
		iscsi_err(__FILE__, __LINE__,
				"Bad \"StatSN\": %u != %u.\n",
				text_rsp.StatSN, sess->ExpStatSN);
		TI_ERROR;
	}
	if (text_rsp.ExpCmdSN != sess->CmdSN) {
		iscsi_err(__FILE__, __LINE__,
				"Bad \"ExpCmdSN\": %u != %u.\n",
				text_rsp.ExpCmdSN, sess->CmdSN);
		TI_ERROR;
	}
	sess->ExpStatSN = text_rsp.StatSN + 1;

	/* Parse input text parameters and generate any response */

	if ((len_in = text_rsp.length) != 0) {
		iscsi_trace(TRACE_ISCSI_PARAM, "allocating %d bytes input parameters\n", len_in);
		if ((text_in = iscsi_malloc_atomic((unsigned)len_in + 1)) == NULL) {
			iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
			TI_ERROR;
		}
		if ((text_out = iscsi_malloc_atomic(TEXT_RESPONSE_TEXT_LEN)) == NULL) {
			iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
			if (text_in != NULL)
				iscsi_free_atomic(text_in);
			TI_ERROR;
		}
		if (iscsi_sock_msg(sess->sock, 0, (unsigned)len_in, text_in, 0) != len_in) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			TI_ERROR;
		}
		text_in[len_in] = 0x0;
		iscsi_trace(TRACE_ISCSI_PARAM, "read %d bytes input parameters ok\n", len_in);

		/* Reset the value lists for TargetName and TargetAddress */

		if (param_val_reset(sess->params, "TargetName") != 0) {
			iscsi_err(__FILE__, __LINE__, "parm_val_reset() failed\n");
			TI_ERROR;
		}
		if (param_val_reset(sess->params, "TargetAddress") != 0) {
			iscsi_err(__FILE__, __LINE__, "parm_val_reset() failed\n");
			TI_ERROR;
		}
		/* Parse the incoming answer */

		PARAM_TEXT_PARSE(l, &sess->sess_params.cred, text_in, len_in, text_out, &len_out, TEXT_RESPONSE_TEXT_LEN, 0, TI_ERROR);

		if (len_out) {
			if (text_rsp.final != 0) {
				iscsi_err(__FILE__, __LINE__,
					"Bad \"text_rsp.final\": %u != 0.\n",
					text_rsp.final);
				TI_ERROR;
			}
			/*
			 * Copy response text into text_cmd->text and
			 * update the length text_cmd->length.  This
			 * will be sent out on the next text command. 
			 * */

			PARAM_TEXT_PARSE(l, &sess->sess_params.cred, text_out, len_out, NULL, NULL, TEXT_RESPONSE_TEXT_LEN, 1, TI_ERROR);

			iscsi_trace(TRACE_ISCSI_PARAM, "need to send %d bytes response back to target\n", len_out);
			text_cmd->length = len_out;
			memcpy(text_cmd->text, text_out, (size_t)len_out);
		} else {
			text_cmd->length = 0;
		}
	}
	text_cmd->final = text_rsp.final;

	/* Issue callback */

	iscsi_trace(TRACE_ISCSI_DEBUG, "iscsi_text_cmd_args_t done\n");
callback:
	if (cmd->status == -1)
		ret = -1;
	if (cmd->callback(cmd) != 0) {
		ret = -1;
		iscsi_err(__FILE__, __LINE__, "callback() failed\n");
	}
	TI_CLEANUP;
	return ret;
}

#define LOGIN_RESPONSE_TEXT_LEN	2048

static int 
login_response_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_login_cmd_args_t *login_cmd;
	iscsi_login_rsp_args_t login_rsp;
	iscsi_parameter_t *l = sess->params;
	char           *text_in = NULL;
	char           *text_out = NULL;
	int             len_in = 0;
	int             len_out = 0;

	if ((text_out = iscsi_malloc_atomic(LOGIN_RESPONSE_TEXT_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		cmd->status = -1;
		goto callback;
	}
#define LIR_CLEANUP {if (text_in != NULL) iscsi_free_atomic(text_in); if (text_out != NULL) iscsi_free_atomic(text_out);}
#define LIR_ERROR {cmd->status=-1; goto callback;}
	if (cmd) {
		login_cmd = (iscsi_login_cmd_args_t *) cmd->ptr;
	} else {
		iscsi_err(__FILE__, __LINE__, "no initiator_cmd_t specified for iscsi_login_cmd_args_t??\n");
		LIR_ERROR;
	}

	/* Read login response */

	if (iscsi_login_rsp_decap(header, &login_rsp) != 0) {
		iscsi_err(__FILE__, __LINE__, "login_response_decap() failed\n");
		LIR_ERROR;
	}
	if (login_rsp.length > 8192) {
		iscsi_err(__FILE__, __LINE__, "login_rsp.length %u\n",
			login_rsp.length);
		LIR_CLEANUP;
		return -1;
	}

	/* Read & parse text response */
	if ((len_in = login_rsp.length) != 0) {
		if ((text_in = iscsi_malloc_atomic((unsigned)len_in + 1)) == NULL) {
			iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
			LIR_ERROR;
		}
		if (iscsi_sock_msg(sess->sock, 0, (unsigned)len_in, text_in, 0) != len_in) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			LIR_ERROR;
		}
		text_in[len_in] = 0x0;
		PARAM_TEXT_PARSE(l, &sess->sess_params.cred, text_in, len_in, text_out, &len_out, LOGIN_RESPONSE_TEXT_LEN, 0, LIR_ERROR);
		if (login_rsp.transit && len_out != 0) {
			iscsi_warn(__FILE__, __LINE__,
				  "Bad \"len_out\": Got %u expected %u.\n",
				  len_out, 0);
		}
	}

	/* Check args */
	if (login_rsp.status_class != 0) {
		iscsi_err(__FILE__, __LINE__, "Bad Status-Class: got %d, expected %d\n", login_rsp.status_class, 0);
		LIR_ERROR;
	}
	if (login_rsp.tag != login_cmd->tag) {
		iscsi_err(__FILE__, __LINE__, "Bad Tag: got %x, expected %x\n", login_rsp.tag, login_cmd->tag);
		LIR_ERROR;
	}
	sess->ExpStatSN = login_rsp.StatSN + 1;


	if (login_rsp.transit) {

		if (login_cmd->transit != 1)
			iscsi_warn(__FILE__, __LINE__, "incoming packet transit bit not set, csg = %d, nsg = %d\n",
				      login_cmd->csg, login_cmd->nsg);

		switch (login_rsp.nsg) {
		case ISCSI_LOGIN_STAGE_NEGOTIATE:
			login_cmd->csg = login_cmd->nsg;
			login_cmd->nsg = ISCSI_LOGIN_STAGE_FULL_FEATURE;
			if (params_out(sess, text_out, &len_out, LOGIN_RESPONSE_TEXT_LEN, SESS_TYPE_NONE, /*LINTED*/!IS_SECURITY) != 0) {
				iscsi_err(__FILE__, __LINE__, "params_out() failed\n");
				LIR_ERROR;
			}
			login_cmd->length = len_out;
			(void) memcpy(login_cmd->text, text_out,
						(size_t)len_out);
			break;

		case ISCSI_LOGIN_STAGE_FULL_FEATURE:
			/* Check post conditions */

			if (login_rsp.tsih == 0) {
				iscsi_err(__FILE__, __LINE__,
				"Bad \"TSIH\": %u == 0.\n", login_rsp.tsih);
				LIR_ERROR;
			}
			if (login_rsp.isid != login_cmd->isid) {
				iscsi_err(__FILE__, __LINE__,
					"Bad \"ISID\": %uu != %uu.\n",
					(unsigned)login_rsp.isid,
					(unsigned)login_cmd->isid);
				LIR_ERROR;
			}
			if (login_rsp.ExpCmdSN != login_cmd->CmdSN) {
				iscsi_err(__FILE__, __LINE__,
					"Bad \"ExpCmdSN\": %u != %u.\n",
					(unsigned)login_rsp.ExpCmdSN,
					(unsigned)login_cmd->CmdSN);
				LIR_ERROR;
			}
			if (login_rsp.ExpCmdSN > login_rsp.MaxCmdSN) {
				iscsi_err(__FILE__, __LINE__,
					"Bad \"MaxCmdSN\": %u > %u.\n",
					(unsigned)login_rsp.ExpCmdSN,
					(unsigned)login_rsp.MaxCmdSN);
				LIR_ERROR;
			}

			/* Set remaining session parameters */

			sess->CmdSN = login_rsp.ExpCmdSN;
			sess->MaxCmdSN = login_rsp.MaxCmdSN;
			sess->tsih = login_rsp.tsih;
			sess->isid = login_rsp.isid;

			if (param_equiv(sess->params, "SessionType", "Normal")) {
				sess->state = INITIATOR_SESSION_STATE_LOGGED_IN_NORMAL;
			} else if (param_equiv(sess->params, "SessionType", "Discovery")) {
				sess->state = INITIATOR_SESSION_STATE_LOGGED_IN_DISCOVERY;
			} else {
				iscsi_err(__FILE__, __LINE__, "Unknown SessionType \"%s\"\n", param_val(sess->params, "SessionType"));
				LIR_ERROR;
			}

			iscsi_trace(TRACE_ISCSI_DEBUG, "*********************************************\n");
			iscsi_trace(TRACE_ISCSI_DEBUG, "*              LOGIN SUCCESSFUL             *\n");
			iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "CID", sess->cid);
			iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20" PRIu64 " *\n", "ISID", sess->isid);
			iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "TSIH", sess->tsih);
			iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "CmdSN", sess->CmdSN);
			iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "MaxCmdSN", sess->MaxCmdSN);
			iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "ExpStatSN", sess->ExpStatSN);
			iscsi_trace(TRACE_ISCSI_DEBUG, "*********************************************\n");
			break;
		default:
			LIR_ERROR;
		}
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG, "received partial login response\n");

		/* Copy response text into login_cmd->text and update the */
		/* length login_cmd->length.  This will be sent out on the */
		/* next login command. */

		if (len_out) {
			PARAM_TEXT_PARSE(l, &sess->sess_params.cred, text_out, len_out, NULL, NULL, 0, 1, LIR_ERROR);
			iscsi_trace(TRACE_ISCSI_PARAM, "need to send %d bytes response back to target\n", len_out);

			login_cmd->length = len_out;
			memcpy(login_cmd->text, text_out, (size_t)len_out);
			if (strncmp(text_out, "CHAP_N=", strlen("CHAP_N=")) == 0) {
				login_cmd->nsg = ISCSI_LOGIN_STAGE_NEGOTIATE;
				login_cmd->transit = 1;
			}
		} else {
			login_cmd->length = 0;
		}
	}

	/* Callback */

callback:
	iscsi_trace(TRACE_ISCSI_DEBUG, "iscsi_login_cmd_args_t done (cmd status %d, iscsi status %d)\n",
	      cmd->status, login_rsp.status_class);
	if ((*cmd->callback)(cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "callback() failed\n");
		LIR_CLEANUP;
		return -1;
	}
	LIR_CLEANUP;
	return 0;
}

static int 
logout_command_i(initiator_cmd_t * cmd)
{
	iscsi_logout_cmd_args_t	*logout_cmd;
	initiator_session_t	*sess;
	uint8_t			 header[ISCSI_HEADER_LEN];

	logout_cmd = (iscsi_logout_cmd_args_t *) cmd->ptr;
	sess = g_target[(int)cmd->isid].sess;
	/*
	 * Insert cmd into the hash table, keyed by the tag. The Rx thread
	 * will
	 */
	/* retrieve the cmd ptr using the tag from the response PDU. */

	if (hash_insert(&g_tag_hash, cmd, logout_cmd->tag) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_insert() failed\n");
		return -1;
	}
	/* Send logout command PDU */

	iscsi_trace(TRACE_ISCSI_DEBUG, "sending logout command\n");
	if (iscsi_logout_cmd_encap(header, logout_cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_logout_cmd_encap() failed\n");
		return -1;
	}
	if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed.\n");
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "logout command sent ok\n");

	return 0;
}


static int 
logout_response_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_logout_cmd_args_t *logout_cmd;
	iscsi_logout_rsp_args_t logout_rsp;

#define LOR_ERROR {cmd->status=-1; goto callback;}
	if (cmd) {
		if (cmd->ptr) {
			logout_cmd = (iscsi_logout_cmd_args_t *) cmd->ptr;
		} else {
			iscsi_err(__FILE__, __LINE__, "no iscsi_logout_cmd_args_t specified for initiator_cmd_t??\n");
			LOR_ERROR;
		}
	} else {
		iscsi_err(__FILE__, __LINE__, "no initiator_cmd_t specified for iscsi_logout_cmd_args_t??\n");
		return -1;
	}
	if (iscsi_logout_rsp_decap(header, &logout_rsp) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_logout_rsp_decap() failed\n");
		LOR_ERROR;
	}
	if (logout_rsp.response != ISCSI_LOGOUT_STATUS_SUCCESS) {
		iscsi_err(__FILE__, __LINE__, "Bad \"Response\": Got %u\n",
				logout_rsp.response);
		LOR_ERROR;
	}
	if (logout_rsp.tag != logout_cmd->tag) {
		iscsi_err(__FILE__, __LINE__, "Bad \"Tag\": Got %u\n",
				logout_rsp.tag);
		LOR_ERROR;
	}

	/* Check and update numbering */
	if (logout_rsp.StatSN != sess->ExpStatSN) {
		iscsi_err(__FILE__, __LINE__,
			"Bad \"StatSN\": Got %u, needed %u\n",
			logout_rsp.StatSN, sess->ExpStatSN);
		LOR_ERROR;
	}
	sess->ExpStatSN += 1;
	if (logout_rsp.ExpCmdSN != sess->CmdSN) {
		iscsi_err(__FILE__, __LINE__,
			"Bad \"ExpCmdSN\": Got %u, needed %u\n",
			logout_rsp.ExpCmdSN, sess->CmdSN);
		LOR_ERROR;
	}
	sess->MaxCmdSN = logout_rsp.MaxCmdSN;

	/* Callback */
	cmd->status = 0;
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"LOGOUT_CMD_T done (cmd status %d, iscsi status %d)\n",
		cmd->status, logout_rsp.response);
callback:
	if ((*cmd->callback)(cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "callback() failed\n");
		return -1;
	}

	iscsi_trace(TRACE_ISCSI_DEBUG, "*********************************************\n");
	iscsi_trace(TRACE_ISCSI_DEBUG, "*             LOGOUT SUCCESSFUL             *\n");
	iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "CID", sess->cid);
	iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20" PRIu64 " *\n", "ISID", sess->isid);
	iscsi_trace(TRACE_ISCSI_DEBUG, "* %20s:%20u *\n", "TSIH", sess->tsih);
	iscsi_trace(TRACE_ISCSI_DEBUG, "*********************************************\n");

	return 0;
}

static int 
nop_out_i(initiator_cmd_t * cmd)
{
	uint8_t   header[ISCSI_HEADER_LEN];
	iscsi_nop_out_args_t *nop_out;
	initiator_session_t *sess;
	int             rc, length;

	nop_out = cmd->ptr;
	sess = g_target[(int)cmd->isid].sess;
	length = nop_out->length;
	if (nop_out->tag != 0xffffffff) {

		/*
		 * Insert cmd into the hash table, keyed by
		 * nop_out->tag.  Upon receipt of the NOP_IN_T, the Rx
		 * thread will retrieve the cmd ptr using the tag from
		 * the NOP_IN_T PDU.  */

		if (hash_insert(&g_tag_hash, cmd, nop_out->tag) != 0) {
			iscsi_err(__FILE__, __LINE__, "hash_insert() failed\n");
			return -1;
		}
	}
	/* Encapsulate and send NOP */

	nop_out->ExpStatSN = sess->ExpStatSN;
	nop_out->immediate = 1;
	nop_out->CmdSN = sess->CmdSN;
	nop_out->transfer_tag = 0xffffffff;
	if (iscsi_nop_out_encap(header, nop_out) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_nop_out_encap() failed\n");
		return -1;
	}
	/*
	 * We need to make a copy of nop_out->length and save in the
	 * variable length.  Otherwise, we may get a seg fault - as if
	 * this is a NOP_OUT without ping, the Tx thread will issue
	 * the callback function immediately after we return - thereby
	 * de-allocating the NOP_OUT and initiator command structures. 
	 * */

	if ((rc = iscsi_sock_send_header_and_data(sess->sock, header,
			ISCSI_HEADER_LEN, nop_out->data, (unsigned)length,
			0)) != ISCSI_HEADER_LEN + length) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed: got %d expected %d\n", rc, ISCSI_HEADER_LEN + length);
		return -1;
	}
	cmd->status = 0;
	return 0;
}

static int 
scsi_command_i(initiator_cmd_t * cmd)
{
	iscsi_scsi_cmd_args_t *scsi_cmd;
	uint8_t   header[ISCSI_HEADER_LEN];
	uint64_t target;
	initiator_session_t *sess;
	iscsi_write_data_t data;
	struct iovec    sg_singleton;
	struct iovec   *sg, *sg_copy, *sg_copy_orig, *sg_which;
	int             sg_len, sg_len_copy, sg_len_which;
	int             fragment_flag;

	scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr;
	target = cmd->isid;
	sess = g_target[(int)target].sess;
	fragment_flag = 0;
	sg = sg_copy = sg_copy_orig = sg_which = NULL;
	sg_len = sg_len_copy = sg_len_which = 0;
	scsi_cmd->status = 0;

	iscsi_trace(TRACE_ISCSI_DEBUG, "tx_worker[%" PRIu64 "]: scsi op %#x lun %" PRIu64 " trans_len %d length %d send_sg_len %d recv_sg_len %d\n", target, scsi_cmd->cdb[0], scsi_cmd->lun, scsi_cmd->trans_len, scsi_cmd->length, scsi_cmd->send_sg_len, scsi_cmd->recv_sg_len);

	if ((uint32_t)target > CONFIG_INITIATOR_NUM_TARGETS) {
		iscsi_err(__FILE__, __LINE__, "target %u\n",
				(uint32_t)target);
		NO_CLEANUP;
		return -1;
	}

	/* Set and check scsi_cmd */
	if (scsi_cmd->trans_len > sess->sess_params.max_burst_length) {
		iscsi_err(__FILE__, __LINE__, "scsi_cmd->trans_len (%u) > MaxBurstLength (%u)\n",
		   scsi_cmd->trans_len, sess->sess_params.max_burst_length);
		return -1;
	}
	if (scsi_cmd->length > scsi_cmd->trans_len) {
		iscsi_err(__FILE__, __LINE__, "scsi_cmd->length (%u) > scsi_cmd->trans_len (%u)\n",
			    scsi_cmd->length, scsi_cmd->trans_len);
		return -1;
	}
	scsi_cmd->ExpStatSN = sess->ExpStatSN;
	scsi_cmd->CmdSN = sess->CmdSN;
	scsi_cmd->bytes_sent = scsi_cmd->bytes_recv = 0;

	/* Always use iovec for data */

	if (scsi_cmd->output) {
		if (scsi_cmd->send_sg_len) {	/* Data already an iovec */
			sg = (struct iovec *)(void *)scsi_cmd->send_data;
			sg_len = scsi_cmd->send_sg_len;
		} else {	/* Make iovec for data */
			sg_singleton.iov_base = scsi_cmd->send_data;
			sg_singleton.iov_len = scsi_cmd->trans_len;
			sg = &sg_singleton;
			sg_len = 1;
		}
	}
	/*
	 * Insert cmd into the hash table, keyed by scsi_cmd->tag.  The Rx
	 * thread will
	 */
	/* retrieve the cmd ptr using the tag from the response PDU. */

	if (hash_insert(&g_tag_hash, cmd, scsi_cmd->tag) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_insert() failed\n");
		goto error;
	}
	/* Send command PDU */

	if (scsi_cmd->output && sess->sess_params.immediate_data) {
		if (sess->sess_params.max_dataseg_len) {
			scsi_cmd->length = MIN(sess->sess_params.max_dataseg_len,
					       scsi_cmd->trans_len);
		} else {
			scsi_cmd->length = scsi_cmd->trans_len;
		}
		if (scsi_cmd->length == scsi_cmd->trans_len)
			scsi_cmd->final = 1;
	} else {
		scsi_cmd->length = 0;
		scsi_cmd->final = 1;
	}
	if (iscsi_scsi_cmd_encap(header, scsi_cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_scsi_cmd_encap() failed\n");
		goto error;
	}
	/*
	 * If we're sending any immediate data, we need to make a new
	 * iovec that contains only the immediata data (a subset of
	 * the original iovec).  */
	iscsi_trace(TRACE_ISCSI_DEBUG, "sending command PDU with %u bytes immediate data\n", scsi_cmd->length);
	if (scsi_cmd->length && sess->sess_params.immediate_data) {
		if ((sg_copy = iscsi_malloc_atomic(sg_len * sizeof(struct iovec))) == NULL) {
			iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
			goto error;
		}
		fragment_flag++;
		sg_copy_orig = sg_copy;
		memcpy(sg_copy, sg, sizeof(struct iovec) * sg_len);
		sg_len_copy = sg_len;
		if (modify_iov(&sg_copy, &sg_len_copy, 0, scsi_cmd->length) != 0) {
			iscsi_err(__FILE__, __LINE__, "modify_iov() failed\n");
			goto error;
		}
		if (scsi_cmd->ahs) {
			if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
				goto error;
			}
			if (iscsi_sock_msg(sess->sock, 1, (unsigned)scsi_cmd->ahs_len, scsi_cmd->ahs, 0) != scsi_cmd->ahs_len) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
				goto error;
			}
			if ((unsigned)iscsi_sock_msg(sess->sock, 1, scsi_cmd->length, sg_copy, sg_len_copy) != scsi_cmd->length) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
				goto error;
			}
		} else {
			if ((unsigned)iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, sg_copy, scsi_cmd->length, sg_len_copy)
			    != ISCSI_HEADER_LEN + scsi_cmd->length) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
				goto error;
			}
		}
		scsi_cmd->bytes_sent += scsi_cmd->length;
	} else {
		if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0) != ISCSI_HEADER_LEN) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			goto error;
		}
		if (scsi_cmd->ahs_len) {
			if (iscsi_sock_msg(sess->sock, 1, (unsigned)scsi_cmd->ahs_len, scsi_cmd->ahs, 0) != scsi_cmd->ahs_len) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
				goto error;
			}
		}
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "command PDU sent with %u bytes immediate data (%u bytes AHS)\n", scsi_cmd->length, scsi_cmd->ahs_len);

	/*
	 * Send data PDUS if 1) we're not in R2T mode and 2) we
	 * haven't sent everything as immediate data and 3) we have
	 * not reached the first burst when sending immediate data
	 */
	if (scsi_cmd->output
	    && (!sess->sess_params.initial_r2t)
	    && (scsi_cmd->bytes_sent != scsi_cmd->trans_len)
	    && ((!sess->sess_params.first_burst_length)
	|| (scsi_cmd->bytes_sent < sess->sess_params.first_burst_length))) {

		uint32_t        DataSN = 0;

		iscsi_trace(TRACE_ISCSI_DEBUG, "preparing to send %d bytes write data\n", scsi_cmd->trans_len - scsi_cmd->bytes_sent);

		do {
			(void) memset(&data, 0x0, sizeof(data));

			/*
			 * Take into account that MaxRecvPDULength and
			 * FirstBurstLength could both be "0" (no limit)
			 */
			if (sess->sess_params.max_dataseg_len) {
				if (sess->sess_params.first_burst_length) {
					data.length = MIN_3(
							    sess->sess_params.first_burst_length - scsi_cmd->bytes_sent,
							    sess->sess_params.max_dataseg_len,
							    scsi_cmd->trans_len - scsi_cmd->bytes_sent);
				} else {
					data.length = MIN(
							  sess->sess_params.max_dataseg_len,
							  scsi_cmd->trans_len - scsi_cmd->bytes_sent);
				}
			} else {
				if (sess->sess_params.first_burst_length) {
					data.length = MIN(
							  sess->sess_params.first_burst_length - scsi_cmd->bytes_sent,
							  scsi_cmd->trans_len - scsi_cmd->bytes_sent);
				} else {
					data.length = scsi_cmd->trans_len - scsi_cmd->bytes_sent;
				}
			}
#define FRAG_CLEANUP {if (fragment_flag) iscsi_free_atomic(sg_copy);}

			if (data.length == 0) {
				 iscsi_err(__FILE__, __LINE__,
				 		"Zero data.length\n");
				 FRAG_CLEANUP;
				 return -1;
			}

			if (scsi_cmd->bytes_sent + data.length ==
						scsi_cmd->trans_len) {
				data.final = 1;
			}
			data.tag = scsi_cmd->tag;
			data.transfer_tag = 0xffffffff;
			data.ExpStatSN = sess->ExpStatSN;
			data.DataSN = DataSN++;
			data.offset = scsi_cmd->bytes_sent;

			if (iscsi_write_data_encap(header, &data) != 0) {
				iscsi_err(__FILE__, __LINE__, "iscsi_write_data_encap() failed\n");
				goto error;
			}
			if (data.length != scsi_cmd->trans_len) {

				/*
				 * Make copy of iovec and modify with offset
				 * and length
				 */

				if (!fragment_flag) {
					if ((sg_copy = iscsi_malloc_atomic(sg_len * sizeof(struct iovec))) == NULL) {
						iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
						goto error;
					}
					sg_copy_orig = sg_copy;
					fragment_flag++;
				}
				sg_copy = sg_copy_orig;
				memcpy(sg_copy, sg, sizeof(struct iovec) * sg_len);
				sg_len_copy = sg_len;
				if (modify_iov(&sg_copy, &sg_len_copy, scsi_cmd->bytes_sent, data.length) != 0) {
					iscsi_err(__FILE__, __LINE__, "modify_iov() failed\n");
					goto error;
				}
				sg_which = sg_copy;
				sg_len_which = sg_len_copy;

			} else {

				/*
				 * Data was not fragmented; use the original
				 * iovec.
				 */

				sg_which = sg;
				sg_len_which = sg_len;
			}

			iscsi_trace(TRACE_ISCSI_DEBUG, "sending write data PDU (offset %u, len %u, sg_len %u)\n",
			      data.offset, data.length, sg_len_which);

			if ((unsigned)iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, sg_which, data.length, sg_len_which)
			    != ISCSI_HEADER_LEN + data.length) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
				goto error;
			}
			iscsi_trace(TRACE_ISCSI_DEBUG, "sent write data PDU (offset %u, len %u)\n", data.offset, data.length);
			scsi_cmd->bytes_sent += data.length;
		} while ((scsi_cmd->bytes_sent < scsi_cmd->trans_len)
			 && ((scsi_cmd->bytes_sent < sess->sess_params.first_burst_length)
			     || (!sess->sess_params.first_burst_length)));
		if (scsi_cmd->trans_len - scsi_cmd->bytes_sent) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "REACHED FIRST BURST\n");
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "successfully sent %u of %u bytes write data\n", scsi_cmd->bytes_sent, scsi_cmd->trans_len);
	}
	if (scsi_cmd->output && (scsi_cmd->trans_len - scsi_cmd->bytes_sent)) {
		iscsi_trace(TRACE_ISCSI_DEBUG, "expecting R2T for remaining %u bytes write data\n", scsi_cmd->trans_len - scsi_cmd->bytes_sent);
	}
	if (fragment_flag)
		iscsi_free_atomic(sg_copy_orig);
	sess->CmdSN++;

	return 0;

error:
	if (fragment_flag)
		iscsi_free_atomic(sg_copy);
	return -1;
}

static int 
reject_i(initiator_session_t * sess, uint8_t *header)
{
	initiator_cmd_t *cmd = NULL;
	iscsi_reject_t  reject;
	uint8_t   bad_header[ISCSI_HEADER_LEN];
	uint32_t        tag;

	/* Get & check args */

	if (iscsi_reject_decap(header, &reject) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_reject_decap() failed\n");
		return -1;
	}
	if (reject.length != ISCSI_HEADER_LEN) {
		iscsi_err(__FILE__, __LINE__, "reject.length %u\n",
				reject.length);
		NO_CLEANUP;
		return -1;
	}

	/* Read bad header, extract tag, and get cmd from hash table */

	if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, bad_header, 0) != ISCSI_HEADER_LEN) {
		iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
		return -1;
	}
	(void) memcpy(&tag, bad_header + 16, sizeof(tag));
	tag = ISCSI_NTOHL(tag);
	iscsi_err(__FILE__, __LINE__, "REJECT PDU: tag %#x (reason %#x)\n", tag, reject.reason);
	if (tag != 0xffffffff) {
		if ((cmd = hash_remove(&g_tag_hash, tag)) == NULL) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "no cmd ptr associated with tag %#x\n", tag);
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG, "cmd %p associated with tag %#x\n", cmd, tag);
			ISCSI_LOCK(&sess->rx_worker.work_mutex, return -1);
			if (!cmd->tx_done)
				ISCSI_WAIT(&sess->rx_worker.work_cond, &sess->rx_worker.work_mutex, return -1);
			ISCSI_UNLOCK(&sess->rx_worker.work_mutex, return -1);
		}
	} else {
		iscsi_err(__FILE__, __LINE__, "no command associated with tag %#x\n", tag);
	}

	/* Execute callback to complete initiator_cmd_t  */

	if (cmd) {
		cmd->status = -1;
		if (cmd->callback) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "issuing callback for cmd associated with tag %#x\n", tag);
			if ((*cmd->callback)(cmd) != 0) {
				iscsi_err(__FILE__, __LINE__, "callback() failed\n");
				return -1;
			}
		} else {
			iscsi_err(__FILE__, __LINE__, "no callback associated with tag %#x\n", tag);
		}
	}
	return 0;
}

static int 
async_msg_i(initiator_session_t * sess, uint8_t *header)
{
	iscsi_async_msg_t    msg;

	/* Get & check args */
	if (iscsi_amsg_decap(header, &msg) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_amsg_decap() failed\n");
		return -1;
	}
	sess->CmdSN = msg.ExpCmdSN;
	sess->MaxCmdSN = msg.MaxCmdSN;
	sess->ExpStatSN = msg.StatSN + 1;

	/* Read Sense Data */
	if (msg.length) {
		uint8_t  *sense_data = NULL;
		if ((sense_data = iscsi_malloc(msg.length)) == NULL) {
			iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "reading %d bytes sense data \n", msg.length);
		if ((unsigned)iscsi_sock_msg(sess->sock, 0, msg.length, sense_data, 0) != msg.length) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
			if (sense_data != NULL)
				iscsi_free(sense_data);
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "read %d bytes sense data ok (currently discarding)\n", msg.length);
		if (sense_data != NULL)
			iscsi_free(sense_data);
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG, "no sense data available\n");
	}

	switch (msg.AsyncEvent) {
	case 0:
		/* Ignore SCSI asyn messages for now */
		break;
	case 1:
	case 4:
		/* Ignore Parameter Negotiation. Send Logout */
		logout_phase_i(sess);
		/* FALLTHROUGH */
	case 2:
	case 3:
		if (iscsi_sock_shutdown(sess->sock, 1) != 0) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_shutdown() failed\n");
		}
		return -1;
	case 255:
		break;
	default:
		break;
	}

	return 0;
}

static int 
nop_in_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_nop_out_args_t *nop_out = NULL;
	iscsi_nop_in_args_t  nop_in;
	uint8_t  *ping_data = NULL;
	unsigned             i;

	if (cmd) {
		nop_out = (iscsi_nop_out_args_t *) cmd->ptr;
	} else {
		iscsi_err(__FILE__, __LINE__, "no initiator_cmd_t associated with this NOP_IN\n");
	}
	if (iscsi_nop_in_decap(header, &nop_in) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_nop_in() failed\n");
		return -1;
	}
	if (nop_in.transfer_tag == 0xffffffff) {
		if (nop_in.length != 0) {
			iscsi_err(__FILE__, __LINE__,
				"nop_in.length %u not 0\n",
				nop_in.length);
			NO_CLEANUP;
			return -1;
		}
		return 0;
	}
	if (cmd) {
#if 0
		RETURN_NOT_EQUAL("nop_in.length", nop_in.length, nop_out->length, NO_CLEANUP, -1);
#else
		if (nop_in.length != nop_out->length) {
			iscsi_err(__FILE__, __LINE__,
				"nop_in.length %u, nopout->length %u\n",
				nop_in.length, nop_out->length);
			NO_CLEANUP;
			return -1;
		}
#endif
	}
	if (nop_in.length) {
		iscsi_trace(TRACE_ISCSI_DEBUG,
				"reading %d bytes ping data\n", nop_in.length);
		if ((ping_data = iscsi_malloc_atomic(nop_in.length)) == NULL) {
			iscsi_err(__FILE__, __LINE__,
				"iscsi_malloc_atomic() failed\n");
			return -1;
		}
#define NOI_CLEANUP {if (ping_data) iscsi_free_atomic(ping_data);}
#define NOI_ERROR {NOI_CLEANUP; return -1;}
		if ((unsigned)iscsi_sock_msg(sess->sock, 0, nop_in.length,
				ping_data, 0) != nop_in.length) {
			iscsi_err(__FILE__, __LINE__,
					"iscsi_sock_msg() failed\n");
			NOI_ERROR;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"successfully read %d bytes ping data\n",
			nop_in.length);
		if (cmd) {
			for (i = 0; i < nop_in.length; i++) {
				if (nop_out->data[i] != ping_data[i]) {
					iscsi_err(__FILE__, __LINE__,
						"Bad ping data[%d]. "
						"Got %#x, expected %#x\n",
						i, ping_data[i],
						nop_out->data[i]);
					NOI_ERROR;
				}
			}
		}
	}

	/* Send ping response (if initiated by target) */
	if (nop_in.transfer_tag != 0xffffffff) {
		uint8_t   nop_header[ISCSI_HEADER_LEN];
		iscsi_nop_out_args_t nop_out_args;

		iscsi_trace(TRACE_ISCSI_DEBUG,
			"sending %d byte ping response\n", nop_in.length);
		(void) memset(&nop_out_args, 0x0, sizeof(nop_out_args));
		nop_out_args.tag = 0xffffffff;
		nop_out_args.immediate = 0x40;
		nop_out_args.transfer_tag = nop_in.transfer_tag;
		nop_out_args.length = nop_in.length;
		nop_out_args.lun = nop_in.lun;
		nop_out_args.ExpStatSN = sess->ExpStatSN;
		nop_out_args.CmdSN = sess->CmdSN;
		if (iscsi_nop_out_encap(nop_header, &nop_out_args) != 0) {
			iscsi_err(__FILE__, __LINE__,
				"iscsi_nop_out_encap() failed\n");
			NOI_ERROR;
		}
		if ((unsigned)iscsi_sock_send_header_and_data(sess->sock,
				nop_header, nop_out_args.length, ping_data,
				nop_in.length, 0) != nop_in.length) {
			iscsi_err(__FILE__, __LINE__,
					"iscsi_sock_msg() failed\n");
			NOI_ERROR;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"successfully sent %d byte ping response\n",
			nop_in.length);
	}
	NOI_CLEANUP;
	/* Check and update numbering  */
	sess->ExpStatSN = nop_in.StatSN + 1;
	/*
	 * RETURN_NOT_EQUAL("StatSN", nop_in.StatSN, sess->ExpStatSN++,
	 * NO_CLEANUP, -1);
	 */
	sess->CmdSN = nop_in.ExpCmdSN;
	/*
	 * RETURN_NOT_EQUAL("ExpCmdSN", nop_in.ExpCmdSN, sess->CmdSN,
	 * NO_CLEANUP, -1);
	 */
	sess->MaxCmdSN = nop_in.MaxCmdSN;

	/* Callback */

	if (cmd) {
		cmd->status = 0;
		if (cmd->callback) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "NOP_OUT_T done (cmd status %d)\n", cmd->status);
			if ((*cmd->callback)(cmd) != 0) {
				iscsi_err(__FILE__, __LINE__, "callback() failed\n");
				return -1;
			}
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG, "no callback associated with NOP_IN_T??\n");
			return -1;
		}
	}
	return 0;
}

static int 
scsi_r2t_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_r2t_t     r2t;
	iscsi_scsi_cmd_args_t *scsi_cmd;
	iscsi_write_data_t data;
	uint32_t        bytes_sent;
	uint32_t        DataSN;
	struct iovec    sg_singleton;
	struct iovec   *sg, *sg_copy, *sg_copy_orig, *sg_which;
	int             sg_len, sg_len_copy, sg_len_which;
	int             fragment_flag;

	/* Make sure an initiator_cmd_t was specified, that it has a
	 * callback function specified and that it also has a
	 * iscsi_scsi_cmd_args_t associated with it.  */

	if (cmd) {
		if ((scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr) == NULL) {
			iscsi_err(__FILE__, __LINE__, "no iscsi_scsi_cmd_args_t associated with this initiator_cmd_t??\n");
			return -1;
		} else if (cmd->callback == NULL) {
			iscsi_err(__FILE__, __LINE__, "no callback associated with this initiator_cmd_t??\n");
			return -1;
		}
	} else {
		iscsi_err(__FILE__, __LINE__, "no initiator_cmd_t associated with this iscsi_r2t_t??\n");
		return -1;
	}

	sg = sg_copy = sg_copy_orig = sg_which = NULL;
	sg_len = sg_len_copy = sg_len_which = 0;
	if (iscsi_r2t_decap(header, &r2t) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_r2t_decap() failed\n");
		return -1;
	}

	/* Check args */
	if (r2t.length == 0) {
		iscsi_err(__FILE__, __LINE__, "Zero r2t.length\n");
		NO_CLEANUP;
		return -1;
	}

	/* Check and update numbering */
#if 0
	RETURN_NOT_EQUAL("StatSN", r2t.StatSN, sess->ExpStatSN, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("ExpCmdSN", r2t.ExpCmdSN, sess->CmdSN, NO_CLEANUP, -1);
#else
	if (r2t.StatSN != sess->ExpStatSN) {
		iscsi_err(__FILE__, __LINE__,
				"r2t.StatSN %u, sess->ExpStatSN %u\n",
				r2t.StatSN, sess->ExpStatSN);
		NO_CLEANUP;
		return -1;
	}
	if (r2t.ExpCmdSN != sess->CmdSN) {
		iscsi_err(__FILE__, __LINE__,
				"r2t.ExpCmdSN %u, sess->CmdSN %u\n",
				r2t.ExpCmdSN, sess->CmdSN);
		NO_CLEANUP;
		return -1;
	}
#endif
	sess->MaxCmdSN = r2t.MaxCmdSN;

	/* Send back requested data */
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"sending %d bytes R2T write data (offset %u)\n",
		r2t.length, r2t.offset);
	if (scsi_cmd->send_sg_len) {
		sg = (struct iovec *)(void *)scsi_cmd->send_data;
		sg_len = scsi_cmd->send_sg_len;
	} else {
		sg_singleton.iov_base = scsi_cmd->send_data;
		sg_singleton.iov_len = scsi_cmd->trans_len;
		sg = &sg_singleton;
		sg_len = 1;
	}
	fragment_flag = 0;
	bytes_sent = 0;
	DataSN = 0;
#define FF_CLEANUP {if (fragment_flag) iscsi_free_atomic(sg_copy_orig);}
	do {
		(void) memset(&data, 0x0, sizeof(data));
		if (sess->sess_params.max_dataseg_len) {
			data.length = MIN(sess->sess_params.max_dataseg_len,
					  r2t.length - bytes_sent);
		} else {
			data.length = r2t.length - bytes_sent;
		}
		if (bytes_sent + data.length == r2t.length) {
			data.final = 1;
		}
		data.tag = r2t.tag;
		data.transfer_tag = r2t.transfer_tag;
		data.ExpStatSN = sess->ExpStatSN;
		data.DataSN = DataSN++;
		data.offset = r2t.offset + bytes_sent;
		data.lun = scsi_cmd->lun;
		if (iscsi_write_data_encap(header, &data) != 0) {
			iscsi_err(__FILE__, __LINE__, "iscsi_write_data_encap() failed\n");
			FF_CLEANUP;
			return -1;
		}
		if ((data.length < r2t.length) || (r2t.offset)) {
			if (data.length < r2t.length) {
				iscsi_trace(TRACE_ISCSI_DEBUG, "R2T data is being fragmented: sending %u bytes of %u requested\n",
				      data.length, r2t.length);
			} else {
				iscsi_trace(TRACE_ISCSI_DEBUG, "R2T data starts at offset %u, desired length %u\n",
				      r2t.offset, r2t.length);
			}

			/* Allocate space for a copy of the original iovec */

			if (!fragment_flag) {
				if ((sg_copy_orig = iscsi_malloc_atomic(sg_len * sizeof(struct iovec))) == NULL) {
					iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
					return -1;
				}
				fragment_flag++;
			}
			/*
			 * Copy and modify original iovec with new offset and
			 * length
			 */

			iscsi_trace(TRACE_ISCSI_DEBUG, "modifying original iovec with offset %u length %u\n",
			      r2t.offset + bytes_sent, data.length);
			sg_copy = sg_copy_orig;
			sg_len_copy = sg_len;
			memcpy(sg_copy, sg, sizeof(struct iovec) * sg_len);
			if (modify_iov(&sg_copy, &sg_len_copy, r2t.offset + bytes_sent, data.length) != 0) {
				iscsi_err(__FILE__, __LINE__, "modify_iov() failed\n");
				FF_CLEANUP;
				return -1;
			}
			sg_which = sg_copy;
			sg_len_which = sg_len_copy;
		} else {
			iscsi_trace(TRACE_ISCSI_DEBUG, "using original iovec for R2T transfer (offset %u, length %u)\n",
			      r2t.offset, r2t.length);
			sg_which = sg;
			sg_len_which = sg_len;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "sending R2T write data PDU (offset %u, len %u, sg_len %u)\n",
		      data.offset, data.length, sg_len_which);
		if ((unsigned)iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, sg_which, data.length, sg_len_which)
		    != ISCSI_HEADER_LEN + data.length) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_send_header_and_data() failed\n");
			FF_CLEANUP;
			return -1;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "sent write data PDU OK (offset %u, len %u)\n", data.offset, data.length);
		bytes_sent += data.length;
		scsi_cmd->bytes_sent += data.length;
	} while (bytes_sent < r2t.length);
	FF_CLEANUP;
	if (hash_insert(&g_tag_hash, cmd, scsi_cmd->tag) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_insert() failed\n");
		return -1;
	}
	return 0;
}

static int 
scsi_response_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_scsi_cmd_args_t	*scsi_cmd;
	iscsi_scsi_rsp_t	 scsi_rsp;
	const char		*errmsg;

	/* Make sure an initiator_cmd_t was specified, that it has a
	 * callback function specified and that it also has a
	 * iscsi_scsi_cmd_args_t associated with it.  */

	if (cmd) {
		if ((scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr) == NULL) {
			iscsi_err(__FILE__, __LINE__, "no iscsi_scsi_cmd_args_t associated with this initiator_cmd_t??\n");
			return -1;
		} else if (cmd->callback == NULL) {
			iscsi_err(__FILE__, __LINE__, "no callback associated with this initiator_cmd_t??\n");
			return -1;
		}
	} else {
		iscsi_err(__FILE__, __LINE__, "no initiator_cmd_t associated with this iscsi_scsi_rsp_t??\n");
		return -1;
	}

	/*
	 * Read SCSI response and check return args.  Those marked
	 * "FIX ME" are not yet implemented.  */

	if (iscsi_scsi_rsp_decap(header, &scsi_rsp) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_scsi_rsp_decap() failed\n");
		return -1;
	}
#if 0
	RETURN_NOT_EQUAL("o bit (FIX ME)", scsi_rsp.bidi_overflow, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("u bit (FIX ME)", scsi_rsp.bidi_underflow, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("O bit (FIX ME)", scsi_rsp.overflow, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("iSCSI Response (FIX ME)", scsi_rsp.response, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("Tag", scsi_rsp.tag, scsi_cmd->tag, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("Bidi Residual Count", scsi_rsp.bidi_res_cnt, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("StatSN", scsi_rsp.StatSN, sess->ExpStatSN, NO_CLEANUP, -1);
#else
	errmsg = NULL;
	if (scsi_rsp.bidi_overflow != 0) {
		errmsg = "o bit (FIX ME)\n";
	} else if (scsi_rsp.bidi_underflow != 0) {
		errmsg = "u bit (FIX ME)\n";
	} else if (scsi_rsp.overflow != 0) {
		errmsg = "O bit (FIX ME)\n";
	} else if (scsi_rsp.response != 0) {
		errmsg = "Response (FIX ME)\n";
	} else if (scsi_rsp.tag != scsi_cmd->tag) {
		errmsg = "Tags don't match\n";
	} else if (scsi_rsp.bidi_res_cnt != 0) {
		errmsg = "Bidi Residual Count";
	} else if (scsi_rsp.StatSN != sess->ExpStatSN) {
		errmsg = "StatSN";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return -1;
	}
#endif
	sess->ExpStatSN = scsi_rsp.StatSN + 1;

	if (sess->sess_params.max_dataseg_len && 
	    scsi_rsp.length > sess->sess_params.max_dataseg_len) {
		iscsi_err(__FILE__, __LINE__,
			"scsi_rsp.length %u\n", scsi_rsp.length);
		NO_CLEANUP;
		return -1;
	}
	if ((scsi_rsp.status == 0) && (scsi_rsp.length != 0)) {
		iscsi_err(__FILE__, __LINE__,
			"Unexpected DataSegmentLength %u "
			"with GOOD SCSI status\n", scsi_rsp.length);
		return -1;
	}
	/*
	 * Make sure all data was successfully transferred if command
	 * completed successfully, otherwise read sense data.  */

	if (scsi_rsp.status == 0) {
		if (scsi_cmd->output) {
#if 0
			RETURN_NOT_EQUAL("scsi_cmd->bytes_sent", scsi_cmd->bytes_sent, scsi_cmd->trans_len, NO_CLEANUP, -1);
#else
			if (scsi_cmd->bytes_sent != scsi_cmd->trans_len) {
				iscsi_err(__FILE__, __LINE__,
					"scsi_cmd->bytes_sent\n");
				NO_CLEANUP;
				return -1;
			}
#endif
			if (scsi_cmd->input) {

#if 0
				RETURN_NOT_EQUAL("scsi_cmd->bytes_recv", scsi_cmd->bytes_recv, scsi_cmd->bidi_trans_len, NO_CLEANUP, -1);
#else
				if (scsi_cmd->bytes_recv != scsi_cmd->bidi_trans_len) {
					iscsi_err(__FILE__, __LINE__,
						"scsi_cmd->bytes_recv\n");
					NO_CLEANUP;
					return -1;
				}
#endif
			}
		} else if (scsi_cmd->input) {


		}
	} else if (scsi_rsp.length) {
		uint8_t  *sense_data = NULL;

		if ((sense_data = iscsi_malloc(scsi_rsp.length)) == NULL) {
			iscsi_err(__FILE__, __LINE__,
				"iscsi_malloc() failed\n");
			return -1;
		}
		iscsi_err(__FILE__, __LINE__,
			"reading %d bytes sense data (recv_sg_len %u)\n",
			scsi_rsp.length, scsi_cmd->recv_sg_len);
		if ((unsigned)iscsi_sock_msg(sess->sock, 0, scsi_rsp.length,
				sense_data, 0) != scsi_rsp.length) {
			iscsi_err(__FILE__, __LINE__,
				"iscsi_sock_msg() failed\n");
			if (sense_data != NULL) {
				iscsi_free(sense_data);
			}
			return -1;
		}
		iscsi_err(__FILE__, __LINE__,
			"read %d bytes sense data ok (currently discarding)\n",
			scsi_rsp.length);
		if (sense_data != NULL) {
			iscsi_free(sense_data);
		}
	} else {
		iscsi_trace(TRACE_ISCSI_DEBUG, "no sense data available\n");
	}

	/* Check and update numbering  */

	/*
	 * RETURN_NOT_EQUAL("ExpCmdSN", scsi_rsp.ExpCmdSN, sess->CmdSN,
	 * NO_CLEANUP, -1);
	 */
	sess->MaxCmdSN = scsi_rsp.MaxCmdSN;

	/* Set initiator_cmd_t status, iscsi_scsi_cmd_args_t status  */
	/* and execute callback function */

	cmd->status = 0;
	scsi_cmd->status = scsi_rsp.status;
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"iscsi_scsi_cmd_args_t done (cmd status %d, iscsi status %d, "
		"scsi status %d)\n",
		cmd->status, scsi_rsp.response, scsi_rsp.status);
	if ((*cmd->callback)(cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "callback() failed\n");
		return -1;

	}
	return 0;
}

static int 
scsi_read_data_i(initiator_session_t * sess, initiator_cmd_t * cmd, uint8_t *header)
{
	iscsi_scsi_cmd_args_t	*scsi_cmd;
	iscsi_read_data_t	 data;
	const char 		*errmsg;
	int			 rc;

	iscsi_trace(TRACE_ISCSI_DEBUG, "processing read data\n");

	/* Make sure an initiator_cmd_t was specified, that it has a
	 * callback function specified and that it also has a
	 * iscsi_scsi_cmd_args_t associated with it.  */

	if (cmd) {
		if (cmd->type != ISCSI_SCSI_CMD) {
			iscsi_err(__FILE__, __LINE__,
				"Invalid response from target for cmd "
				"type (%#x)\n", cmd->type);
			cmd->status = -1;
			if (cmd->callback) {
				(*cmd->callback)(cmd);
			}
			return -1;
		}
		if ((scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr) == NULL) {
			iscsi_err(__FILE__, __LINE__,
				"no iscsi_scsi_cmd_args_t associated with "
				"this initiator_cmd_t??\n");
			return -1;
		} else if (cmd->callback == NULL) {
			iscsi_err(__FILE__, __LINE__,
				"no callback associated with this "
				"initiator_cmd_t??\n");
			return -1;
		}
	} else {
		iscsi_err(__FILE__, __LINE__,
			"no initiator_cmd_t associated with this "
			"iscsi_read_data_t??\n");
		return -1;
	}
	if (iscsi_read_data_decap(header, &data) != 0) {
		iscsi_err(__FILE__, __LINE__,
			"iscsi_scsi_rsp_decap() failed\n");
		return -1;
	}

	/* Check args */
#if 0
	RETURN_NOT_EQUAL("Overflow bit", data.overflow, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("Underflow bit", data.underflow, 0, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("Tag", data.task_tag, scsi_cmd->tag, NO_CLEANUP, -1);
	RETURN_NOT_EQUAL("Residual Count", data.res_count, 0, NO_CLEANUP, -1);
#else
	errmsg = NULL;
	if (data.overflow != 0) {
		errmsg = "Overflow bit";
	} else if (data.task_tag != scsi_cmd->tag) {
		errmsg = "Tag";
	} else if (!data.underflow) {
		if (data.res_count != 0) {
			errmsg = "Residual Count";
		}
	} else {
		iscsi_warn(__FILE__, __LINE__, "Underflow %" PRIu32 "\n", data.res_count);
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return -1;
	}
#endif

	if (sess->sess_params.max_dataseg_len) {
		if (data.length > sess->sess_params.max_dataseg_len) {
			iscsi_err(__FILE__, __LINE__,
				"data.length %u\n", data.length);
			NO_CLEANUP;
			return -1;
		}
	}

	/* Check and update numbering  */
	if (data.ExpCmdSN != sess->CmdSN) {
		iscsi_warn(__FILE__, __LINE__,
			"Bad \"ExpCmdSN\": Got %u expected %u.\n",
			data.ExpCmdSN, sess->CmdSN);
	}
	sess->MaxCmdSN = data.MaxCmdSN;

	/* Need to optimize this section */

	if (scsi_cmd->recv_sg_len) {
		int             sg_len = scsi_cmd->recv_sg_len;
		struct iovec   *sg;
		struct iovec   *sg_orig = NULL;
		char		*sgp;
		uint32_t        total_len, disp;
		int             i;

		if (data.length != scsi_cmd->trans_len) {

			/* Make a copy of the iovec */

			sg_orig = sg = iscsi_malloc_atomic(sizeof(struct iovec)
					* sg_len);
			if (sg_orig == NULL) {
				iscsi_err(__FILE__, __LINE__,
					"iscsi_malloc_atomic() failed\n");
				return -1;

			}
			(void) memcpy(sg, scsi_cmd->recv_data,
					sizeof(struct iovec) * sg_len);

			/* Find offset in iovecs */
			total_len = 0;
			disp = data.offset;
			for (i = 0; i < sg_len; i++) {
				total_len += sg[i].iov_len;
				if (total_len > data.offset) {
					break;
				}
				disp -= sg[i].iov_len;
			}
			sg[i].iov_len -= disp;
			sgp = sg[i].iov_base;
			sgp += disp;
			sg[i].iov_base = sgp;
			sg_len -= i;
			sg = &sg[i];

			/* Find last iovec needed for read */

			total_len = 0;
			for (i = 0; i < sg_len; i++) {
				total_len += sg[i].iov_len;
				if (total_len >= data.length) {
					break;
				}
			}
			sg[i].iov_len -= (total_len - data.length);
			sg_len = i + 1;
		} else {
			sg = (struct iovec *)(void *)scsi_cmd->recv_data;
		}
		iscsi_trace(TRACE_ISCSI_DEBUG, "reading %d bytes into sg buffer (total offset %u)\n", data.length, data.offset);
		if ((rc = iscsi_sock_msg(sess->sock, 0, data.length, (uint8_t *)(void *) sg, sg_len)) != (int)data.length) {
			iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed: got %u, expected %u\n", rc, data.length);
			if (sg_orig)
				iscsi_free_atomic(sg_orig);
			return -1;
		}
		scsi_cmd->bytes_recv += data.length;
		if (sg_orig)
			iscsi_free_atomic(sg_orig);
	} else {
		if (data.length) {
			iscsi_trace(TRACE_ISCSI_DEBUG, "reading %d bytes into dest buffer (offset %u)\n", data.length, data.offset);
			if (iscsi_sock_msg(sess->sock, 0, data.length, scsi_cmd->recv_data + data.offset, 0) != (int)data.length) {
				iscsi_err(__FILE__, __LINE__, "iscsi_sock_msg() failed\n");
				return -1;
			}
			scsi_cmd->bytes_recv += data.length;
		}
	}


	/* Check for status */

	if (data.S_bit) {
		iscsi_trace(TRACE_ISCSI_DEBUG,
				"received status with final PDU\n");
#if 0
		RETURN_NOT_EQUAL("Final Bit", data.final, 1, NO_CLEANUP, -1);
		RETURN_NOT_EQUAL("StatSN", data.StatSN, sess->ExpStatSN++, NO_CLEANUP, -1);
		/* XXX - agc - increment in macro !!! */
#else
		if (data.final != 1) {
			iscsi_err(__FILE__, __LINE__, "Final Bit");
			NO_CLEANUP;
			return -1;
		}
		if (data.StatSN != sess->ExpStatSN++) {
			iscsi_err(__FILE__, __LINE__, "StatSN");
			NO_CLEANUP;
			return -1;
		}
#endif
		scsi_cmd->status = data.status = 0;
		cmd->status = 0;
		iscsi_trace(TRACE_ISCSI_DEBUG,
			"scsi op %#x done (tag %u, status %d)\n",
			scsi_cmd->cdb[0], scsi_cmd->tag, scsi_cmd->status);
		if ((*cmd->callback)(cmd) != 0) {
			iscsi_err(__FILE__, __LINE__,
				"callback() failed\n");
			return -1;
		}
	} else {
		if (hash_insert(&g_tag_hash, cmd, scsi_cmd->tag) != 0) {
			iscsi_err(__FILE__, __LINE__,
				"hash_insert() failed\n");
			return -1;
		}
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "read data processed\n");
	return 0;
}

int 
iscsi_initiator_info(char *ptr, int size, int len)
{
	initiator_session_t	*sess;
	int			 i;

	ptr[0] = 0x0;
	len += snprintf(ptr, (size_t)(size - len),
		"  %3s %30s %25s\n\n", "TID", "TargetName", "TargetAddress");
	for (i = 0; i < CONFIG_INITIATOR_NUM_TARGETS; i++) {
		len += snprintf(ptr + len, (size_t)(size - len),
				"  %3i %30s %20s:%d (",
				i, g_target[i].TargetName,
				g_target[i].ip, g_target[i].port);
		if (g_target[i].has_session) {
			sess = g_target[i].sess;
			if (sess->state & INITIATOR_SESSION_STATE_INITIALIZING)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"initializing");
			if (sess->state & INITIATOR_SESSION_STATE_INITIALIZED)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"initialized");
			if (sess->state & INITIATOR_SESSION_STATE_CONNECTING)
				len += snprintf(ptr + len,
						(size_t)(size - len),
						"%s", "connecting");
			if (sess->state & INITIATOR_SESSION_STATE_CONNECTED)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"connected");
			if (sess->state & INITIATOR_SESSION_STATE_LOGGING_IN)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"logging in");
			if (sess->state &
				INITIATOR_SESSION_STATE_LOGGED_IN_NORMAL)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"Normal session");
			if (sess->state &
				INITIATOR_SESSION_STATE_LOGGED_IN_DISCOVERY)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"Discovery session");
			if (sess->state & INITIATOR_SESSION_STATE_LOGGING_OUT)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"logging out");
			if (sess->state & INITIATOR_SESSION_STATE_LOGGED_OUT)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"logged out");
			if (sess->state & INITIATOR_SESSION_STATE_DESTROYING)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						"destroying");
			if (sess->tx_worker.state & ISCSI_WORKER_STATE_ERROR)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						" **Tx Error** ");
			if (sess->rx_worker.state & ISCSI_WORKER_STATE_ERROR)
				len += snprintf(ptr + len,
						(size_t)(size - len), "%s",
						" **Rx Error** ");
		} else {
			len += snprintf(ptr + len, (size_t)(size - len), "%s",
					"No Session");
		}
		len += snprintf(ptr + len, (size_t)(size - len), ")\n");
	}
	return len;
}

int 
iscsi_initiator_discover(char *host, uint64_t target, int lun)
{
	iscsi_nop_out_args_t	discover_cmd;
	initiator_cmd_t		cmd;

	cmd.type = ISCSI_NOP_OUT;
	cmd.ptr = &discover_cmd;
	cmd.isid = target;
	(void) strlcpy(cmd.targetname, host, sizeof(cmd.targetname));
	(void) memset(&discover_cmd, 0x0, sizeof(iscsi_nop_out_args_t));
	discover_cmd.length = 1;
	discover_cmd.data = (const uint8_t *) "";
	discover_cmd.lun = lun;
	discover_cmd.tag = 0xffffffff;
	if (initiator_command(&cmd) != 0) {
		iscsi_err(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	return 0;
}

void
get_target_info(uint64_t target, initiator_target_t *ip)
{
	(void) memcpy(ip, &g_target[(int)target], sizeof(*ip));
}

int 
ii_initiator_init(const char *hostname, int port, int address_family, const char *user, char *lun, int auth_type, int mutual_auth, int digest_type)
{
	initiator_session_t *sess = NULL;

#define INIT_CLEANUP {if (sess != NULL) iscsi_free_atomic(sess);}
#define INIT_ERROR {INIT_CLEANUP; return -1;}

	USE_ARG(address_family);
	iscsi_trace(TRACE_ISCSI_DEBUG, "initializing initiator\n");
	iscsi_trace(TRACE_ISCSI_DEBUG, "target config filename to read from:%s\n", gfilename);
	if (get_target_config(hostname, port) != 0) {
		iscsi_err(__FILE__, __LINE__, "Error getting target configuration from config file\n");
		return -1;
	}
	(void) strlcpy(g_target[0].iqnwanted, lun, sizeof(g_target[0].iqnwanted));
	g_initiator_state = 0;
	if (iscsi_queue_init(&g_session_q, CONFIG_INITIATOR_MAX_SESSIONS) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		return -1;
	}
	if ((sess = iscsi_malloc_atomic(sizeof(initiator_session_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
	if (iscsi_queue_insert(&g_session_q, sess) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	if (user)
		sess->sess_params.cred.user = strdup(user);
	else
		sess->sess_params.cred.user = NULL;
	
	sess->sess_params.auth_type = auth_type;
	sess->sess_params.mutual_auth = mutual_auth;
	sess->sess_params.digest_wanted = digest_type;
	iscsi_trace(TRACE_ISCSI_DEBUG, "%d free sessions available\n",
			CONFIG_INITIATOR_MAX_SESSIONS);

	g_tag = 0xabc123;
	if (hash_init(&g_tag_hash, CONFIG_INITIATOR_QUEUE_DEPTH) != 0) {
		iscsi_err(__FILE__, __LINE__, "hash_init() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	iscsi_spin_init(&g_tag_spin);
	iscsi_trace(TRACE_ISCSI_DEBUG,
		"tag hash table initialized with queue depth %d\n",
		CONFIG_INITIATOR_QUEUE_DEPTH);

	/*
	 * Start enqueue worker.  This thread accepts scsi commands
	 * from initiator_enqueue() and queues them onto one of the tx
	 * worker queues.
	 */
	iscsi_trace(TRACE_ISCSI_DEBUG, "starting enqueue worker\n");
	if (iscsi_queue_init(&g_enqueue_q, CONFIG_INITIATOR_QUEUE_DEPTH) != 0) {
		iscsi_err(__FILE__, __LINE__, "iscsi_queue_init() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "about to initialize mutex\n");
	ISCSI_MUTEX_INIT(&g_enqueue_worker.work_mutex, INIT_ERROR);
	ISCSI_COND_INIT(&g_enqueue_worker.work_cond, INIT_ERROR);
	ISCSI_MUTEX_INIT(&g_enqueue_worker.exit_mutex, INIT_ERROR);
	ISCSI_COND_INIT(&g_enqueue_worker.exit_cond, INIT_ERROR);
	ISCSI_LOCK(&g_enqueue_worker.exit_mutex, INIT_ERROR);

	iscsi_trace(TRACE_ISCSI_DEBUG, "spawning thread for enqueue worker\n");
	if (iscsi_thread_create(&g_enqueue_worker.thread,
		(void *) &enqueue_worker_proc, &g_enqueue_worker) != 0) {
		iscsi_err(__FILE__, __LINE__,
				"iscsi_threads_create() failed\n");
		INIT_CLEANUP;
		return -1;
	}
	iscsi_trace(TRACE_ISCSI_DEBUG, "thread spawned, waiting for signal\n");
	ISCSI_WAIT(&g_enqueue_worker.exit_cond, &g_enqueue_worker.exit_mutex,
			INIT_ERROR);
	ISCSI_UNLOCK(&g_enqueue_worker.exit_mutex, INIT_ERROR);
	iscsi_trace(TRACE_ISCSI_DEBUG, "successfully started enqueue worker\n");

	iscsi_trace(TRACE_ISCSI_DEBUG, "initiator initialization complete\n");
	return 0;
}

int
iscsi_initiator_set_defaults(iscsi_initiator_t *ini)
{
	char	buf[32];

	/* set defaults */
	(void) memset(ini, 0x0, sizeof(*ini));
	iscsi_initiator_setvar(ini, "address family", "unspec");
	iscsi_initiator_setvar(ini, "digest type", "none");
	iscsi_initiator_setvar(ini, "auth type", "none");
	iscsi_initiator_setvar(ini, "mutual auth", "none");
	iscsi_initiator_setvar(ini, "target hostname", "localhost");
	(void) snprintf(buf, sizeof(buf), "%d", ISCSI_PORT);
	iscsi_initiator_setvar(ini, "target port", buf);
	return 1;
}

/* check there's enough space in the arrays */
static void
size_arrays(iscsi_initiator_t *ini, unsigned needed)
{
	if (ini->size == 0) {
		/* only get here first time around */
		ini->size = needed;
		ini->name = calloc(sizeof(char *), needed);
		ini->value = calloc(sizeof(char *), needed);
	} else if (ini->c == ini->size) {
		/* only uses 'needed' when filled array */
		ini->size += needed;
		ini->name = realloc(ini->name, sizeof(char *) * needed);
		ini->value = realloc(ini->value, sizeof(char *) * needed);
	}
}

/* find the name in the array */
static int
findvar(iscsi_initiator_t *ini, const char *name)
{
	unsigned	i;

	for (i = 0 ; i < ini->c && strcmp(ini->name[i], name) != 0; i++) {
	}
	return (i == ini->c) ? -1 : (int)i;
}

/* set a variable */
int
iscsi_initiator_setvar(iscsi_initiator_t *ini, const char *name,
			const char *value)
{
	int	i;

	if ((i = findvar(ini, name)) < 0) {
		/* add the element to the array */
		size_arrays(ini, ini->size + 15);
		ini->name[i = ini->c++] = strdup(name);
	} else {
		/* replace the element in the array */
		if (ini->value[i]) {
			(void) free(ini->value[i]);
			ini->value[i] = NULL;
		}
	}
	/* sanity checks for range of values would go here */
	ini->value[i] = strdup(value);
	return 1;
}

/* get a variable's value (NULL if not set) */
char *
iscsi_initiator_getvar(iscsi_initiator_t *ini, const char *name)
{
	int	i;

	return ((i = findvar(ini, name)) < 0) ? NULL : ini->value[i];
}
