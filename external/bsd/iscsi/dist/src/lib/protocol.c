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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif      
             
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
   
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
           
#include "iscsiprotocol.h"
#include "iscsiutil.h"
#include <compat.h>


/*
 * Task Command
 */

int 
iscsi_task_cmd_encap(uint8_t *header, iscsi_task_cmd_t * cmd)
{
	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate: %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Function:  %u\n", cmd->function);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:       %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:       %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Ref Tag:   %#x\n", cmd->ref_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:     %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN: %u\n", cmd->ExpStatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "RefCmdSN:  %u\n", cmd->RefCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpDataSN: %u\n", cmd->ExpDataSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= ISCSI_TASK_CMD;	/* Opcode */
	if (cmd->immediate) {
		header[0] |= 0x40;	/* Immediate bit  */
	}
	header[1] = cmd->function & 0x80;	/* Function  */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->ref_tag);	/* Reference Tag */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->CmdSN);	/* CmdSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(cmd->RefCmdSN);	/* RefCmdSN */
	*((uint32_t *) (void *) (header + 36)) = ISCSI_HTONL(cmd->ExpDataSN);	/* ExpDataSN */

	return 0;
}

int 
iscsi_task_cmd_decap(uint8_t *header, iscsi_task_cmd_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_TASK_CMD) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->immediate = ((header[0] & 0x40) == 0x40);
	cmd->function = header[1] & 0x80;
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));
	cmd->ref_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));
	cmd->CmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));
	cmd->RefCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));
	cmd->ExpDataSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 36)));

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[1] & 0x80) != 0x80) {
		errmsg = "Byte 1 bit 0";
	} else if (header[2] != 0) {
		errmsg = "Byte 2";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (memcmp(header + 4, zeros, 4) != 0) {
		errmsg = "Bytes 4-7";
	} else if (memcmp(header + 40, zeros, 8) != 0) {
		errmsg = "Bytes 40-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}

	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate: %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Function:  %u\n", cmd->function);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:       %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:       %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Ref Tag:   %#x\n", cmd->ref_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:     %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN: %u\n", cmd->ExpStatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "RefCmdSN:  %u\n", cmd->RefCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpDataSN: %u\n", cmd->ExpDataSN);
	return 0;
}

/*
 * Task Response
 */

int 
iscsi_task_rsp_encap(uint8_t *header, iscsi_task_rsp_t * rsp)
{
	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "Response:  %u\n", rsp->response);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:    %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:       %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:    %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:  %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:  %u\n", rsp->MaxCmdSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= ISCSI_TASK_RSP;	/* Opcode */
	header[1] |= 0x80;	/* Byte 1 bit 0  */
	header[2] = rsp->response;	/* Response */
	length = (rsp->length & 0x00ffffff);	/* Length */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(rsp->tag);
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(rsp->StatSN);
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(rsp->ExpCmdSN);
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(rsp->MaxCmdSN);
	return 0;
}

int 
iscsi_task_rsp_decap(uint8_t *header, iscsi_task_rsp_t * rsp)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_TASK_RSP) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	rsp->response = header[2];
	rsp->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));
	rsp->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));
	rsp->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));
	rsp->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));
	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[0] & 0x00) != 0x00) {
		errmsg = "Byte 0 bits 0-1";
	} else if ((header[1] & 0x80) != 0x80) {
		errmsg = "Byte 1 bit 0";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (memcmp(header + 4, zeros, 12) != 0) {
		errmsg = "Bytes 4-15";
	} else if (memcmp(header + 20, zeros, 4) != 0) {
		errmsg = "Bytes 20-23";
	} else if (memcmp(header + 36, zeros, 12) != 0) {
		errmsg = "Bytes 36-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Response:  %u\n", rsp->response);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:       %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:    %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:  %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:  %u\n", rsp->MaxCmdSN);
	return 0;
}

/*
 * NOP-Out
 */

int 
iscsi_nop_out_encap(uint8_t *header, iscsi_nop_out_args_t * cmd)
{

	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate:    %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:        %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:    %u\n", cmd->ExpStatSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] = ISCSI_NOP_OUT;	/* Opcode */
	if (cmd->immediate) {
		header[0] |= 0x40;	/* Immediate bit */
	}
	header[1] |= 0x80;	/* Byte 1 bit 0 and Reserved */
	length = (cmd->length & 0x00ffffff);	/* Length  */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);	/* Length  */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->transfer_tag);	/* Target Transfer Tag  */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->CmdSN);	/* CmdSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN */

	return 0;
}

int 
iscsi_nop_out_decap(uint8_t *header, iscsi_nop_out_args_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_NOP_OUT) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->immediate = ((header[0] & 0x40) == 0x40);	/* Immediate bit  */
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	cmd->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Target Tranfer Tag */
	cmd->CmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* CmdSN */
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpStatSN */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if (header[1] != 0x80) {
		errmsg = "Byte 1";
	} else if (memcmp(header + 2, zeros, 3) != 0) {
		errmsg = "Bytes 2-4";
	} else if (memcmp(header + 32, zeros, 16) != 0) {
		errmsg = "Bytes 32-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate:    %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:        %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:    %u\n", cmd->ExpStatSN);
	return 0;
}

/*
 * NOP-In
 */

int 
iscsi_nop_in_encap(uint8_t *header, iscsi_nop_in_args_t * cmd)
{
	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:       %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:     %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:     %u\n", cmd->MaxCmdSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] = 0x00 | ISCSI_NOP_IN;	/* Opcode  */
	header[1] |= 0x80;	/* Reserved */
	length = (cmd->length & 0x00ffffff);	/* Length */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);	/* Length */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->transfer_tag);	/* Target Transfer Tag        */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->StatSN);	/* StatSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(cmd->MaxCmdSN);	/* MaxCmdSN */

	return 0;
}

