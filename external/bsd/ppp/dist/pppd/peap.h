/*
 * Copyright (c) 2011 Rustam Kovhaev. All rights reserved.
 * Copyright (c) 2021 Eivind Næss. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PPP_PEAP_H
#define	PPP_PEAP_H

#include "pppdconf.h"

#define	PEAP_PHASE_1			1
#define	PEAP_PHASE_2			2

#define	PEAP_HEADERLEN			6
#define	PEAP_FRAGMENT_LENGTH_FIELD	4
#define	PEAP_FLAGS_FIELD		1
#define	PEAP_FLAGS_ACK			0

#define PEAP_CAPABILITIES_TYPE		254
#define PEAP_CAPABILITIES_LEN		12

#define PEAP_TLV_TYPE			12
#define PEAP_TLV_LENGTH_FIELD		56
#define PEAP_TLV_SUBTYPE_REQUEST	0
#define PEAP_TLV_SUBTYPE_RESPONSE	1
#define PEAP_TLV_HEADERLEN		8
#define PEAP_TLV_RESULT_LEN		7
#define PEAP_TLV_LEN			71

/*
 * Microsoft PEAP client/server never exchange
 * outer TLVs during PEAP authentication
 */
#define	PEAP_TLV_DATA_LEN		61

#define	PEAP_TLV_TK_LEN			60
#define	PEAP_TLV_ISK_LEN		32
#define	PEAP_TLV_IPMKSEED_LEN		59
#define	PEAP_TLV_TEMPKEY_LEN		40
#define	PEAP_TLV_IPMK_LEN		40
#define	PEAP_TLV_CMK_LEN		20
#define	PEAP_TLV_NONCE_LEN		32
#define	PEAP_TLV_COMP_MAC_LEN		20
#define	PEAP_TLV_CSK_LEN		128
#define	PEAP_TLV_TK_SEED_LABEL		"client EAP encryption"
#define	PEAP_TLV_IPMK_SEED_LABEL	"Inner Methods Compound Keys"
#define	PEAP_TLV_CSK_SEED_LABEL		"Session Key Generating Function"

#define	PEAP_S_FLAG_SET			0x20
#define	PEAP_L_FLAG_SET			0x80
#define	PEAP_LM_FLAG_SET		0xC0
#define	PEAP_M_FLAG_SET			0x40
#define	PEAP_NO_FLAGS			0x00

#define	EAP_TLS_KEY_LEN			0x40
#define	TLS_RECORD_MAX_SIZE		0x4000

struct peap_state;

/**
 * Initialize the PEAP structure
 */
int peap_init(struct peap_state** psm, const char *remote_name);

/**
 * Process a PEAP packet
 */
int peap_process(eap_state *esp, u_char id, u_char *inp, int len);

/**
 * Clean up the PEAP structure
 */
void peap_finish(struct peap_state **psm);

#endif /* PPP_PEAP_H */
