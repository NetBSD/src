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

#ifndef ISCSI_H
#define ISCSI_H

#include "util.h"

/*
 * iSCSI Version 18
 */

#define ISCSI_VENDOR	"NetBSD"
#define ISCSI_PRODUCT	"NetBSD iSCSI"
#define ISCSI_VERSION	0

/*
 * Parameters
 */

#define ISCSI_IMMEDIATE_DATA_DFLT            1
#define ISCSI_INITIAL_R2T_DFLT               1
#define ISCSI_USE_PHASE_COLLAPSED_READ_DFLT  0
#define ISCSI_HEADER_LEN                     48
#define ISCSI_PORT                           3260	/* Default port */
#define ISCSI_OPCODE(HEADER)                 (HEADER[0] & 0x3f)

#define ISCSI_FIRST_BURST                    (ISCSI_FIRST_BURST_DFLT)
#define ISCSI_DATA_PDU_LENGTH                (ISCSI_DATA_PDU_LENGTH_DFLT)

/*
 * Opcodes
 */

#define ISCSI_NOP_OUT       0x00
#define ISCSI_SCSI_CMD      0x01
#define ISCSI_TASK_CMD      0x02
#define ISCSI_LOGIN_CMD     0x03
#define ISCSI_TEXT_CMD      0x04
#define ISCSI_WRITE_DATA    0x05
#define ISCSI_LOGOUT_CMD    0x06
#define ISCSI_NOP_IN        0x20
#define ISCSI_SCSI_RSP      0x21
#define ISCSI_TASK_RSP      0x22
#define ISCSI_LOGIN_RSP     0x23
#define ISCSI_TEXT_RSP      0x24
#define ISCSI_READ_DATA     0x25
#define ISCSI_LOGOUT_RSP    0x26
#define ISCSI_R2T           0x31
#define ISCSI_REJECT        0x3f
#define ISCSI_SNACK         0x10/* not implemented */
#define ISCSI_ASYNC         0x32

/*
 * Login Phase
 */


#define ISCSI_LOGIN_STATUS_SUCCESS          0
#define ISCSI_LOGIN_STATUS_REDIRECTION      1
#define ISCSI_LOGIN_STATUS_INITIATOR_ERROR  2
#define ISCSI_LOGIN_STATUS_TARGET_ERROR     3
#define ISCSI_LOGIN_STAGE_SECURITY          0
#define ISCSI_LOGIN_STAGE_NEGOTIATE         1
#define ISCSI_LOGIN_STAGE_FULL_FEATURE      3


/*
 * Logout Phase
 */


#define ISCSI_LOGOUT_CLOSE_SESSION      0
#define ISCSI_LOGOUT_CLOSE_CONNECTION   1
#define ISCSI_LOGOUT_CLOSE_RECOVERY     2
#define ISCSI_LOGOUT_STATUS_SUCCESS     0
#define ISCSI_LOGOUT_STATUS_NO_CID      1
#define ISCSI_LOGOUT_STATUS_NO_RECOVERY 2
#define ISCSI_LOGOUT_STATUS_FAILURE     3


/*
 * Task Command
 */

#define ISCSI_TASK_CMD_ABORT_TASK           1
#define ISCSI_TASK_CMD_ABORT_TASK_SET       2
#define ISCSI_TASK_CMD_CLEAR_ACA            3
#define ISCSI_TASK_CMD_CLEAR_TASK_SET       4
#define ISCSI_TASK_CMD_LOGICAL_UNIT_RESET   5
#define ISCSI_TASK_CMD_TARGET_WARM_RESET    6
#define ISCSI_TASK_CMD_TARGET_COLD_RESET    7
#define ISCSI_TASK_CMD_TARGET_REASSIGN      8

typedef struct iscsi_task_cmd_t {
	int32_t         immediate;
	uint8_t         function;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        ref_tag;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
	uint32_t        RefCmdSN;
	uint32_t        ExpDataSN;
}               iscsi_task_cmd_t;

int             iscsi_task_cmd_encap(uint8_t *header, iscsi_task_cmd_t * cmd);
int             iscsi_task_cmd_decap(uint8_t *header, iscsi_task_cmd_t * cmd);


/*
 * Task Response
 */

#define ISCSI_TASK_RSP_FUNCTION_COMPLETE  0
#define ISCSI_TASK_RSP_NO_SUCH_TASK       1
#define ISCSI_TASK_RSP_NO_SUCH_LUN        2
#define ISCSI_TASK_RSP_STILL_ALLEGIANT    3
#define ISCSI_TASK_RSP_NO_FAILOVER        4
#define ISCSI_TASK_RSP_NO_SUPPORT	  5
#define ISCSI_TASK_RSP_AUTHORIZED_FAILED  6