int 
iscsi_nop_in_decap(uint8_t *header, iscsi_nop_in_args_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_NOP_IN) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	cmd->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Target Transfer Tag */
	cmd->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN  */
	cmd->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN */
	cmd->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[0] & 0xc0) != 0x00) {
		errmsg = "Byte 0, bits 0-1";
	} else if (header[1] != 0x80) {
		errmsg = "Byte 1";
	} else if (memcmp(header + 2, zeros, 3) != 0) {
		errmsg = "Bytes 2-4";
	} else if (memcmp(header + 36, zeros, 12) != 0) {
		errmsg = "Bytes 36-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:       %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:     %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:     %u\n", cmd->MaxCmdSN);
	return 0;
}

/*
 * Text Command
 */

int 
iscsi_text_cmd_encap(uint8_t *header, iscsi_text_cmd_args_t * cmd)
{
	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate:    %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:        %d\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:     %d\n", cmd->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:        %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:    %u\n", cmd->ExpStatSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= ISCSI_TEXT_CMD;	/* Opcode */
	if (cmd->immediate) {
		header[0] |= 0x40;	/* Immediate bit */
	}
	if (cmd->final) {
		header[1] |= 0x80;	/* Final bit */
	}
	if (cmd->cont) {
		header[1] |= 0x40;	/* Continue bit */
	}
	length = (cmd->length & 0x00ffffff);	/* Length */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);	/* Length */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->transfer_tag);	/* Transfer Tag */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->CmdSN);	/* CmdSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN */

	return 0;
}

int 
iscsi_text_cmd_decap(uint8_t *header, iscsi_text_cmd_args_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_TEXT_CMD) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->immediate = ((header[0] & 0x40) == 0x40);	/* Immediate bit  */
	cmd->final = ((header[1] & 0x80) == 0x80);	/* Final bit */
	cmd->cont = ((header[1] & 0x40) == 0x40);	/* Continue bit */
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	cmd->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Transfer Tag */
	cmd->CmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* CmdSN */
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpStatSN */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[1] & 0x00) != 0x00) {
		errmsg = "Byte 1, bits 2-7";
	} else if (memcmp(header + 2, zeros, 3) != 0) {
		errmsg = "Bytes 2-4";
	} else if (memcmp(header + 8, zeros, 8) != 0) {
		errmsg = "Bytes 8-15";
	} else if (memcmp(header + 32, zeros, 16) != 0) {
		errmsg = "Bytes 32-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate:    %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:        %d\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:     %d\n", cmd->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:        %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:    %u\n", cmd->ExpStatSN);

	return 0;
}

/*
 * Text Response
 */

int 
iscsi_text_rsp_encap(uint8_t *header, iscsi_text_rsp_args_t * rsp)
{
	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "Final:        %d\n", rsp->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:     %d\n", rsp->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", rsp->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", rsp->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:       %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:     %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:     %u\n", rsp->MaxCmdSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);
	header[0] |= 0x00 | ISCSI_TEXT_RSP;	/* Opcode */
	if (rsp->final) {
		header[1] |= 0x80;	/* Final bit */
	}
	if (rsp->cont) {
		header[1] |= 0x40;	/* Continue */
	}
	length = (rsp->length & 0x00ffffff);	/* Length */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);	/* Length */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(rsp->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(rsp->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(rsp->transfer_tag);	/* Transfer Tag */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(rsp->StatSN);	/* StatSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(rsp->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(rsp->MaxCmdSN);	/* MaxCmdSN */

	return 0;
}

int 
iscsi_text_rsp_decap(uint8_t *header, iscsi_text_rsp_args_t * rsp)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_TEXT_RSP) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	rsp->final = ((header[1] & 0x80) == 0x80);	/* Final bit  */
	rsp->cont = ((header[1] & 0x40) == 0x40);	/* Continue bit */
	rsp->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	rsp->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	rsp->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	rsp->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Transfer Tag */
	rsp->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN */
	rsp->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN */
	rsp->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[1] & 0x3f) != 0x00) {
		errmsg = "Byte 1, bits 2-7";
	} else if (memcmp(header + 2, zeros, 3) != 0) {
		errmsg = "Bytes 2-4";
	} else if (memcmp(header + 8, zeros, 8) != 0) {
		errmsg = "Bytes 8-15";
	} else if (memcmp(header + 36, zeros, 12) != 0) {
		errmsg = "Bytes 36-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:        %d\n", rsp->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:     %d\n", rsp->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", rsp->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", rsp->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:       %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:     %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:     %u\n", rsp->MaxCmdSN);

	return 0;
}

/*
 * Login Command
 */

int 
iscsi_login_cmd_encap(uint8_t *header, iscsi_login_cmd_args_t * cmd)
{
	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "Transit:           %d\n", cmd->transit);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:          %d\n", cmd->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "CSG:               %u\n", cmd->csg);
	iscsi_trace(TRACE_ISCSI_ARGS, "NSG:               %u\n", cmd->nsg);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_min:       %u\n", cmd->version_min);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_max:       %u\n", cmd->version_max);
	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", cmd->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "ISID:              %" PRIu64 "\n", cmd->isid);
	iscsi_trace(TRACE_ISCSI_ARGS, "TSIH:              %hu\n", cmd->tsih);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CID:               %hu\n", cmd->cid);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:             %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:         %u\n", cmd->ExpStatSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= 0x40 | ISCSI_LOGIN_CMD;	/* Opcode  */
	if (cmd->transit) {
		header[1] |= 0x80;	/* Transit */
	}
	if (cmd->cont) {
		header[1] |= 0x40;	/* Continue */
	}
	header[1] |= ((cmd->csg) << 2) & 0x0c;	/* CSG */
	header[1] |= (cmd->nsg) & 0x03;	/* NSG */
	header[2] = cmd->version_max;	/* Version-Max  */
	header[3] = cmd->version_min;	/* Version-Min  */
	header[4] = cmd->AHSlength;	/* TotalAHSLength */
	length = (cmd->length & 0x00ffffff);	/* Length  */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);	/* Length  */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->isid << 16);	/* ISID */
	*((uint16_t *) (void *) (header + 14)) = ISCSI_HTONS(cmd->tsih);	/* TSIH */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Task Tag */
	*((uint16_t *) (void *) (header + 20)) = ISCSI_HTONS(cmd->cid);	/* CID */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->CmdSN);	/* CmdSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN */

	return 0;
}

