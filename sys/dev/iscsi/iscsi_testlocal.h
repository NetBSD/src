/*	$wasabi: iscsi_testlocal.h,v 1.5 2006/05/30 23:22:20 twagner Exp $ */
/*	$NetBSD: iscsi_testlocal.h,v 1.1 2011/10/23 21:15:02 agc Exp $	*/

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
#ifndef _ISCSI_TESTLOCAL_H
#define _ISCSI_TESTLOCAL_H

#ifdef ISCSI_TEST_MODE

typedef struct mod_desc_s
{
	TAILQ_ENTRY(mod_desc_s) link;
	void *pdu_ptr;			/* get only */
	iscsi_test_add_modification_parameters_t pars;
	iscsi_pdu_mod_t mods[0];
} mod_desc_t;


typedef struct neg_desc_s
{
	TAILQ_ENTRY(neg_desc_s) link;
	iscsi_test_negotiation_descriptor_t entry;
} neg_desc_t;


TAILQ_HEAD(mod_list_s, mod_desc_s);
typedef struct mod_list_s mod_list_t;

TAILQ_HEAD(neg_list_s, neg_desc_s);
typedef struct neg_list_s neg_list_t;

#define CNT_TX    0
#define CNT_RX    1

typedef struct test_pars_s
{
	TAILQ_ENTRY(test_pars_s) link;	/* links tests to be assigned */
	uint32_t test_id;			/* Test ID */
	connection_t *connection;	/* Attached Connection */
	uint16_t options;			/* Test options */
	uint8_t lose_random[2];	/* 0 disables, else mod value */
	uint32_t r2t_val;			/* Used only if NEGOTIATE_R2T */
	uint32_t maxburst_val;		/* Used only if NEGOTIATE_MAXBURST */
	uint32_t firstburst_val;	/* Used only if NEGOTIATE_FIRSTBURST */
	neg_list_t negs;			/* list of negotiation descriptors */
	mod_list_t mods;			/* list of modification decriptors */
	uint32_t pdu_count[INVALID_PDU + 1][2];	/* PDU counter */
	uint32_t pdu_last[INVALID_PDU + 1][2];	/* last PDU counter */
} test_pars_t;


TAILQ_HEAD(test_pars_list_s, test_pars_s);
typedef struct test_pars_list_s test_pars_list_t;

void test_assign_connection(connection_t *);
void test_remove_connection(connection_t *);

#define TEST_INVALID_HEADER_CRC     -99
#define TEST_READ_ERROR             1
#define TEST_INVALID_DATA_CRC       -1

int test_mode_rx(connection_t *, pdu_t *, int);
int test_mode_tx(connection_t *, pdu_t *);

void test_define(iscsi_test_define_parameters_t *);
void test_add_neg(iscsi_test_add_negotiation_parameters_t *);
void test_add_mod(struct proc *, iscsi_test_add_modification_parameters_t *);
void test_send_pdu(struct proc *, iscsi_test_send_pdu_parameters_t *);
void test_cancel(iscsi_test_cancel_parameters_t *);

#define test_ccb_timeout(conn) ((conn->test_pars == NULL) ? 1 \
      : !(conn->test_pars->options & ISCSITEST_OPT_DISABLE_CCB_TIMEOUT))

#define test_conn_timeout(conn) ((conn->test_pars == NULL) ? 1 \
      : !(conn->test_pars->options & ISCSITEST_OPT_DISABLE_CONN_TIMEOUT))

#endif /* TEST_MODE */
#endif /* TESTLOCAL_H */
