/*	$NetBSD: iscsi_test.h,v 1.1 2011/10/23 21:15:02 agc Exp $	*/

/*-
 * Copyright (c) 2006,2011 The NetBSD Foundation, Inc.
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

#ifndef _ISCSI_TEST_H
#define _ISCSI_TEST_H

#ifdef ISCSI_TEST_MODE

/* Status codes returned in test mode only */

#define ISCSI_STATUS_TEST_INACTIVE				5001	/* Test not assigned to connection */
#define ISCSI_STATUS_TEST_CANCELED				5002	/* Test was cancelled */
#define ISCSI_STATUS_TEST_CONNECTION_CLOSED		5003	/* Connection closed */
#define ISCSI_STATUS_TEST_MODIFICATION_SKIPPED	5004	/* Modification skipped due to bad offset */
#define ISCSI_STATUS_TEST_HEADER_CRC_ERROR		5005	/* Received PDU had bad header CRC */
#define ISCSI_STATUS_TEST_DATA_CRC_ERROR		5006	/* Received PDU had bad data CRC */
#define ISCSI_STATUS_TEST_DATA_READ_ERROR		5007	/* Error while receiving PDU */
#define ISCSI_STATUS_TEST_ALREADY_ASSIGNED		5008	/* Different test already assigned */

/*
   PDU modification descriptor.
*/

#define ISCSITEST_MOD_FLAG_ADD_VAL		0x01	/* Add value to field */
												/* (default is replace) */
#define ISCSITEST_MOD_FLAG_REORDER		0x02	/* Put bytes in network order */

/* Special values for offset field */

#define ISCSITEST_OFFSET_HEADERDIGEST	(-1)	/* offset is header digest */
#define ISCSITEST_OFFSET_DATA			(-2)	/* offset is start of data */
#define ISCSITEST_OFFSET_DATADIGEST		(-3)	/* offset is data digest */
#define ISCSITEST_OFFSET_DRV_CMDSN		(-4)	/* offset is driver's CmdSN */


typedef struct
{
	uint16_t flags;			/* flags */
	uint16_t size;				/* size of field in bytes (1..8) */
	int32_t offset;				/* offset into PDU */
	uint8_t value[8];			/* value to use */
} iscsi_pdu_mod_t;

/* PDU kinds */

typedef enum
{
	ANY_PDU,					/* don't care */
	COMMAND_PDU,
	RESPONSE_PDU,
	TASK_REQ_PDU,
	TASK_RSP_PDU,
	DATA_OUT_PDU,
	DATA_IN_PDU,
	R2T_PDU,
	ASYNCH_PDU,
	TEXT_REQ_PDU,
	TEXT_RSP_PDU,
	LOGIN_REQ_PDU,
	LOGIN_RSP_PDU,
	LOGOUT_REQ_PDU,
	LOGOUT_RSP_PDU,
	SNACK_PDU,
	REJECT_PDU,
	NOP_OUT_PDU,
	NOP_IN_PDU,
	INVALID_PDU
} iscsi_pdu_kind_t;

/* types of offsets supported for determining PDU offset */

typedef enum
{
	ABSOLUTE_ANY,  			/* absolute to beginning of connection, any kind */
	RELATIVE_ANY,  			/* relative to last modified PDU, any kind */
	ABSOLUTE_PDUKIND,  		/* absolute to beginning of connection, same kind */
	RELATIVE_PDUKIND,  		/* relative to last modified PDU, same kind */
	ABSOLUTE_TX,	   		/* absolute to beginning of connection, send only */
	RELATIVE_TX,	   		/* relative to last modified PDU, send only */
	ABSOLUTE_RX,	   		/* absolute to beginning of connection, receive only */
	RELATIVE_RX		   		/* relative to last modified PDU, receive only */
} iscsi_pdu_offset_t;


/*
   Negotiation parameters
*/

/* Negotiation states */

typedef enum
{
	NEG_SECURITY,				/* First security negotiation PDU (method) */
	NEG_CHAP_ALG,				/* CHAP: Algorithm specification */
	NEG_CHAP_NAME_RESPONSE,		/* CHAP: Name and Response */
	NEG_OP_NEG					/* in Operational negotiation stage */
} iscsi_neg_state_t;

#define ISCSITEST_NEGOPT_REPLACE	0x01	/* Replace existing parameters */
											/* (default is append) */

typedef struct
{
	iscsi_neg_state_t state;	/* which negotiation stage */
	uint16_t flags;			/* flags */
	uint16_t size;				/* size of value in bytes */
	uint8_t value[0];			/* value to use */
} iscsi_test_negotiation_descriptor_t;


/*
   Define and add test parameters
      The calling application defines the test ID. Duplicate IDs are rejected.
*/

/* Options for define_parameters only */

#define ISCSITEST_NEGOTIATE_R2T				0x00000100	/* Negotiate MaxOutstandingR2T */
#define ISCSITEST_NEGOTIATE_MAXBURST		0x00000200	/* Negotiate MaxBurstLength */
#define ISCSITEST_NEGOTIATE_FIRSTBURST		0x00000400	/* Negotiate FirstBurstLength */
#define ISCSITEST_OVERRIDE_INITIALR2T		0x00001000	/* Override default InitialR2T (set to TRUE) */
#define ISCSITEST_OVERRIDE_IMMDATA			0x00002000	/* Override default ImmediateData (set to FALSE) */

/* options for both define and add_modification parameters */