int 
iscsi_login_cmd_decap(uint8_t *header, iscsi_login_cmd_args_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_LOGIN_CMD) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->transit = (header[1] & 0x80) ? 1 : 0;	/* Transit */
	cmd->cont = (header[1] & 0x40) ? 1 : 0;	/* Continue */
	cmd->csg = (header[1] & 0x0cU) >> 2;	/* CSG */
	cmd->nsg = header[1] & 0x03;	/* NSG */
	cmd->version_max = header[2];	/* Version-Max  */
	cmd->version_min = header[3];	/* Version-Min  */
	cmd->AHSlength = header[4];	/* TotalAHSLength */
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->isid = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8))) >> 16;	/* ISID */
	cmd->tsih = ISCSI_NTOHS(*((uint16_t *) (void *) (header + 14)));	/* TSIH */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Task Tag */
	cmd->cid = ISCSI_NTOHS(*((uint16_t *) (void *) (header + 20)));	/* CID */
	cmd->CmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* CmdSN  */
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpStatSN */

	iscsi_trace(TRACE_ISCSI_ARGS, "Transit:           %d\n", cmd->transit);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:          %d\n", cmd->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "CSG:               %u\n", cmd->csg);
	iscsi_trace(TRACE_ISCSI_ARGS, "NSG:               %u\n", cmd->nsg);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_min:       %u\n", cmd->version_min);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_max:       %u\n", cmd->version_max);
	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", cmd->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "ISID:              %" PRIu64 "\n", cmd->isid);
	iscsi_trace(TRACE_ISCSI_ARGS, "TSIH:              %hu\n", cmd->tsih);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CID:               %hu\n", cmd->cid);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:             %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:         %u\n", cmd->ExpStatSN);

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if (((header[1] & 0x30U) >> 4U) != 0x0) {
		errmsg = "Byte 1, bits 2-3";
	} else if (memcmp(header + 22, zeros, 2) != 0) {
		errmsg = "Bytes 22-23";
	} else if (memcmp(header + 32, zeros, 16) != 0) {
		errmsg = "Bytes 32-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	if (cmd->transit) {
		if (cmd->nsg <= cmd->csg) {
			return -1;
		}
		if ((cmd->nsg != 0) && (cmd->nsg != 1) && (cmd->nsg != 3)) {
			return -1;
		}
	}
	return 0;
}

/*
 * Login Response
 */

int 
iscsi_login_rsp_encap(uint8_t *header, iscsi_login_rsp_args_t * rsp)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Transit:           %d\n", rsp->transit);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:          %d\n", rsp->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "CSG:               %u\n", rsp->csg);
	iscsi_trace(TRACE_ISCSI_ARGS, "NSG:               %u\n", rsp->nsg);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_max:       %u\n", rsp->version_max);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_active:    %u\n", rsp->version_active);
	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", rsp->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "ISID:              %" PRIu64 "\n", rsp->isid);
	iscsi_trace(TRACE_ISCSI_ARGS, "TSIH:              %u\n", rsp->tsih);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:            %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:          %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:          %u\n", rsp->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Status-Class:      %u\n", rsp->status_class);
	iscsi_trace(TRACE_ISCSI_ARGS, "Status-Detail:     %u\n", rsp->status_detail);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= 0x00 | ISCSI_LOGIN_RSP;	/* Opcode  */
	if (rsp->transit) {
		header[1] |= 0x80;	/* Transit  */
	}
	if (rsp->cont) {
		header[1] |= 0x40;	/* Continue */
	}
	header[1] |= ((rsp->csg) << 2) & 0x0c;	/* CSG */
	if (rsp->transit) {
		header[1] |= (rsp->nsg) & 0x03;	/* NSG */
	}
	header[2] = rsp->version_max;	/* Version-max */
	header[3] = rsp->version_active;	/* Version-active */
	header[4] = rsp->AHSlength;	/* TotalAHSLength */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(rsp->length);	/* Length */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(rsp->isid << 16);	/* ISID */
	*((uint16_t *) (void *) (header + 14)) = ISCSI_HTONS(rsp->tsih);	/* TSIH */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(rsp->tag);	/* Tag  */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(rsp->StatSN);	/* StatRn */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(rsp->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(rsp->MaxCmdSN);	/* MaxCmdSN */
	header[36] = rsp->status_class;	/* Status-Class */
	header[37] = rsp->status_detail;	/* Status-Detail */

	return 0;
}