#define ISCSI_TASK_RSP_REJECTED           255

#define ISCSI_TASK_QUAL_FUNCTION_EXECUTED  0
#define ISCSI_TASK_QUAL_NOT_AUTHORIZED     1

typedef struct iscsi_task_rsp_t {
	uint8_t         response;
	uint32_t        length;
	uint32_t        tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
}               iscsi_task_rsp_t;

int             iscsi_task_rsp_encap(uint8_t *header, iscsi_task_rsp_t * rsp);
int             iscsi_task_rsp_decap(uint8_t *header, iscsi_task_rsp_t * rsp);


/*
 * NOP-Out
 */


typedef struct iscsi_nop_out_args_t {
	int32_t         immediate;
	uint32_t        length;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        transfer_tag;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
	const uint8_t   *data;
}               iscsi_nop_out_args_t;

int             iscsi_nop_out_encap(uint8_t *header, iscsi_nop_out_args_t * cmd);
int             iscsi_nop_out_decap(uint8_t *header, iscsi_nop_out_args_t * cmd);


/*
 * NOP-In
 */


typedef struct iscsi_nop_in_args_t {
	uint32_t        length;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        transfer_tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
}               iscsi_nop_in_args_t;

int             iscsi_nop_in_encap(uint8_t *header, iscsi_nop_in_args_t * cmd);
int             iscsi_nop_in_decap(uint8_t *header, iscsi_nop_in_args_t * cmd);


/*
 * Text Command
 */


typedef struct iscsi_text_cmd_args_t {
	int32_t         immediate;
	int32_t         final;
	int32_t         cont;
	uint32_t        length;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        transfer_tag;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
	char           *text;
}               iscsi_text_cmd_args_t;

int             iscsi_text_cmd_encap(uint8_t *header, iscsi_text_cmd_args_t * cmd);
int             iscsi_text_cmd_decap(uint8_t *header, iscsi_text_cmd_args_t * cmd);


/*
 * Text Response
 */


typedef struct iscsi_text_rsp_args_t {
	int32_t         final;
	int32_t         cont;
	uint32_t        length;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        transfer_tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
}               iscsi_text_rsp_args_t;

int             iscsi_text_rsp_encap(uint8_t *header, iscsi_text_rsp_args_t * rsp);
int             iscsi_text_rsp_decap(uint8_t *header, iscsi_text_rsp_args_t * rsp);


/*
 * Login Command
 */


typedef struct iscsi_login_cmd_args_t {
	int32_t         transit;
	int32_t         cont;
	uint8_t         csg;
	uint8_t         nsg;
	int8_t          version_max;
	int8_t          version_min;
	uint8_t         AHSlength;
	uint32_t        length;
	uint64_t	isid;
	uint16_t        tsih;
	uint32_t        tag;
	uint16_t        cid;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
	char           *text;
}               iscsi_login_cmd_args_t;

int             iscsi_login_cmd_encap(uint8_t *header, iscsi_login_cmd_args_t * cmd);
int             iscsi_login_cmd_decap(uint8_t *header, iscsi_login_cmd_args_t * cmd);


/*
 * Login Response
 */


typedef struct iscsi_login_rsp_args_t {
	int32_t         transit;
	int32_t         cont;
	uint8_t         csg;
	uint8_t         nsg;
	int8_t          version_max;
	int8_t          version_active;
	uint8_t         AHSlength;
	uint32_t        length;
	uint64_t	isid;
	uint16_t        tsih;
	uint32_t        tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint8_t         status_class;
	uint8_t         status_detail;
}               iscsi_login_rsp_args_t;

int             iscsi_login_rsp_encap(uint8_t *header, iscsi_login_rsp_args_t * rsp);
int             iscsi_login_rsp_decap(uint8_t *header, iscsi_login_rsp_args_t * rsp);


/*
 * Logout Command
 */


typedef struct iscsi_logout_cmd_args_t {
	int32_t         immediate;
	uint8_t         reason;
	uint32_t        tag;
	uint16_t        cid;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
}               iscsi_logout_cmd_args_t;

int             iscsi_logout_cmd_encap(uint8_t *header, iscsi_logout_cmd_args_t * cmd);
int             iscsi_logout_cmd_decap(uint8_t *header, iscsi_logout_cmd_args_t * cmd);


/*
 * Logout Response
 */


typedef struct iscsi_logout_rsp_args_t {
	uint8_t         response;
	uint32_t        length;
	uint32_t        tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint16_t        Time2Wait;
	uint16_t        Time2Retain;
}               iscsi_logout_rsp_args_t;