#define ISCSITEST_OPT_DISABLE_CCB_TIMEOUT	0x00020000	/* Disable CCB timeout */
#define ISCSITEST_OPT_ENABLE_CCB_TIMEOUT	0x00040000	/* Enable CCB timeout */
#define ISCSITEST_OPT_DISABLE_CONN_TIMEOUT	0x00080000	/* Disable connection timeout */
#define ISCSITEST_OPT_ENABLE_CONN_TIMEOUT	0x00100000	/* Enable connection timeout */
#define ISCSITEST_OPT_USE_RANDOM_TX			0x00200000	/* Use random tx loss parameter */
#define ISCSITEST_OPT_USE_RANDOM_RX			0x00400000	/* Use random rx loss parameter */

/* options for add_modification parameters only */

#define ISCSITEST_OPT_WAIT_FOR_COMPLETION	0x00000001	/* Wait until processing done */

#define ISCSITEST_OPT_MOD_PERMANENT			0x20000000	/* PDU header mods are permanent */
#define ISCSITEST_OPT_NO_RESPONSE_PDU		0x40000000	/* Do not expect response to this PDU */
#define ISCSITEST_OPT_DISCARD_PDU			0x80000000	/* Don't send PDU/process PDU in driver */

/* options for add_modification and send_pdu */

#define ISCSITEST_KILL_CONNECTION			0x08000000	/* Kill connection when done */

#define ISCSITEST_SFLAG_NO_HEADER_DIGEST	0x00000020	/* Do not update header digest */
#define ISCSITEST_SFLAG_NO_DATA_DIGEST		0x00000040	/* Do not update data digest */

/* options for send_pdu only */

#define ISCSITEST_SFLAG_UPDATE_FIELDS		0x00000010	/* Update fields in PDU */
#define ISCSITEST_SFLAG_NO_PADDING			0x00000080	/* Do not add padding */

/* The parameter for TEST_DEFINE ioctl */

typedef struct
{
	uint32_t test_id;			/* Test ID */
	uint32_t options;			/* Test options */
	uint32_t session_id;		/* Session to attach to (may be 0) */
	uint32_t connection_id;	/* Connection to attach to (may be 0) */
	uint8_t lose_random_tx;	/* 0 disables, else mod value */
	uint8_t lose_random_rx;	/* 0 disables, else mod value */
	uint32_t r2t_val;			/* Used only if NEGOTIATE_R2T */
	uint32_t maxburst_val;		/* Used only if NEGOTIATE_MAXBURST */
	uint32_t firstburst_val;	/* Used only if NEGOTIATE_FIRSTBURST */
	uint32_t neg_descriptor_size;	/* Size of negotiation descriptor (may be 0) */
	void *neg_descriptor_ptr;	/* pointer to negotiation descriptor */
	uint32_t status;			/* Out: Status */
} iscsi_test_define_parameters_t;


/* The parameter for TEST_ADD_MODIFICATION ioctl */

typedef struct
{
	uint32_t test_id;			/* Test ID */
	uint32_t options;			/* Test options */
	uint8_t lose_random_tx;	/* 0 disables, else mod value */
	uint8_t lose_random_rx;	/* 0 disables, else mod value */
	iscsi_pdu_kind_t which_pdu;	/* Which kind of PDU to act on */
	iscsi_pdu_offset_t which_offset;	/* Which offset to use */
	uint32_t pdu_offset;		/* Offset of PDU to act on (0 means next) */
	uint32_t num_pdu_mods;		/* How many PDU modifications */
	iscsi_pdu_mod_t *mod_ptr;	/* pointer to modifications */
	uint32_t pdu_size;			/* size of PDU buffer for get */
	void *pdu_ptr;			/* pointer to PDU buffer */
	uint32_t pdu_actual_size;	/* Out: actual size of PDU */
	uint32_t status;			/* Out: Status */
} iscsi_test_add_modification_parameters_t;


/* The parameter for TEST_ADD_NEGOTIATION ioctl */

typedef struct
{
	uint32_t test_id;			/* Test ID */
	uint32_t neg_descriptor_size;	/* Size of negotiation descriptor (may be 0) */
	void *neg_descriptor_ptr;	/* pointer to negotiation descriptor */
	uint32_t status;			/* Out: Status */
} iscsi_test_add_negotiation_parameters_t;


/*
   Cancel test parameters
*/

typedef struct
{
	uint32_t test_id;			/* Test ID */
	uint32_t status;			/* Out: Status */
} iscsi_test_cancel_parameters_t;

/*
   Send PDU parameters
*/

typedef struct
{
	uint32_t test_id;			/* Test ID */
	uint32_t options;			/* Test options */
	uint32_t pdu_size;			/* size of PDU */
	void *pdu_ptr;				/* pointer to PDU */
	uint32_t status;			/* Out: Status */
} iscsi_test_send_pdu_parameters_t;


/* IOCTL codes */

#define ISCSI_TEST_DEFINE           _IOWR(0, 100, iscsi_test_define_parameters_t)
#define ISCSI_TEST_ADD_NEGOTIATION  _IOWR(0, 101, iscsi_test_add_negotiation_parameters_t)
#define ISCSI_TEST_ADD_MODIFICATION _IOWR(0, 102, iscsi_test_add_modification_parameters_t)
#define ISCSI_TEST_SEND_PDU         _IOWR(0, 103, iscsi_test_send_pdu_parameters_t)
#define ISCSI_TEST_CANCEL           _IOWR(0, 109, iscsi_test_cancel_parameters_t)

#endif
#endif