int 
iscsi_login_rsp_decap(uint8_t *header, iscsi_login_rsp_args_t * rsp)
{
	const char	*errmsg;
	uint8_t		 zeros[8];

	if (ISCSI_OPCODE(header) != ISCSI_LOGIN_RSP) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	rsp->transit = (header[1] & 0x80U) >> 7;	/* Transit  */
	rsp->cont = (header[1] & 0x40U) >> 6;	/* Continue */
	rsp->csg = (header[1] & 0x0cU) >> 2;	/* CSG  */
	rsp->nsg = header[1] & 0x03;	/* NSG  */
	rsp->version_max = header[2];	/* Version-max */
	rsp->version_active = header[3];	/* Version-active */
	rsp->AHSlength = header[4];	/* TotalAHSLength */
	rsp->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	rsp->isid = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8))) >> 16;	/* ISID */
	rsp->tsih = ISCSI_NTOHS(*((uint16_t *) (void *) (header + 14)));	/* TSIH */

	rsp->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	rsp->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN */
	rsp->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN */
	rsp->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN  */
	rsp->status_class = header[36];	/* Status-Class */
	rsp->status_detail = header[37];	/* Status-Detail */

	iscsi_trace(TRACE_ISCSI_ARGS, "Transit:           %d\n", rsp->transit);
	iscsi_trace(TRACE_ISCSI_ARGS, "Continue:          %d\n", rsp->cont);
	iscsi_trace(TRACE_ISCSI_ARGS, "CSG:               %u\n", rsp->csg);
	iscsi_trace(TRACE_ISCSI_ARGS, "NSG:               %u\n", rsp->nsg);

	iscsi_trace(TRACE_ISCSI_ARGS, "Version_max:       %u\n", rsp->version_max);
	iscsi_trace(TRACE_ISCSI_ARGS, "Version_active:    %u\n", rsp->version_active);
	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", rsp->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "ISID:              %" PRIu64 "\n", rsp->isid);
	iscsi_trace(TRACE_ISCSI_ARGS, "TSIH:              %u\n", rsp->tsih);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:            %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:          %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:          %u\n", rsp->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Status-Class:      %u\n", rsp->status_class);
	iscsi_trace(TRACE_ISCSI_ARGS, "Status-Detail:     %u\n", rsp->status_detail);
	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if (((header[1] & 0x30U) >> 4U) != 0x0) {
		errmsg = "Byte 1, bits 2-3";
	} else if (memcmp(header + 20, zeros, 4) != 0) {
		errmsg = "Bytes 20-23";
	} else if (memcmp(header + 38, zeros, 2) != 0) {
		errmsg = "Bytes 38-39";
	} else if (memcmp(header + 40, zeros, 8) != 0) {
		errmsg = "Bytes 40-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	return 0;
}

/*
 * Logout Command
 */

int 
iscsi_logout_cmd_encap(uint8_t *header, iscsi_logout_cmd_args_t * cmd)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate: %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Reason:    %u\n", cmd->reason);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:  %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "CID:       %hu\n", cmd->cid);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:     %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN: %u\n", cmd->ExpStatSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] = ISCSI_LOGOUT_CMD;	/* Opcode */
	if (cmd->immediate) {
		header[0] |= 0x40;	/* Immediate */
	}
	header[1] = cmd->reason | 0x80;	/* Reason  */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint16_t *) (void *) (header + 20)) = ISCSI_HTONS(cmd->cid);	/* CID */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->CmdSN);	/* CmdSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN  */

	return 0;
}

int 
iscsi_logout_cmd_decap(uint8_t *header, iscsi_logout_cmd_args_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_LOGOUT_CMD) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->immediate = (header[0] & 0x40) ? 1 : 0;	/* Immediate */
	cmd->reason = header[1] & 0x7f;	/* Reason */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	cmd->cid = ISCSI_NTOHS(*((uint16_t *) (void *) (header + 20)));	/* CID */
	cmd->CmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* CmdSN */
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpStatSN */

	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate: %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Reason:    %u\n", cmd->reason);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:  %#x\n", cmd->tag);

	iscsi_trace(TRACE_ISCSI_ARGS, "CID:       %hu\n", cmd->cid);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:     %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN: %u\n", cmd->ExpStatSN);
	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((unsigned)(header[0]) >> 0x7U != 0) {
		errmsg = "Byte 0, bit 0";
	} else if ((unsigned)(header[1]) >> 7U != 1) {
		errmsg = "Byte 1, bit 0";
	} else if (header[2] != 0) {
		errmsg = "Byte 2";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (memcmp(header + 4, zeros, 12) != 0) {
		errmsg = "Bytes 4-7";
	} else if (memcmp(header + 22, zeros, 2) != 0) {
		errmsg = "Bytes 22-23";
	} else if (memcmp(header + 32, zeros, 16) != 0) {
		errmsg = "Bytes 32-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	return 0;
}

/*
 * Logout Response
 */

int 
iscsi_logout_rsp_encap(uint8_t *header, iscsi_logout_rsp_args_t * rsp)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Response:    %u\n", rsp->response);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:      %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:    %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:      %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:    %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:    %u\n", rsp->MaxCmdSN);

	iscsi_trace(TRACE_ISCSI_ARGS, "Time2Wait:   %hu\n", rsp->Time2Wait);
	iscsi_trace(TRACE_ISCSI_ARGS, "Time2Retain: %hu\n", rsp->Time2Retain);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= 0x00 | ISCSI_LOGOUT_RSP;	/* Opcode  */
	header[1] |= 0x80;	/* Reserved  */
	header[2] = rsp->response;	/* Response */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(rsp->length);	/* Length */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(rsp->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(rsp->StatSN);	/* StatSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(rsp->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(rsp->MaxCmdSN);	/* MaxCmdSN */
	*((uint16_t *) (void *) (header + 40)) = ISCSI_HTONS(rsp->Time2Wait);	/* Time2Wait */
	*((uint16_t *) (void *) (header + 42)) = ISCSI_HTONS(rsp->Time2Retain);	/* Time2Retain */

	return 0;
}