int             iscsi_logout_rsp_encap(uint8_t *header, iscsi_logout_rsp_args_t * rsp);
int             iscsi_logout_rsp_decap(uint8_t *header, iscsi_logout_rsp_args_t * rsp);


/*
 * SCSI Command
 */


typedef struct iscsi_scsi_cmd_args_t {
	int32_t         immediate;
	int32_t         final;
	int32_t         input;
	int32_t         output;
	uint8_t         attr;
	uint32_t        length;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        trans_len;
	uint32_t        bidi_trans_len;
	uint32_t        CmdSN;
	uint32_t        ExpStatSN;
	uint8_t  	*cdb;
	uint8_t  	*ext_cdb;
	uint8_t  	*ahs;
	uint8_t         ahs_len;
	uint8_t  	*send_data;
	int32_t         send_sg_len;
	uint8_t  	*recv_data;
	int32_t         recv_sg_len;
	uint8_t         status;
	uint32_t        bytes_sent;
	uint32_t        bytes_recv;
}               iscsi_scsi_cmd_args_t;

int             iscsi_scsi_cmd_encap(uint8_t *header, iscsi_scsi_cmd_args_t * cmd);
int             iscsi_scsi_cmd_decap(uint8_t *header, iscsi_scsi_cmd_args_t * cmd);


/*
 * SCSI Response
 */


typedef struct iscsi_scsi_rsp_args_t {
	int32_t         bidi_overflow;
	int32_t         bidi_underflow;
	int32_t         overflow;
	int32_t         underflow;


	uint8_t         response;
	uint8_t         status;
	uint32_t        ahs_len;
	uint32_t        length;
	uint32_t        tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint32_t        ExpDataSN;
	uint32_t        bidi_res_cnt;
	uint32_t        basic_res_cnt;
}               iscsi_scsi_rsp_t;

int             iscsi_scsi_rsp_encap(uint8_t *header, iscsi_scsi_rsp_t * rsp);
int             iscsi_scsi_rsp_decap(uint8_t *header, iscsi_scsi_rsp_t * rsp);


/*
 * Ready To Transfer (R2T)
 */


typedef struct iscsi_r2t_args_t {
	uint32_t        AHSlength;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        transfer_tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint32_t        R2TSN;
	uint32_t        offset;
	uint32_t        length;
}               iscsi_r2t_t;

int             iscsi_r2t_encap(uint8_t *header, iscsi_r2t_t * cmd);
int             iscsi_r2t_decap(uint8_t *header, iscsi_r2t_t * cmd);


/*
 * SCSI Write Data
 */


typedef struct iscsi_write_data_args_t {
	int32_t         final;
	uint32_t        length;
	uint64_t        lun;
	uint32_t        tag;
	uint32_t        transfer_tag;
	uint32_t        ExpStatSN;
	uint32_t        DataSN;
	uint32_t        offset;
}               iscsi_write_data_t;

int             iscsi_write_data_encap(uint8_t *header, iscsi_write_data_t * cmd);
int             iscsi_write_data_decap(uint8_t *header, iscsi_write_data_t * cmd);


/*
 * SCSI Read Data
 */


typedef struct iscsi_read_data_args_t {
	int32_t         final;
	int32_t         ack;
	int32_t         overflow;
	int32_t         underflow;
	int32_t         S_bit;
	uint8_t         status;
	uint32_t        length;
	uint64_t        lun;
	uint32_t        task_tag;
	uint32_t        transfer_tag;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint32_t        DataSN;
	uint32_t        offset;
	uint32_t        res_count;
}               iscsi_read_data_t;

int             iscsi_read_data_encap(uint8_t *header, iscsi_read_data_t * cmd);
int             iscsi_read_data_decap(uint8_t *header, iscsi_read_data_t * cmd);


/*
 * Reject
 */

typedef struct iscsi_reject_args_t {
	uint8_t         reason;
	uint32_t        length;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint32_t        DataSN;
	char           *header;

}               iscsi_reject_t;

int             iscsi_reject_encap(uint8_t *header, iscsi_reject_t * cmd);
int             iscsi_reject_decap(uint8_t *header, iscsi_reject_t * cmd);

/*
 * Async Message
 */

typedef struct iscsi_async_msg_args_t {
	uint8_t         AHSlength;
	uint64_t        lun;
	uint32_t        StatSN;
	uint32_t        ExpCmdSN;
	uint32_t        MaxCmdSN;
	uint32_t        length;
	uint8_t         AsyncEvent;
	uint8_t         AsyncVCode;
}               iscsi_async_msg_t;

int             iscsi_amsg_decap(uint8_t *header, iscsi_async_msg_t * msg);

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#endif				/* ISCSI_H */
