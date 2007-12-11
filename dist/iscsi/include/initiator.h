/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software.
 *
 * Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _INITIATOR_H_
#define _INITIATOR_H_

#include "iscsi.h"
#include "parameters.h"
#include "defs.h"

#define CONFIG_INITIATOR_NUM_TARGETS 16

/***********
 * Private *
 ***********/

#define CONFIG_INITIATOR_QUEUE_DEPTH  CONFIG_INITIATOR_NUM_TARGETS
#define CONFIG_INITIATOR_MAX_SESSIONS CONFIG_INITIATOR_NUM_TARGETS

enum {
	INITIATOR_SESSION_STATE_INITIALIZING =		0x001,
	INITIATOR_SESSION_STATE_INITIALIZED =		0x002,
	INITIATOR_SESSION_STATE_CONNECTING =		0x004,
	INITIATOR_SESSION_STATE_CONNECTED =		0x008,
	INITIATOR_SESSION_STATE_LOGGING_IN =		0x010,
	INITIATOR_SESSION_STATE_AUTHENTICATED =		0x020,
	INITIATOR_SESSION_STATE_LOGGED_IN_NORMAL =	0x040,
	INITIATOR_SESSION_STATE_LOGGED_IN_DISCOVERY =	0x080,
	INITIATOR_SESSION_STATE_LOGGING_OUT =		0x100,
	INITIATOR_SESSION_STATE_LOGGED_OUT =		0x200,
	INITIATOR_SESSION_STATE_DESTROYING =		0x400
};

enum {
	TARGET_HOSTNAME_SIZE = 1024,
	TARGET_IP_SIZE = 16,
	TARGET_NAME_SIZE = 256
};

#define INITIATOR_STATE_SHUTDOWN 1

typedef struct {
	iscsi_mutex_t   mutex;
	iscsi_cond_t    cond;
}               initiator_wait_t;

typedef struct initiator_session_t {
	iscsi_socket_t  sock;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
	uint32_t        MaxCmdSN;
	iscsi_queue_t   tx_queue;
	iscsi_worker_t  tx_worker;
	iscsi_worker_t  rx_worker;
	uint64_t        isid;
	int             tsih;
	int             cid;
	uint32_t        state;
	iscsi_parameter_t *params;
	struct initiator_cmd_t *cmds;
	iscsi_spin_t    cmds_spin;
	iscsi_sess_param_t sess_params;
}               initiator_session_t;

typedef struct initiator_cmd_t {
	void           *ptr;
	int             type;
	int             (*callback) (void *);
	void           *callback_arg;
	uint64_t        isid;
	int             tx_done;
	int             status;
	struct initiator_cmd_t *next;
	struct initiator_cmd_t *hash_next;
	uint32_t        key;
	char            targetname[TARGET_HOSTNAME_SIZE];
}               initiator_cmd_t;

typedef struct initiator_target_t {
	char            name[TARGET_HOSTNAME_SIZE];
	char            ip[TARGET_IP_SIZE];
	int             port;
	char            TargetName[TARGET_NAME_SIZE];
	initiator_session_t *sess;
	int             has_session;
	char		iqnwanted[TARGET_NAME_SIZE];
}               initiator_target_t;

DEFINE_ARRAY(strv_t, char *);

enum {
	ISCSI_IPv4 = AF_INET,
	ISCSI_IPv6 = AF_INET6,
	ISCSI_UNSPEC = PF_UNSPEC
};

/**********
 * Public *
 **********/

int             initiator_init(const char *, int, int, const char *, int, int, int);
int             initiator_info(char *, int, int);
int             initiator_command(initiator_cmd_t *);
int             initiator_enqueue(initiator_cmd_t *);
int             initiator_abort(initiator_cmd_t *);
int             initiator_shutdown(void);
int             initiator_discover(char *, uint64_t, int);

void		get_target_info(uint64_t, initiator_target_t *);

int		ii_initiator_init(const char *, int, int, const char *, char *, int, int, int);

int		initiator_get_targets(int, strv_t *);
int		initiator_set_target_name(int, char *);

#endif				/* _INITIATOR_H_ */