int 
iscsi_logout_rsp_decap(uint8_t *header, iscsi_logout_rsp_args_t * rsp)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_LOGOUT_RSP) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	rsp->response = header[2];	/* Response */
	rsp->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	rsp->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	rsp->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN */
	rsp->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN */
	rsp->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN  */
	rsp->Time2Wait = ISCSI_NTOHS(*((uint32_t *) (void *) (header + 40)));	/* Time2Wait */
	rsp->Time2Retain = ISCSI_NTOHS(*((uint32_t *) (void *) (header + 42)));	/* Time2Retain */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[0] & 0x20) != 0x20) {
		errmsg = "Byte 0, bits 0-1";
	} else if ((header[1] & 0x80) != 0x80) {
		errmsg = "Byte 1, bit 0";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (memcmp(header + 4, zeros, 12) != 0) {
		errmsg = "Bytes 4-15";
	} else if (memcmp(header + 20, zeros, 4) != 0) {
		errmsg = "Bytes 20-23";
	} else if (memcmp(header + 36, zeros, 4) != 0) {
		errmsg = "Bytes 36-39";
	} else if (memcmp(header + 44, zeros, 4) != 0) {
		errmsg = "Bytes 44-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Response:    %u\n", rsp->response);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:      %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:    %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:      %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:    %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:    %u\n", rsp->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Time2Wait:   %hu\n", rsp->Time2Wait);
	iscsi_trace(TRACE_ISCSI_ARGS, "Time2Retain: %hu\n", rsp->Time2Retain);

	return 0;
}

/*
 * SCSI Command
 */

int 
iscsi_scsi_cmd_encap(uint8_t *header, iscsi_scsi_cmd_args_t * cmd)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate:         %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:             %d\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Input:             %d\n", cmd->input);
	iscsi_trace(TRACE_ISCSI_ARGS, "Output:            %d\n", cmd->output);
	iscsi_trace(TRACE_ISCSI_ARGS, "ATTR:              %d\n", cmd->attr);
	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", cmd->ahs_len);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:               %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Length:   %u\n", cmd->trans_len);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:             %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:         %u\n", cmd->ExpStatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "CDB:               %#x\n", cmd->cdb[0]);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= ISCSI_SCSI_CMD;	/* Opcode */
	if (cmd->immediate) {
		header[0] |= 0x40;	/* Immediate */
	}
	if (cmd->final) {
		header[1] |= 0x80;	/* Final */
	}
	if (cmd->input) {
		header[1] |= 0x40;	/* Input bit */
	}
	if (cmd->output) {
		header[1] |= 0x20;	/* Output bit */
	}
	header[1] |= cmd->attr & 0x07;	/* ATTR  */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(cmd->length);	/* DataSegmentLength */
	header[4] = cmd->ahs_len;	/* TotalAHSLength  */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Task Tag  */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->trans_len);	/* Expected Transfer
								 * Length */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->CmdSN);	/* CmdSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN */
	memcpy(header + 32, cmd->cdb, 16);	/* CDB */

	return 0;
}

int 
iscsi_scsi_cmd_decap(uint8_t *header, iscsi_scsi_cmd_args_t * cmd)
{
	const char	*errmsg;

	if (ISCSI_OPCODE(header) != ISCSI_SCSI_CMD) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->immediate = (header[0] & 0x40) ? 1 : 0;	/* Immediate */
	cmd->final = (header[1] & 0x80) ? 1 : 0;	/* Final */
	cmd->input = (header[1] & 0x40) ? 1 : 0;	/* Input */
	cmd->output = (header[1] & 0x20) ? 1 : 0;	/* Output */
	cmd->attr = header[1] & 0x07;	/* ATTR  */
	cmd->ahs_len = header[4];
	header[4] = 0x00;
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* DataSegmentLength */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Task Tag */
	cmd->trans_len = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Expected Transfer
								 * Length */
	cmd->CmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* CmdSN  */
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpStatSN */
	cmd->cdb = header + 32;	/* CDB */

	errmsg = NULL;
	if ((header[1] & 0x18) != 0x0) {
		errmsg = "Byte 1, bits 3-4";
	} else if (header[2] != 0) {
		errmsg = "Byte 2";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Immediate:         %d\n", cmd->immediate);
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:             %d\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Input:             %d\n", cmd->input);
	iscsi_trace(TRACE_ISCSI_ARGS, "Output:            %d\n", cmd->output);
	iscsi_trace(TRACE_ISCSI_ARGS, "ATTR:              %d\n", cmd->attr);
	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", cmd->ahs_len);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:               %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Length:   %u\n", cmd->trans_len);
	iscsi_trace(TRACE_ISCSI_ARGS, "CmdSN:             %u\n", cmd->CmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:         %u\n", cmd->ExpStatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "CDB:               %#x\n", cmd->cdb[0]);

	return 0;
}

/*
 * SCSI Response
 */

int 
iscsi_scsi_rsp_encap(uint8_t *header, iscsi_scsi_rsp_t * rsp)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Bidi Overflow:       %d\n", rsp->bidi_overflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Bidi Underflow:      %d\n", rsp->bidi_underflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Overflow:            %d\n", rsp->overflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Underflow:           %d\n", rsp->underflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "iSCSI Response:      %u\n", rsp->response);
	iscsi_trace(TRACE_ISCSI_ARGS, "SCSI Status:         %u\n", rsp->status);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength:   %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:            %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:              %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:            %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:            %u\n", rsp->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpDataSN:           %u\n", rsp->ExpDataSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Bidi Residual Count: %u\n", rsp->bidi_res_cnt);
	iscsi_trace(TRACE_ISCSI_ARGS, "Residual Count:      %u\n", rsp->basic_res_cnt);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= 0x00 | ISCSI_SCSI_RSP;	/* Opcode  */
	header[1] |= 0x80;	/* Byte 1 bit 7 */
	if (rsp->bidi_overflow) {
		header[1] |= 0x10;	/* Bidi overflow */
	}
	if (rsp->bidi_underflow) {
		header[1] |= 0x08;	/* Bidi underflow */
	}
	if (rsp->overflow) {
		header[1] |= 0x04;	/* Overflow */
	}
	if (rsp->underflow) {
		header[1] |= 0x02;	/* Underflow  */
	}
	header[2] = rsp->response;	/* iSCSI Response */
	header[3] = rsp->status;/* SCSI Status */
	header[4] = rsp->ahs_len;	/* TotalAHSLength  */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(rsp->length);	/* DataSegmentLength */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(rsp->tag);	/* Task Tag */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(rsp->StatSN);	/* StatSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(rsp->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(rsp->MaxCmdSN);	/* MaxCmdSN */
	*((uint32_t *) (void *) (header + 36)) = ISCSI_HTONL(rsp->ExpDataSN);	/* ExpDataSN  */
	*((uint32_t *) (void *) (header + 40)) = ISCSI_HTONL(rsp->bidi_res_cnt);	/* Bidi Residual Count */
	*((uint32_t *) (void *) (header + 44)) = ISCSI_HTONL(rsp->basic_res_cnt);	/* Residual Count */

	return 0;
}

