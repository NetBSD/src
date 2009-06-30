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
#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

typedef struct MD5Context MD5Context_t;


#define ISCSI_PARAM_MAX_LEN 256
#define ISCSI_PARAM_KEY_LEN 64

/* Parameter Type */

#define ISCSI_PARAM_TYPE_DECLARATIVE    1
#define ISCSI_PARAM_TYPE_DECLARE_MULTI  2	/* for TargetName and
						 * TargetAddress */
#define ISCSI_PARAM_TYPE_NUMERICAL      3
#define ISCSI_PARAM_TYPE_NUMERICAL_Z    4	/* zero represents no limit */
#define ISCSI_PARAM_TYPE_BINARY_OR      5
#define ISCSI_PARAM_TYPE_BINARY_AND     6
#define ISCSI_PARAM_TYPE_LIST           7

#define ISCSI_CHAP_DATA_LENGTH          16
#define ISCSI_CHAP_STRING_LENGTH        256

#define ISCSI_PARAM_STATUS_AUTH_FAILED	-2
#define ISCSI_PARAM_STATUS_FAILED	-1

/* types of authentication which the initiator can select */
enum {
	AuthNone,
	AuthCHAP,
	AuthKerberos,
	AuthSRP
};

/* types of digest which the initiator can select */
enum {
	DigestNone	= 0x0,
	DigestHeader	= 0x01,
	DigestData	= 0x02
	/* these are bit masks, extend accordingly */
};

typedef struct iscsi_parameter_item_t {
	char				 value[ISCSI_PARAM_MAX_LEN];
	struct iscsi_parameter_item_t	*next;
} iscsi_parameter_value_t;

/* this struct defines the credentials a user has */
typedef struct iscsi_cred_t {
	char		*user;		/* user's name */
	char		*auth_type;	/* preferred authentication type */
	char		*shared_secret;	/* the shared secret */
} iscsi_cred_t;

/*
 * Structure for storing negotiated parameters that are frequently accessed
 * on an active session
 */
typedef struct iscsi_sess_param_t {
	uint32_t        max_burst_length;
	uint32_t        first_burst_length;
	uint32_t        max_dataseg_len;
	iscsi_cred_t	cred;
	uint8_t         initial_r2t;
	uint8_t         immediate_data;
	uint8_t		header_digest;
	uint8_t		data_digest;
	uint8_t		auth_type;
	uint8_t		mutual_auth;
	uint8_t		digest_wanted;
} iscsi_sess_param_t;

typedef struct iscsi_parameter_t {
	char            key[ISCSI_PARAM_KEY_LEN];	/* key */
	int             type;	/* type of parameter */
	char            valid[ISCSI_PARAM_MAX_LEN];	/* list of valid values */
	char            dflt[ISCSI_PARAM_MAX_LEN];	/* default value */
	iscsi_parameter_value_t *value_l;	/* value list */
	char            offer_rx[ISCSI_PARAM_MAX_LEN];	/* outgoing offer */
	char            offer_tx[ISCSI_PARAM_MAX_LEN];	/* incoming offer */
	char            answer_tx[ISCSI_PARAM_MAX_LEN];	/* outgoing answer */
	char            answer_rx[ISCSI_PARAM_MAX_LEN];	/* incoming answer */
	char            negotiated[ISCSI_PARAM_MAX_LEN];	/* negotiated value */
	int             tx_offer;	/* sent offer  */
	int             rx_offer;	/* received offer  */
	int             tx_answer;	/* sent answer */
	int             rx_answer;	/* received answer */
	int             reset;	/* reset value_l */
	struct iscsi_parameter_t *next;
} iscsi_parameter_t;

int             param_list_add(iscsi_parameter_t ** , int , const char *, const char *, const char *);
int             param_list_print(iscsi_parameter_t * );
int             param_list_destroy(iscsi_parameter_t * );
int             param_text_add(iscsi_parameter_t *, const char *, const char *, char *, int *, int, int );
int             param_text_parse(iscsi_parameter_t *, iscsi_cred_t *, char *, int , char *, int *, int, int);
int             param_text_print(char *, uint32_t );
int             param_num_vals(iscsi_parameter_t * , char *);
int             param_val_reset(iscsi_parameter_t * , const char *);
char           *param_val(iscsi_parameter_t * , const char *);
char           *param_val_which(iscsi_parameter_t * , const char *, int );
char           *param_offer(iscsi_parameter_t * , char *);
char           *param_answer(iscsi_parameter_t * , char *);
iscsi_parameter_t *param_get(iscsi_parameter_t * , const char *);
int		driver_atoi(const char *);
int             param_atoi(iscsi_parameter_t * , const char *);
int             param_equiv(iscsi_parameter_t * , const char *, const char *);
int             param_commit(iscsi_parameter_t * );
void            set_session_parameters(iscsi_parameter_t * , iscsi_sess_param_t * );

/*
 * Macros
 */

#define PARAM_LIST_DESTROY(LIST, ELSE)                    \
if (param_list_destroy(LIST)!=0) {                             \
  iscsi_err(__FILE__, __LINE__, "param_list_destroy() failed\n");                \
  ELSE;                                                        \
}

#define PARAM_LIST_ADD(LIST, TYPE, KEY, DFLT, VALID, ELSE)   \
if (param_list_add(LIST, TYPE, KEY, DFLT, VALID)!=0) {            \
  iscsi_err(__FILE__, __LINE__, "param_list_add() failed\n");                       \
  ELSE;                                                           \
}

#define PARAM_TEXT_ADD(LIST, KEY, VAL, TEXT, LEN, SIZE, OFFER, ELSE )  \
if (param_text_add(LIST, KEY, VAL, TEXT, LEN, SIZE, OFFER)!=0) {            \
  iscsi_err(__FILE__, __LINE__, "param_text_add() failed\n");                           \
  ELSE;                                                               \
}

#define PARAM_TEXT_PARSE(LIST, CRED, TEXT, LEN, TEXT_OUT, LEN_OUT, SIZE, OFFER, ELSE )   \
if (param_text_parse(LIST, CRED, TEXT, LEN, TEXT_OUT, LEN_OUT, SIZE, OFFER)!=0) {             \
  iscsi_err(__FILE__, __LINE__, "param_text_parse_offer() failed\n");                               \
  ELSE;                                                                           \
}
#endif