int 
iscsi_scsi_rsp_decap(uint8_t *header, iscsi_scsi_rsp_t * rsp)
{
	const char	*errmsg;

	if (ISCSI_OPCODE(header) != ISCSI_SCSI_RSP) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	rsp->bidi_overflow = (header[1] & 0x10) ? 1 : 0;	/* Bidi overflow */
	rsp->bidi_underflow = (header[1] & 0x08) ? 1 : 0;	/* Bidi underflow */
	rsp->overflow = (header[1] & 0x04) ? 1 : 0;	/* Overflow */
	rsp->underflow = (header[1] & 0x02) ? 1 : 0;	/* Underflow */

	rsp->response = header[2];	/* iSCSI Response */
	rsp->status = header[3];/* SCSI Status */
	rsp->ahs_len = header[4];	/* TotalAHSLength  */
	rsp->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* DataSegmentLength */
	rsp->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Task Tag  */
	rsp->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN  */
	rsp->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN  */
	rsp->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN  */
	rsp->ExpDataSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 36)));	/* ExpDataSN  */
	rsp->bidi_res_cnt = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 40)));	/* Bidi Residual Count  */
	rsp->basic_res_cnt = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 44)));	/* Residual Count */

	errmsg = NULL;
	if ((header[0] & 0xc0) != 0x0) {
		errmsg = "Byte 0, bits 0-1";
	} else if ((header[1] & 0x80) != 0x80) {
		errmsg = "Byte 1, bit 0";
	} else if (rsp->bidi_res_cnt != 0) {
		errmsg = "bidi_res_cnt";
	} else if (rsp->bidi_overflow != 0) {
		errmsg = "bidi_overflow";
	} else if (rsp->bidi_underflow != 0) {
		errmsg = "bidi_underflow";
	} else if (rsp->overflow != 0) {
		errmsg = "overflow";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Bidi Overflow:       %d\n", rsp->bidi_overflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Bidi Underflow:      %d\n", rsp->bidi_underflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Overflow:            %d\n", rsp->overflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Underflow:           %d\n", rsp->underflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "iSCSI Response:      %u\n", rsp->response);
	iscsi_trace(TRACE_ISCSI_ARGS, "SCSI Status:         %u\n", rsp->status);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength:   %u\n", rsp->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:            %#x\n", rsp->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Residual Count:      %u\n", rsp->basic_res_cnt);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:              %u\n", rsp->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:            %u\n", rsp->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:            %u\n", rsp->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpDataSN:           %u\n", rsp->ExpDataSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Bidi Residual Count: %u\n", rsp->bidi_res_cnt);

	return 0;
}


/*
 * Ready To Transfer
 */

int 
iscsi_r2t_encap(uint8_t *header, iscsi_r2t_t * cmd)
{
	uint32_t        length;

	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", cmd->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:       %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:     %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:     %u\n", cmd->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "R2TSN:        %u\n", cmd->R2TSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Offset:       %u\n", cmd->offset);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= 0x00 | ISCSI_R2T;	/* Opcode  */
	header[1] |= 0x80;
	length = (cmd->AHSlength & 0x00ffffff);	/* AHSLength */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(length);	/* AHSLength */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->transfer_tag);	/* Transfer Tag */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->StatSN);	/* StatSN  */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(cmd->MaxCmdSN);	/* MaxCmdSN */
	*((uint32_t *) (void *) (header + 36)) = ISCSI_HTONL(cmd->R2TSN);	/* R2TSN */
	*((uint32_t *) (void *) (header + 40)) = ISCSI_HTONL(cmd->offset);	/* Buffer Offset */
	*((uint32_t *) (void *) (header + 44)) = ISCSI_HTONL(cmd->length);	/* Transfer Length */

	return 0;
}

int 
iscsi_r2t_decap(uint8_t *header, iscsi_r2t_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[12];

	if (ISCSI_OPCODE(header) != ISCSI_R2T) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->AHSlength = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* TotalAHSLength */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));
	cmd->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));
	cmd->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));
	cmd->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));
	cmd->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));
	cmd->R2TSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 36)));
	cmd->offset = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 40)));
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 44)));

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[1] & 0x7f) != 0x0) {
		errmsg = "Byte 1, bits 1-7";
	} else if (header[2] != 0) {
		errmsg = "Byte 2";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (memcmp(header + 4, zeros, 12) != 0) {
		errmsg = "Bytes 4-15";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "AHSLength:    %u\n", cmd->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:          %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Tag:          %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag: %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:       %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:     %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:     %u\n", cmd->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "R2TSN:        %u\n", cmd->R2TSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Offset:       %u\n", cmd->offset);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:       %u\n", cmd->length);
	return 0;
}

/*
 * SCSI Write Data
 */

int 
iscsi_write_data_encap(uint8_t *header, iscsi_write_data_t * cmd)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Final:              %u\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength:  %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:                %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:           %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag:       %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:          %u\n", cmd->ExpStatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSN:             %u\n", cmd->DataSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Buffer Offset:      %u\n", cmd->offset);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);
	header[0] = 0x00 | ISCSI_WRITE_DATA;	/* Opcode  */
	if (cmd->final) {
		header[1] |= 0x80;	/* Final */
	}
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(cmd->length);	/* Length */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->tag);	/* Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->transfer_tag);	/* Transfer Tag */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpStatSN);	/* ExpStatSN */
	*((uint32_t *) (void *) (header + 36)) = ISCSI_HTONL(cmd->DataSN);	/* DataSN */
	*((uint32_t *) (void *) (header + 40)) = ISCSI_HTONL(cmd->offset);	/* Buffer Offset */

	return 0;
}

int 
iscsi_write_data_decap(uint8_t *header, iscsi_write_data_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_WRITE_DATA) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->final = (header[1] & 0x80) ? 1 : 0;	/* Final */
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));	/* LUN */
	cmd->tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Tag */
	cmd->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Transfer Tag */
	cmd->ExpStatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpStatSN  */
	cmd->DataSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 36)));	/* DataSN    */
	cmd->offset = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 40)));	/* Buffer Offset */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[1] & 0x7f) != 0x0) {
		errmsg = "Byte 1, bits 1-7";
	} else if (header[2] != 0) {
		errmsg = "Byte 2";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (header[4] != 0) {
		errmsg = "Byte 4";
	} else if (memcmp(header + 24, zeros, 4) != 0) {
		errmsg = "Bytes 24-27";
	} else if (memcmp(header + 32, zeros, 4) != 0) {
		errmsg = "Bytes 32-35";
	} else if (memcmp(header + 44, zeros, 4) != 0) {
		errmsg = "Bytes 44-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:              %u\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength:  %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:                %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:           %#x\n", cmd->tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag:       %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpStatSN:          %u\n", cmd->ExpStatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSN:             %u\n", cmd->DataSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Buffer Offset:      %u\n", cmd->offset);

	return 0;
}

/*
 * SCSI Read Data
 */

int 
iscsi_read_data_encap(uint8_t *header, iscsi_read_data_t * cmd)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Final:             %d\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Acknowledge:       %d\n", cmd->ack);
	iscsi_trace(TRACE_ISCSI_ARGS, "Overflow:          %d\n", cmd->overflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Underflow:         %d\n", cmd->underflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "S_bit:             %d\n", cmd->S_bit);
	iscsi_trace(TRACE_ISCSI_ARGS, "Status:            %u\n", cmd->status);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:               %" PRIu64 "\n", cmd->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", cmd->task_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Transfer Tag:      %#x\n", cmd->transfer_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:            %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:          %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:          %u\n", cmd->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSN:            %u\n", cmd->DataSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Buffer Offset      %u\n", cmd->offset);
	iscsi_trace(TRACE_ISCSI_ARGS, "Residual Count:    %u\n", cmd->res_count);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] = 0x00 | ISCSI_READ_DATA;	/* Opcode  */
	if (cmd->final) {
		header[1] |= 0x80;	/* Final */
	}
	if (cmd->ack) {
		header[1] |= 0x40;	/* ACK */
	}
	if (cmd->overflow) {
		header[1] |= 0x04;	/* Overflow  */
	}
	if (cmd->underflow) {
		header[1] |= 0x02;	/* Underflow */
	}
	if (cmd->S_bit) {
		header[1] |= 0x01;	/* S Bit */
	}
	if (cmd->S_bit) {
		header[3] = cmd->status;	/* Status  */
	}
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(cmd->length);	/* Length */
	*((uint64_t *) (void *) (header + 8)) = ISCSI_HTONLL(cmd->lun);	/* LUN */
	*((uint32_t *) (void *) (header + 16)) = ISCSI_HTONL(cmd->task_tag);	/* Task Tag */
	*((uint32_t *) (void *) (header + 20)) = ISCSI_HTONL(cmd->transfer_tag);	/* Transfer Tag */
	if (cmd->S_bit) {
		*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->StatSN);	/* StatSN */
	}
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpCmdSN);	/* ExpCmdSN  */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(cmd->MaxCmdSN);	/* MaxCmdSN  */
	*((uint32_t *) (void *) (header + 36)) = ISCSI_HTONL(cmd->DataSN);	/* DataSN  */
	*((uint32_t *) (void *) (header + 40)) = ISCSI_HTONL(cmd->offset);	/* Buffer Offset */
	if (cmd->S_bit) {
		*((uint32_t *) (void *) (header + 44)) = ISCSI_HTONL(cmd->res_count);	/* Residual Count  */
	}

	return 0;
}

int 
iscsi_read_data_decap(uint8_t *header, iscsi_read_data_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[16];

	if (ISCSI_OPCODE(header) != ISCSI_READ_DATA) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->final = (header[1] & 0x80) ? 1 : 0;	/* Final */
	cmd->ack = (header[1] & 0x40) ? 1 : 0;	/* Acknowledge */
	cmd->overflow = (header[1] & 0x04) ? 1 : 0;	/* Overflow  */
	cmd->underflow = (header[1] & 0x02) ? 1 : 0;	/* Underflow  */
	cmd->S_bit = (header[1] & 0x01) ? 1 : 0;	/* S Bit  */
	cmd->status = header[3];/* Status */
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));		/* LUN  */
	cmd->task_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 16)));	/* Task Tag */
	cmd->transfer_tag = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 20)));	/* Transfer Tag  */
	cmd->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN  */
	cmd->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN  */
	cmd->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN  */
	cmd->DataSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 36)));	/* DataSN  */
	cmd->offset = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 40)));	/* Buffer Offset */
	cmd->res_count = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 44)));	/* Residual Count  */
	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[0] & 0xc0) != 0x0) {
		errmsg = "Byte 0, bits 0-1";
	} else if ((header[1] & 0x38) != 0x0) {
		errmsg = "Byte 1, bits 2-4";
	} else if (header[2] != 0) {
		errmsg = "Byte 2";
	} else if (header[4] != 0) {
		errmsg = "Byte 4";
	} else if (memcmp(header + 8, zeros, 8) != 0) {
		errmsg = "Bytes 8-15";
	} else if (!cmd->underflow && memcmp(header + 44, zeros, 4) != 0) {
		errmsg = "Bytes 44-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Final:             %d\n", cmd->final);
	iscsi_trace(TRACE_ISCSI_ARGS, "Acknowledge:       %d\n", cmd->ack);
	iscsi_trace(TRACE_ISCSI_ARGS, "Overflow:          %d\n", cmd->overflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "Underflow:         %d\n", cmd->underflow);
	iscsi_trace(TRACE_ISCSI_ARGS, "S_bit:             %d\n", cmd->S_bit);
	iscsi_trace(TRACE_ISCSI_ARGS, "Status:            %u\n", cmd->status);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "Task Tag:          %#x\n", cmd->task_tag);
	iscsi_trace(TRACE_ISCSI_ARGS, "Residual Count:    %u\n", cmd->res_count);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:            %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:          %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:          %u\n", cmd->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSN:            %u\n", cmd->DataSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "Buffer Offset      %u\n", cmd->offset);
	return 0;
}

/*
 * Reject
 */

int 
iscsi_reject_encap(uint8_t *header, iscsi_reject_t * cmd)
{

	iscsi_trace(TRACE_ISCSI_ARGS, "Reason:   %u\n", cmd->reason);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:   %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:   %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN: %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN: %u\n", cmd->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSN:   %u\n", cmd->DataSN);

	(void) memset(header, 0x0, ISCSI_HEADER_LEN);

	header[0] |= 0x00 | ISCSI_REJECT;	/* Opcode  */
	header[1] |= 0x80;
	header[2] = cmd->reason;/* Reason */
	*((uint32_t *) (void *) (header + 4)) = ISCSI_HTONL(cmd->length);	/* Length  */
	*((uint32_t *) (void *) (header + 24)) = ISCSI_HTONL(cmd->StatSN);	/* StatSN */
	*((uint32_t *) (void *) (header + 28)) = ISCSI_HTONL(cmd->ExpCmdSN);	/* ExpCmdSN */
	*((uint32_t *) (void *) (header + 32)) = ISCSI_HTONL(cmd->MaxCmdSN);	/* MaxCmdSN */
	*((uint32_t *) (void *) (header + 36)) = ISCSI_HTONL(cmd->DataSN);	/* DataSN */

	return 0;
}

int 
iscsi_reject_decap(uint8_t *header, iscsi_reject_t * cmd)
{
	const char	*errmsg;
	uint8_t		 zeros[8];

	if (ISCSI_OPCODE(header) != ISCSI_REJECT) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	cmd->reason = header[2];/* Reason */
	cmd->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	cmd->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN */
	cmd->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN */
	cmd->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN */
	cmd->DataSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 36)));	/* DataSN */

	errmsg = NULL;
	(void) memset(zeros, 0x0, sizeof(zeros));
	if ((header[0] & 0xc0) != 0x0) {
		errmsg = "Byte 0, bits 0-1";
	} else if ((header[1] & 0x7f) != 0x0) {
		errmsg = "Byte 1, bits 0-7";
	} else if (header[3] != 0) {
		errmsg = "Byte 3";
	} else if (header[4] != 0) {
		errmsg = "Byte 4";
	} else if (memcmp(header + 8, zeros, 8) != 0) {
		errmsg = "Bytes 8-15";
	} else if (memcmp(header + 20, zeros, 4) != 0) {
		errmsg = "Bytes 20-23";
	} else if (memcmp(header + 40, zeros, 8) != 0) {
		errmsg = "Bytes 40-47";
	}
	if (errmsg) {
		iscsi_err(__FILE__, __LINE__, "%s", errmsg);
		NO_CLEANUP;
		return 1;
	}
	iscsi_trace(TRACE_ISCSI_ARGS, "Reason:   %u\n", cmd->reason);
	iscsi_trace(TRACE_ISCSI_ARGS, "Length:   %u\n", cmd->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:   %u\n", cmd->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN: %u\n", cmd->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN: %u\n", cmd->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSN:   %u\n", cmd->DataSN);
	return 0;
}

int 
iscsi_amsg_decap(uint8_t *header, iscsi_async_msg_t * msg)
{

	if (ISCSI_OPCODE(header) != ISCSI_ASYNC) {
		iscsi_err(__FILE__, __LINE__, "Opcode");
		return 1;
	}
	msg->AHSlength = header[4];	/* TotalAHSLength */
	msg->length = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 4)));	/* Length */
	msg->lun = ISCSI_NTOHLL(*((uint64_t *) (void *) (header + 8)));		/* LUN  */
	msg->StatSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 24)));	/* StatSN */
	msg->ExpCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 28)));	/* ExpCmdSN */
	msg->MaxCmdSN = ISCSI_NTOHL(*((uint32_t *) (void *) (header + 32)));	/* MaxCmdSN  */
	msg->AsyncEvent = header[36];	/* Async Event */
	msg->AsyncVCode = header[37];	/* Async Vendor Code */

	iscsi_trace(TRACE_ISCSI_ARGS, "TotalAHSLength:    %u\n", msg->AHSlength);
	iscsi_trace(TRACE_ISCSI_ARGS, "DataSegmentLength: %u\n", msg->length);
	iscsi_trace(TRACE_ISCSI_ARGS, "LUN:               %" PRIu64 "\n", msg->lun);
	iscsi_trace(TRACE_ISCSI_ARGS, "StatSN:            %u\n", msg->StatSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "ExpCmdSN:          %u\n", msg->ExpCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "MaxCmdSN:          %u\n", msg->MaxCmdSN);
	iscsi_trace(TRACE_ISCSI_ARGS, "AsyncEvent:      %u\n", msg->AsyncEvent);
	iscsi_trace(TRACE_ISCSI_ARGS, "AsyncVCode:     %u\n", msg->AsyncVCode);

	return 0;
}
