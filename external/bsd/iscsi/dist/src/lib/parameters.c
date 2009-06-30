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
#include "compat.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "iscsi-md5.h"
#include "iscsiutil.h"
#include "parameters.h"
#include "conffile.h"


int 
param_list_add(iscsi_parameter_t ** head, int type, const char *key, const char *dflt, const char *valid)
{
	iscsi_parameter_t *param;

	/* Allocated new parameter type */

	if (*head == NULL) {
		if ((*head = iscsi_malloc_atomic(sizeof(iscsi_parameter_t))) == NULL) {
			iscsi_err(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}
		param = *head;
	} else {
		for (param = *head; param->next != NULL; param = param->next) {
		}
		if ((param->next = iscsi_malloc_atomic(sizeof(iscsi_parameter_t))) == NULL) {
			iscsi_err(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}
		param = param->next;
	}

	/* Initilized parameter */

	param->type = type;	/* type */
	(void) strlcpy(param->key, key, sizeof(param->key));/* key */
	(void) strlcpy(param->dflt, dflt, sizeof(param->dflt));	/* default value */
	(void) strlcpy(param->valid, valid, sizeof(param->valid));	/* list of valid values */
	param->tx_offer = 0;	/* sent offer */
	param->rx_offer = 0;	/* received offer */
	param->tx_answer = 0;	/* sent answer */
	param->rx_answer = 0;	/* received answer */
	param->reset = 0;	/* used to erase value_l on next parse */
	param->next = NULL;	/* terminate list */

	/* Allocated space for value list and set first item to default; and */
	/* set offer and answer lists to NULL */

	if ((param->value_l = iscsi_malloc_atomic(sizeof(iscsi_parameter_value_t))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
		return -1;
	}
	param->value_l->next = NULL;
	(void) strlcpy(param->value_l->value, dflt, sizeof(param->value_l->value));

	/* Arg check */

	switch (type) {
	case ISCSI_PARAM_TYPE_DECLARATIVE:
		break;
	case ISCSI_PARAM_TYPE_DECLARE_MULTI:
		break;
	case ISCSI_PARAM_TYPE_BINARY_OR:
		if (strcmp(valid, "Yes,No") != 0 &&
		    strcmp(valid, "No,Yes") != 0 &&
		    strcmp(valid, "No") != 0 &&
		    strcmp(valid, "Yes") != 0 &&
		    strcmp(valid, "yes,no") != 0 &&
		    strcmp(valid, "no,yes") != 0 &&
		    strcmp(valid, "no") != 0 &&
		    strcmp(valid, "yes") != 0) {
			iscsi_err(__FILE__, __LINE__, "bad <valid> field \"%s\" for ISCSI_PARAM_TYPE_BINARY\n", valid);
			return -1;
		}
		break;
	case ISCSI_PARAM_TYPE_BINARY_AND:
		if (strcmp(valid, "Yes,No") != 0 &&
		    strcmp(valid, "No,Yes") != 0 &&
		    strcmp(valid, "No") != 0 &&
		    strcmp(valid, "Yes") != 0 &&
		    strcmp(valid, "yes,no") != 0 &&
		    strcmp(valid, "no,yes") != 0 &&
		    strcmp(valid, "no") != 0 &&
		    strcmp(valid, "yes") != 0) {
			iscsi_err(__FILE__, __LINE__, "bad <valid> field \"%s\" for ISCSI_PARAM_TYPE_BINARY\n", valid);
			return -1;
		}
		break;
	case ISCSI_PARAM_TYPE_NUMERICAL:
		break;
	case ISCSI_PARAM_TYPE_NUMERICAL_Z:
		break;
	case ISCSI_PARAM_TYPE_LIST:
		break;
	default:
		iscsi_err(__FILE__, __LINE__, "unknown parameter type %d\n", type);
		return -1;
	}

	iscsi_trace(TRACE_ISCSI_PARAM, "\"%s\": valid \"%s\", default \"%s\", current \"%s\"\n",
	      param->key, param->valid, param->dflt, param->value_l->value);
		return 0;
}

int 
param_list_destroy(iscsi_parameter_t * head)
{
	iscsi_parameter_t *ptr, *tmp;
	iscsi_parameter_value_t *item_ptr, *next;

	for (ptr = head; ptr != NULL;) {
		tmp = ptr;
		ptr = ptr->next;
		if (tmp->value_l) {
			for (item_ptr = tmp->value_l; item_ptr != NULL; item_ptr = next) {
				next = item_ptr->next;
				/*
				 * iscsi_trace(TRACE_ISCSI_PARAM, "freeing \"%s\"
				 * (%p)\n", item_ptr->value, item_ptr);
				 */
				iscsi_free_atomic(item_ptr);
			}
		}
		/* iscsi_trace(TRACE_ISCSI_PARAM, "freeing %p\n", tmp); */
		iscsi_free_atomic(tmp);
	}
	return 0;
}


iscsi_parameter_t *
param_get(iscsi_parameter_t * head, const char *key)
{
	iscsi_parameter_t *ptr;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			return ptr;
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return NULL;
}

char           *
param_val(iscsi_parameter_t * head, const char *key)
{
	return param_val_which(head, key, 0);
}

char           *
param_val_which(iscsi_parameter_t * head, const char *key, int which)
{
	iscsi_parameter_t *ptr;
	iscsi_parameter_value_t *item_ptr;
	int             i = 0;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			item_ptr = ptr->value_l;
			for (i = 0; i != which; i++) {
				if (item_ptr == NULL) {
					iscsi_err(__FILE__, __LINE__, "item %d in value list is NULL\n", i);
					return NULL;
				}
				item_ptr = item_ptr->next;
			}
			if (item_ptr == NULL) {
				iscsi_err(__FILE__, __LINE__, "item %d in value list is NULL\n", which);
				return NULL;
			}
			return item_ptr->value;
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return NULL;
}

static int 
param_val_delete_all(iscsi_parameter_t * head, char *key)
{
	iscsi_parameter_t *ptr;
	iscsi_parameter_value_t *item_ptr, *next;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			for (item_ptr = ptr->value_l; item_ptr != NULL; item_ptr = next) {
				next = item_ptr->next;
				iscsi_free_atomic(item_ptr);
			}
			ptr->value_l = NULL;
			return 0;
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return -1;
}

int 
param_val_reset(iscsi_parameter_t * head, const char *key)
{
	iscsi_parameter_t *ptr;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			ptr->reset = 1;
			return 0;
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return -1;
}

int 
param_atoi(iscsi_parameter_t * head, const char *key)
{
	iscsi_parameter_t *ptr;
	char           *value;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			if (ptr->value_l) {
				if ((value = param_val(head, key)) != NULL) {
					return iscsi_atoi(value);
				} else {
					iscsi_err(__FILE__, __LINE__, "value is NULL\n");
					return 0;
				}
			} else {
				iscsi_err(__FILE__, __LINE__, "param \"%s\" has NULL value list\n", key);
				return 0;
			}
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return 0;
}

int 
param_equiv(iscsi_parameter_t * head, const char *key, const char *val)
{
	iscsi_parameter_t *ptr;
	char           *value;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			if (ptr->value_l == NULL) {
				iscsi_err(__FILE__, __LINE__, "param \"%s\" has NULL value list\n", key);
				return 0;
			}
			if ((value = param_val(head, key)) == NULL) {
				iscsi_err(__FILE__, __LINE__, "key \"%s\" value is NULL\n", key);
				return 0;
			}
			return (strcmp(value, val) == 0);
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return -1;
}

int 
param_num_vals(iscsi_parameter_t * head, char *key)
{
	iscsi_parameter_t *ptr;
	iscsi_parameter_value_t *item_ptr;
	int             num = 0;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		if (strcmp(ptr->key, key) == 0) {
			for (item_ptr = ptr->value_l; item_ptr != NULL; item_ptr = item_ptr->next) {
				num++;
			}
			return num;
		}
	}
	iscsi_err(__FILE__, __LINE__, "key \"%s\" not found in param list\n", key);
	return -1;
}

int 
param_list_print(iscsi_parameter_t * head)
{
	iscsi_parameter_t *ptr;
	iscsi_parameter_value_t *item_ptr;

	for (ptr = head; ptr != NULL; ptr = ptr->next) {
		for (item_ptr = ptr->value_l; item_ptr != NULL; item_ptr = item_ptr->next) {
			printf("\"%s\"=\"%s\"\n", ptr->key, item_ptr->value);
		}
	}
	return 0;
}

int 
param_text_print(char *text, uint32_t text_len)
{
	char            key[256];
	char           *ptr, *eq, *value;

	for (ptr = text; (uint32_t)(ptr - text) < text_len; ptr += (strlen(ptr) + 1)) {

		/* Skip over any NULLs */

		while (!(*ptr) && ((uint32_t)(ptr - text) < text_len))
			ptr++;
		if ((uint32_t)(ptr - text) >= text_len)
			break;

		if ((eq = strchr(ptr, '=')) == NULL) {
			iscsi_err(__FILE__, __LINE__, "delimiter \'=\' not found in token \"%s\"\n", ptr);
			return -1;
		}
		strncpy(key, ptr, (unsigned)(eq - ptr));
		key[(int)(eq - ptr)] = 0x0;
		value = eq + 1;
		printf("\"%s\"=\"%s\"\n", key, value);
	}
	return 0;
}

/* ARGSUSED */
int 
param_text_add(iscsi_parameter_t * head, const char *key, const char *value, char *text, int *len, int size, int offer)
{
	int	cc;

	cc = snprintf(text + *len, (unsigned)(size - *len), "%s=%s", key, value);
	*len += cc + 1;
	return 0;
}

int 
driver_atoi(const char *s)
{
	int             k = 0;

	while (*s != 0x0 && *s >= '0' && *s <= '9') {
		k = 10 * k + (*s - '0');
		s++;
	}
	return k;
}

/* find the credentials for `user' and put them in `cred' */
static int
find_credentials(iscsi_cred_t *cred, char *user, const char *auth)
{
	conffile_t	 conf;
	const char	*authtype;
	unsigned	 cc;
	ent_t	 	 e;

	(void) memset(&conf, 0x0, sizeof(conf));
	(void) memset(&e, 0x0, sizeof(e));

	if (!conffile_open(&conf, _PATH_ISCSI_PASSWD, "r", ":", "#")) {
		iscsi_err(__FILE__, __LINE__, "can't open `%s'\n", _PATH_ISCSI_PASSWD);
		exit(EXIT_FAILURE);
	}
	while (conffile_getent(&conf, &e)) {
		if (strcasecmp(e.sv.v[0], user) == 0) {
			authtype = (e.sv.c == 1) ? "none" : e.sv.v[1];
			cc = strlen(authtype);
			if (auth == NULL || (strncasecmp(authtype, auth, cc) == 0 && cc == strlen(auth))) {
				cred->user = strdup(e.sv.v[0]);
				cred->auth_type = strdup(authtype);
				cred->shared_secret = (e.sv.c == 3) ? strdup(e.sv.v[2]) : NULL;
				conffile_close(&conf);
				return 1;
			}
		}
	}
	conffile_close(&conf);
	(void) fprintf(stderr, "No matching user configuration entry for `%s' was found\n", user);
	(void) fprintf(stderr, "Please add an entry for `%s' to `%s'\n", user, _PATH_ISCSI_PASSWD);
	return 0;
}

#if 0
/* free any storage allocated in `cred' */
static void
free_cred(iscsi_cred_t *cred)
{
	if (cred) {
		if (cred->user) {
			iscsi_free_atomic(cred->user);
		}
		if (cred->auth_type) {
			iscsi_free_atomic(cred->auth_type);
		}
		if (cred->shared_secret) {
			iscsi_free_atomic(cred->shared_secret);
		}
	}
}
#endif

/* Security offering and check */
/*
 * ret values: =0: succeed or no security >0: security negotiation in process
 * <0: failed
 */
static int
param_parse_security(iscsi_parameter_t * head,
		     iscsi_parameter_t * param_in,
		     iscsi_cred_t *cred,
		     char *text_out, int *text_len_out, int textsize)
{

	static uint8_t idData;
	static uint8_t chapdata[ISCSI_CHAP_DATA_LENGTH];
	static uint8_t respdata[ISCSI_CHAP_DATA_LENGTH];
	char           *chapstring = NULL;
	iSCSI_MD5_CTX   *context = NULL;
	iscsi_parameter_t *param = NULL;
	int             ret = 1;

	if ((chapstring = iscsi_malloc(ISCSI_CHAP_STRING_LENGTH)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	if ((context = iscsi_malloc(sizeof(*context))) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (chapstring != NULL)
			iscsi_free(chapstring);
		return -1;
	}
#define PPS_CLEANUP { if (chapstring != NULL) iscsi_free(chapstring);if (context != NULL) iscsi_free(context); }
#define PPS_ERROR { PPS_CLEANUP; return (-1); };

	if (strcmp(param_in->key, "AuthMethod") == 0) {
		if (param_in->rx_answer && strcmp(param_in->answer_rx, "None") == 0) {
			PPS_CLEANUP;
			return 0;	/* Proposed None for
					 * Authentication */
		}
		if (param_in->rx_offer && strcmp(param_in->offer_rx, "None") == 0) {
			PPS_CLEANUP;
			return 0;
		}
		if (!param_in->rx_offer) {
			param = param_get(head, "CHAP_A");
			if (param == NULL)
				PPS_ERROR;
			param->tx_offer = 1;	/* sending an offer */
			param->rx_offer = 0;	/* reset */
			(void) strlcpy(param->offer_tx, param->valid, sizeof(param->offer_tx));
			PARAM_TEXT_ADD(head, param->key, param->valid,
				      text_out, text_len_out, textsize, 0, PPS_ERROR);
			ret++;
		}
	} else if (strcmp(param_in->key, "CHAP_A") == 0) {
		if (param_in->rx_offer) {
			PARAM_TEXT_ADD(head, param_in->key, param_in->offer_rx,
				      text_out, text_len_out, textsize, 0, PPS_ERROR);

			if ((param = param_get(head, "CHAP_I")) == NULL) {
				PPS_ERROR;
			}
			param->tx_offer = 1;	/* sending an offer */
			param->rx_offer = 0;	/* reset */
			GenRandomData(&idData, 1);
			(void) snprintf(chapstring, ISCSI_CHAP_STRING_LENGTH, "%d", idData);
			(void) strlcpy(param->offer_tx, chapstring, sizeof(param->offer_tx));
			PARAM_TEXT_ADD(head, param->key, param->offer_tx,
				      text_out, text_len_out, textsize, 0, PPS_ERROR);

			if ((param = param_get(head, "CHAP_C")) == NULL) {
				PPS_ERROR;
			}
			param->tx_offer = 1;	/* sending an offer */
			param->rx_offer = 0;	/* reset */
			GenRandomData(chapdata, ISCSI_CHAP_DATA_LENGTH);
			HexDataToText(chapdata, ISCSI_CHAP_DATA_LENGTH,
				      chapstring, ISCSI_CHAP_STRING_LENGTH);
			(void) strlcpy(param->offer_tx, chapstring, sizeof(param->offer_tx));
			PARAM_TEXT_ADD(head, param->key, param->offer_tx,
				      text_out, text_len_out, textsize, 0, PPS_ERROR);
			ret++;
		}
	} else if (strcmp(param_in->key, "CHAP_I") == 0) {

		idData = driver_atoi((param_in->rx_offer) ? param_in->offer_rx : param_in->answer_rx);
		ret++;

	} else if (strcmp(param_in->key, "CHAP_C") == 0) {

		HexTextToData((param_in->rx_offer) ? param_in->offer_rx : param_in->answer_rx, ISCSI_CHAP_STRING_LENGTH,
				      chapdata, ISCSI_CHAP_DATA_LENGTH);

		if ((param = param_get(head, "CHAP_N")) == NULL) {
			PPS_ERROR;
		}
		param->tx_offer = 1;	/* sending an offer */
		param->rx_offer = 0;	/* reset */

		if (cred->shared_secret == NULL && !find_credentials(cred, cred->user, "chap")) {
			iscsi_err(__FILE__, __LINE__, "Unknown user `%s'\n", param_in->offer_rx);
			PPS_ERROR;
		}

		if (cred->user) {
			(void) strlcpy(param->offer_tx, cred->user, sizeof(param->offer_tx));
		} else {
			iscsi_err(__FILE__, __LINE__, "no valid user credentials\n");
			PPS_ERROR;
		}

		PARAM_TEXT_ADD(head, param->key, param->offer_tx,
				    text_out, text_len_out, textsize, 0, PPS_ERROR);

		if ((param = param_get(head, "CHAP_R")) == NULL) {
			PPS_ERROR;
		}
		param->tx_offer = 1;	/* sending an offer */
		param->rx_offer = 0;	/* reset */
		iSCSI_MD5Init(context);
		iSCSI_MD5Update(context, &idData, 1);

		if (cred->shared_secret == NULL) {
			iscsi_err(__FILE__, __LINE__, "null shared secret\n");
			PPS_ERROR;
		} else {
			iSCSI_MD5Update(context, (const uint8_t *)cred->shared_secret, strlen(cred->shared_secret));
		}

		HexDataToText(chapdata, ISCSI_CHAP_DATA_LENGTH,
			      param->offer_tx, ISCSI_CHAP_STRING_LENGTH);
		iSCSI_MD5Update(context, chapdata, ISCSI_CHAP_DATA_LENGTH);
		iSCSI_MD5Final(chapdata, context);
		HexDataToText(chapdata, ISCSI_CHAP_DATA_LENGTH,
			      param->offer_tx, ISCSI_CHAP_STRING_LENGTH);

		PARAM_TEXT_ADD(head, param->key, param->offer_tx,
				    text_out, text_len_out, textsize, 0, PPS_ERROR);

		if (param_in->rx_offer) {

			if ((param = param_get(head, "CHAP_I")) == NULL) {
				PPS_ERROR;
			}
			param->tx_offer = 1;	/* sending an offer */
			param->rx_offer = 0;	/* reset */
			GenRandomData(&idData, 1);
			(void) snprintf(chapstring, ISCSI_CHAP_STRING_LENGTH, "%d", idData);
			(void) strlcpy(param->offer_tx, chapstring, sizeof(param->offer_tx));
			PARAM_TEXT_ADD(head, param->key, param->offer_tx,
				      text_out, text_len_out, textsize, 0, PPS_ERROR);

			if ((param = param_get(head, "CHAP_C")) == NULL) {
				PPS_ERROR;
			}
			param->tx_offer = 1;	/* sending an offer */
			param->rx_offer = 0;	/* reset */
			GenRandomData(chapdata, ISCSI_CHAP_DATA_LENGTH);
			HexDataToText(chapdata, ISCSI_CHAP_DATA_LENGTH,
				      chapstring, ISCSI_CHAP_STRING_LENGTH);
			(void) strlcpy(param->offer_tx, chapstring, sizeof(param->offer_tx));
			PARAM_TEXT_ADD(head, param->key, param->offer_tx,
				      text_out, text_len_out, textsize, 0, PPS_ERROR);
		}
		ret++;

	} else if (strcmp(param_in->key, "CHAP_N") == 0) {
		char	*user;

		user = (param_in->rx_offer) ? param_in->offer_rx : param_in->answer_rx;
		if (!find_credentials(cred, user, "chap")) {
			iscsi_err(__FILE__, __LINE__, "Unknown user `%s'\n", user);
			PPS_ERROR;
		}
		ret++;

	} else if (strcmp(param_in->key, "CHAP_R") == 0) {

		iSCSI_MD5Init(context);

		iSCSI_MD5Update(context, &idData, 1);

		HexDataToText(&idData, 1, param_in->offer_tx, ISCSI_CHAP_STRING_LENGTH);
		HexDataToText(chapdata, ISCSI_CHAP_DATA_LENGTH,
			      chapstring, ISCSI_CHAP_STRING_LENGTH);

		if (cred->shared_secret == NULL) {
			iscsi_err(__FILE__, __LINE__, "Null shared secret in initiator\n");
			PPS_ERROR;
		} else {
			iSCSI_MD5Update(context, (const uint8_t *)cred->shared_secret, strlen(cred->shared_secret));
		}

		iSCSI_MD5Update(context, chapdata, ISCSI_CHAP_DATA_LENGTH);
		iSCSI_MD5Final(chapdata, context);

		HexTextToData((param_in->rx_offer) ? param_in->offer_rx : param_in->answer_rx, ISCSI_CHAP_STRING_LENGTH,
				      respdata, ISCSI_CHAP_DATA_LENGTH);

		HexDataToText(chapdata, ISCSI_CHAP_DATA_LENGTH,
			      param_in->offer_rx, ISCSI_CHAP_STRING_LENGTH);

		if (memcmp(respdata, chapdata, ISCSI_CHAP_DATA_LENGTH) != 0) {
			iscsi_err(__FILE__, __LINE__, "Initiator authentication failed %x %x\n", *chapdata, *respdata);
			PPS_ERROR;
		} else {
			PPS_CLEANUP;
		}
		return 0;
	}
	PPS_CLEANUP;
	return (ret);
}

int 
param_text_parse(iscsi_parameter_t * head,
		 iscsi_cred_t *cred,
		 char *text_in, int text_len_in,
		 char *text_out, int *text_len_out,
		 int textsize, 
		 int outgoing)
{
	static char    *key = NULL;
	char           *value = NULL;
	char           *ptr, *eq;
	iscsi_parameter_t *param;
	iscsi_parameter_value_t *item_ptr;
	int             offer_i, answer_i, max_i, val1_i, val2_i, negotiated_i;
	char           *p1, *p2, *p3, *p4;
	char           *offer = NULL;
	char           *valid = NULL;
	char           *val1 = NULL;
	char           *val2 = NULL;
	char           *tmp_key = NULL;
	char            c;
	int             ret;

	/*
	 * Whether incoming or outgoing, some of the params might be offers
	 * and some answers.  Incoming
	 */
	/*
	 * text has the potential for creating outgoing text - and this will
	 * happen when the incoming
	 */
	/* text has offers that need an answer. */

	iscsi_trace(TRACE_ISCSI_PARAM, "parsing %d %s bytes of text parameters\n", text_len_in, outgoing ? "outgoing" : "incoming");

	if ((key = iscsi_malloc(ISCSI_PARAM_KEY_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	if ((offer = iscsi_malloc(ISCSI_PARAM_MAX_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (key != NULL) {
			iscsi_free(key);
		}
		return -1;
	}
	if ((valid = iscsi_malloc(ISCSI_PARAM_MAX_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (key != NULL) {
			iscsi_free(key);
		}
		if (offer != NULL) {
			iscsi_free(offer);
		}
		return -1;
	}
	if ((val1 = iscsi_malloc(ISCSI_PARAM_MAX_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (key != NULL) {
			iscsi_free(key);
		}
		if (offer != NULL) {
			iscsi_free(offer);
		}
		if (valid != NULL) {
			iscsi_free(valid);
		}
		return -1;
	}
	if ((val2 = iscsi_malloc(ISCSI_PARAM_MAX_LEN)) == NULL) {
		iscsi_err(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		if (key != NULL) {
			iscsi_free(key);
		}
		if (offer != NULL) {
			iscsi_free(offer);
		}
		if (valid != NULL) {
			iscsi_free(valid);
		}
		if (val1 != NULL) {
			iscsi_free(val1);
		}
		return -1;
	}
#define PTP_CLEANUP { if (key != NULL) iscsi_free(key);    \
    if (offer != NULL) iscsi_free(offer);                    \
    if (valid != NULL) iscsi_free(valid);                    \
    if (val1 != NULL) iscsi_free(val1);                      \
    if (val2 != NULL) iscsi_free(val2);						 \
    if (tmp_key != NULL) iscsi_free(tmp_key); }
#define PTP_ERROR {PTP_CLEANUP; return -1;}

	if (!outgoing) {
		*text_len_out = 0;
	}

#if ISCSI_DEBUG
	printf("**************************************************\n");
	printf("*              PARAMETERS NEGOTIATED             *\n");
	printf("*                                                *\n");
#endif

	for (ptr = text_in; ptr - text_in < text_len_in; ptr += (strlen(ptr) + 1)) {

		/* Skip over any NULLs */

		while (!(*ptr) && ((ptr - text_in) < text_len_in)) {
			ptr++;
		}
		if ((ptr - text_in) >= text_len_in) {
			break;
		}

		/* Extract <key>=<value> token from text_in */

		if ((eq = strchr(ptr, '=')) == NULL) {
			iscsi_err(__FILE__, __LINE__, "delimiter \'=\' not found in token \"%s\"\n", ptr);
		} else {
			if ((int)(eq - ptr) >= (ISCSI_PARAM_KEY_LEN - 1)) {
				if (!outgoing) {
					tmp_key = iscsi_malloc((unsigned)(eq - ptr));
					if (tmp_key) {
						strncpy(tmp_key, ptr, (unsigned)(eq - ptr));
						tmp_key[(int)(eq - ptr)] = 0x0;
						/* Key not understood. */
						PARAM_TEXT_ADD(head, tmp_key, "NotUnderstood", text_out, text_len_out, textsize, 0, PTP_ERROR);
					}
				} else {
					printf("ignoring \"%s\"\n", key);
				}
				goto next;
			}
			strncpy(key, ptr, (unsigned)(eq - ptr));
			key[(int)(eq - ptr)] = 0x0;
			value = eq + 1;
		}

		/* Find key in param list */

		for (param = head; param != NULL; param = param->next) {
			if (strcmp(param->key, key) == 0) {
				break;
			}
		}
		if (param == NULL) {
			if (!outgoing) {
				/* Key not understood. */
				PARAM_TEXT_ADD(head, key, "NotUnderstood", text_out, text_len_out, textsize, 0, PTP_ERROR);
			} else {
				printf("ignoring \"%s\"\n", key);
			}
			goto next;
		}
		if (strlen(value) > ISCSI_PARAM_MAX_LEN) {
			iscsi_err(__FILE__, __LINE__,
				"strlen(value) %u\n", strlen(value));
			PTP_CLEANUP;
			return -1;
		}

		/* We're sending|receiving an offer|answer  */
		if (outgoing) {
			if (param->rx_offer) {
				param->tx_answer = 1;	/* sending an answer */
				param->rx_answer = 0; /* reset */
				param->tx_offer = 0; /* reset */
				param->rx_offer = 0; /* reset */
				(void) strlcpy(param->answer_tx, value, sizeof(param->answer_tx));
				iscsi_trace(TRACE_ISCSI_PARAM, "sending answer \"%s\"=\"%s\" for offer \"%s\"\n",
				      param->key, param->answer_tx, param->offer_rx);
				goto negotiate;
			} else {
				param->tx_offer = 1;	/* sending an offer */
				param->tx_answer = 0;
				param->rx_answer = 0;
				(void) strlcpy(param->offer_tx, value, sizeof(param->offer_tx));
				iscsi_trace(TRACE_ISCSI_PARAM, "sending offer \"%s\"=\"%s\"\n", param->key, param->offer_tx);
				if ((param->type == ISCSI_PARAM_TYPE_DECLARATIVE) ||
				    (param->type == ISCSI_PARAM_TYPE_DECLARE_MULTI)) {
					goto negotiate;
				}
				goto next;
			}
		} else {
			if (param->tx_offer) {
				param->rx_answer = 1;	/* received an answer */
				param->tx_answer = 0;
				param->rx_offer = 0;
				param->tx_offer = 0;	/* reset */
				(void) strlcpy(param->answer_rx, value, sizeof(param->answer_rx));
				iscsi_trace(TRACE_ISCSI_PARAM, "received answer \"%s\"=\"%s\" for offer \"%s\"\n",
				      param->key, param->answer_rx, param->offer_tx);

				if ((ret = param_parse_security(head, param, cred,
					       text_out, text_len_out, textsize)) > 1) {
					goto next;
				} else if (ret == 0) {
					/*
					 * FIX ME Happens in initiator code
					 * currently we ignore initiator
					 * authentication status See comments
					 * at the beginning of parse_security
					 */
					goto negotiate;
				} else if (ret == 1) {
					goto negotiate;
				} else {
					PTP_CLEANUP;
				}
				return ISCSI_PARAM_STATUS_AUTH_FAILED;
			} else {
				param->rx_offer = 1;	/* received an offer */
				param->rx_answer = 0;
				param->tx_answer = 0; 
				(void) strlcpy(param->offer_rx, value, sizeof(param->offer_rx));
				iscsi_trace(TRACE_ISCSI_PARAM, "received offer \"%s\"=\"%s\"\n", param->key, param->offer_rx);

				if ((ret = param_parse_security(head, param, cred,
					       text_out, text_len_out, textsize)) > 1) {
					goto next;
				} else if (ret < 0) {
					iscsi_parameter_t *auth_result;
					if ((auth_result = param_get(head, "AuthResult")) != 0) {
						(void) strlcpy(auth_result->value_l->value, "Fail", sizeof(auth_result->value_l->value));
					}
					PTP_CLEANUP;
					return (ISCSI_PARAM_STATUS_AUTH_FAILED);
				} else if (ret == 0) {
					iscsi_parameter_t *auth_result;
					if ((auth_result = param_get(head, "AuthResult")) != 0) {
						(void) strlcpy(auth_result->value_l->value, "Yes", sizeof(auth_result->value_l->value));
					}
				}
				/*
				 * Answer the offer if it is an inquiry or
				 * the type is not DECLARATIVE
				 */

				if ((strcmp(param->offer_rx, "?") != 0) && ((param->type == ISCSI_PARAM_TYPE_DECLARATIVE) || (param->type == ISCSI_PARAM_TYPE_DECLARE_MULTI))) {
					goto negotiate;
				} else {
					goto answer;
				}
			}
		}

answer:

		/* Answer with current value if this is an inquiry (<key>=?) */

		if (strcmp(value, "?") == 0) {
			iscsi_trace(TRACE_ISCSI_PARAM, "got inquiry for param \"%s\"\n", param->key);
			if (param->value_l) {
				if (param->value_l->value) {
					(void) strlcpy(param->answer_tx, param->value_l->value, sizeof(param->answer_tx));
				} else {
					iscsi_err(__FILE__, __LINE__, "param \"%s\" has NULL value_l->value\n", param->key);
					param->answer_tx[0] = 0x0;
				}
			} else {
				iscsi_err(__FILE__, __LINE__, "param \"%s\" has NULL value_l\n", param->key);
				param->answer_tx[0] = 0x0;
			}
			goto add_answer;
		}
		/* Generate answer according to the parameter type */

		switch (param->type) {

		case ISCSI_PARAM_TYPE_BINARY_AND:
			goto binary_or;

		case ISCSI_PARAM_TYPE_BINARY_OR:
binary_or:
			if (strcmp(value, "yes") != 0 &&
			    strcmp(value, "no") != 0 &&
			    strcmp(value, "Yes") != 0 &&
			    strcmp(value, "No") != 0) {
				iscsi_err(__FILE__, __LINE__, "\"%s\" is not a valid binary value\n", value);
				(void) strlcpy(param->answer_tx, "Reject", sizeof(param->answer_tx));
				goto add_answer;
			}
			if (strchr(param->valid, ',') != NULL) {
				(void) strlcpy(param->answer_tx, value, sizeof(param->answer_tx));	/* we accept both yes
									 * and no, so answer w/
									 * their offer */
			} else {
				(void) strlcpy(param->answer_tx, param->valid, sizeof(param->answer_tx));	/* answer with the only
									 * value we support */
			}
			break;

		case ISCSI_PARAM_TYPE_LIST:

			/*
			 * Use our default value if it's offered as one of the option
			 * in the parameter list.
			 *
			 * We need to do this at least for CHAP because cisco's initiator
			 * could be sending us a parameter value list with "CHAP,None",
			 * even when it doesn't set username/password in its configration
			 * file, in which case we should pick "None" as for no security instead
			 * of pick the first one on the value list. "None" is the default value
			 * for AuthMethod
			 *
			 * This fix is working well now, though is arguable. We should keep
			 * this just to make us work with Cisco for now.
			 */
			if (strlen(param->dflt)) {
				for (p1 = p2 = param->offer_rx; p2; p1 = p2 + 1) {

					if ((p2 = strchr(p1, ',')) != NULL) {
						strncpy(offer, p1, (unsigned)(p2 - p1));
						offer[(int)(p2 - p1)] = 0x0;
					} else {
						(void) strlcpy(offer, p1, ISCSI_PARAM_MAX_LEN);
					}

					if (strcmp(param->dflt, offer) == 0) {
						(void) strlcpy(param->answer_tx, offer, sizeof(param->answer_tx));
						goto add_answer;
					}
				}
			}
			/* Find the first valid offer that we support */

			for (p1 = p2 = param->offer_rx; p2; p1 = p2 + 1) {
				if ((p2 = strchr(p1, ',')) != NULL) {
					strncpy(offer, p1, (unsigned)(p2 - p1));
					offer[p2 - p1] = 0x0;
				} else {
					(void) strlcpy(offer, p1, ISCSI_PARAM_MAX_LEN);
				}
				if (strlen(param->valid)) {
					for (p3 = p4 = param->valid; p4; p3 = p4 + 1) {
						if ((p4 = strchr(p3, ',')) != NULL) {
							strncpy(valid, p3, (unsigned)(p4 - p3));
							valid[(int)(p4 - p3)] = 0x0;
						} else {
							(void) strlcpy(valid, p3, ISCSI_PARAM_MAX_LEN);
						}
						if (strcmp(valid, offer) == 0) {
							(void) strlcpy(param->answer_tx, offer, sizeof(param->answer_tx));
							goto add_answer;
						}
					}
				} else {
					iscsi_trace(TRACE_ISCSI_PARAM, "Valid list empty. Answering with first in offer list\n");
					(void) strlcpy(param->answer_tx, offer, sizeof(param->answer_tx));
					goto add_answer;
				}
				iscsi_trace(TRACE_ISCSI_PARAM, "\"%s\" is not a valid offer for key \"%s\" (must choose from \"%s\")\n", offer, param->key, param->valid);
			}
			iscsi_trace(TRACE_ISCSI_PARAM, "No Valid offers: \"%s\" is added as value for key \"%s\")\n", "Reject", param->key);
			(void) strlcpy(param->answer_tx, "Reject", sizeof(param->answer_tx));
			break;

		case ISCSI_PARAM_TYPE_NUMERICAL_Z:
			goto numerical;

		case ISCSI_PARAM_TYPE_NUMERICAL:
numerical:
			offer_i = iscsi_atoi(param->offer_rx);
			max_i = iscsi_atoi(param->valid);
			if (param->type == ISCSI_PARAM_TYPE_NUMERICAL_Z) {
				if (max_i == 0) {
					answer_i = offer_i;	/* we support anything,
								 * so return whatever
								 * they offered */
				} else if (offer_i == 0) {
					answer_i = max_i;	/* return only what we
								 * can support */
				} else if (offer_i > max_i) {
					answer_i = max_i;	/* we are the lower of
								 * the two */
				} else {
					answer_i = offer_i;	/* they are the lower of
								 * the two */
				}
			} else {
				if (offer_i > max_i) {
					answer_i = max_i;	/* we are the lower of
								 * the two */
				} else {
					answer_i = offer_i;	/* they are the lower of
								 * the two */
				}
			}
			(void) snprintf(param->answer_tx, sizeof(param->answer_tx), "%d", answer_i);
			goto add_answer;

		default:
			goto next;
		}
add_answer:	PARAM_TEXT_ADD(head, key, param->answer_tx, text_out, text_len_out, textsize, 0, PTP_ERROR);
		iscsi_trace(TRACE_ISCSI_PARAM, "answering \"%s\"=\"%s\"\n", param->key, param->answer_tx);
		goto next;


		/* Negotiate after receiving|sending an answer  */

negotiate:
		switch (param->type) {
		case ISCSI_PARAM_TYPE_DECLARE_MULTI:
			goto declarative_negotiate;
		case ISCSI_PARAM_TYPE_DECLARATIVE:
declarative_negotiate:
			if (param->tx_answer) {
				(void) strlcpy(param->negotiated, param->answer_tx, sizeof(param->negotiated));
			} else if (param->tx_offer) {
				(void) strlcpy(param->negotiated, param->offer_tx, sizeof(param->negotiated));
			} else if (param->rx_answer) {
				(void) strlcpy(param->negotiated, param->answer_rx, sizeof(param->negotiated));
			} else if (param->rx_offer) {
				(void) strlcpy(param->negotiated, param->offer_rx, sizeof(param->negotiated));
			} else {
				iscsi_err(__FILE__, __LINE__, "Invalid negotiation!?!?\n");	
			}
			break;
		case ISCSI_PARAM_TYPE_BINARY_AND:
			goto binary_or_negotiate;
		case ISCSI_PARAM_TYPE_BINARY_OR:
binary_or_negotiate:
			if (outgoing) {
				(void) strlcpy(val1, param->offer_rx, ISCSI_PARAM_MAX_LEN);
				(void) strlcpy(val2, param->answer_tx, ISCSI_PARAM_MAX_LEN);
			} else {
				(void) strlcpy(val1, param->answer_rx, ISCSI_PARAM_MAX_LEN);
				(void) strlcpy(val2, param->offer_tx, ISCSI_PARAM_MAX_LEN);
				/* Make sure the answer is valid */
				if (strcmp(val1, "Yes") != 0 &&
				    strcmp(val1, "No") != 0 &&
				    strcmp(val1, "yes") != 0 &&
				    strcmp(val1, "no") != 0 &&
				    strcmp(val1, "Irrelevant") != 0) {
					/* Invalid value returned as answer. */
					iscsi_err(__FILE__, __LINE__, "Invalid answer (%s) for key (%s)\n",
						    val1, key);
					PTP_ERROR;
				}
			}
			if (param->type == ISCSI_PARAM_TYPE_BINARY_OR) {
				if (strcmp(val1, "yes") == 0 || strcmp(val2, "yes") == 0 || strcmp(val1, "Yes") == 0 || strcmp(val2, "Yes") == 0) {
					(void) strlcpy(param->negotiated, "Yes", sizeof(param->negotiated));
				} else {
					(void) strlcpy(param->negotiated, "No", sizeof(param->negotiated));
				}
			} else {
				if ((strcmp(val1, "yes") == 0 && strcmp(val2, "yes") == 0) || (strcmp(val1, "Yes") == 0 && strcmp(val2, "Yes") == 0)) {
					(void) strlcpy(param->negotiated, "Yes", sizeof(param->negotiated));
				} else {
					(void) strlcpy(param->negotiated, "No", sizeof(param->negotiated));
				}
			}
			break;
		case ISCSI_PARAM_TYPE_NUMERICAL_Z:
			goto numerical_negotiate;
		case ISCSI_PARAM_TYPE_NUMERICAL:
numerical_negotiate:
			if (outgoing) {
				(void) strlcpy(val1, param->offer_rx, ISCSI_PARAM_MAX_LEN);
				(void) strlcpy(val2, param->answer_tx, ISCSI_PARAM_MAX_LEN);
			} else {
				(void) strlcpy(val1, param->answer_rx, ISCSI_PARAM_MAX_LEN);
				(void) strlcpy(val2, param->offer_tx, ISCSI_PARAM_MAX_LEN);
			}
			val1_i = iscsi_atoi(val1);
			val2_i = iscsi_atoi(val2);
			if (param->type == ISCSI_PARAM_TYPE_NUMERICAL_Z) {
				if (val1_i == 0) {
					negotiated_i = val2_i;
				} else if (val2_i == 0) {
					negotiated_i = val1_i;
				} else if (val1_i > val2_i) {
					negotiated_i = val2_i;
				} else {
					negotiated_i = val1_i;
				}
			} else {
				if (val1_i > val2_i) {
					negotiated_i = val2_i;
				} else {
					negotiated_i = val1_i;
				}
			}
			(void) snprintf(param->negotiated, sizeof(param->negotiated), "%d", negotiated_i);
			break;
		case ISCSI_PARAM_TYPE_LIST:
			if (outgoing) {
				if (param->tx_offer) {
					iscsi_err(__FILE__, __LINE__, "we should not be here\n");	/* error - we're sending
										 * an offer */
					PTP_ERROR;
				} else if (param->tx_answer) {
					(void) strlcpy(val1, param->answer_tx, ISCSI_PARAM_MAX_LEN);	/* we're sending an
									 * answer */
				} else {
					iscsi_err(__FILE__, __LINE__, "unexpected error\n");
					PTP_ERROR;
				}
			} else {
				if (param->rx_offer) {
					iscsi_err(__FILE__, __LINE__, "we should not be here\n");	/* error - we received
										 * an offer */
					PTP_ERROR;
				} else if (param->rx_answer) {
					(void) strlcpy(val1, param->answer_rx, ISCSI_PARAM_MAX_LEN);	/* we received an answer */
				} else {
					iscsi_err(__FILE__, __LINE__, "unexpected error\n");
					PTP_ERROR;
				}
			}

			/* Make sure incoming or outgoing answer is valid */
			/*
			 * None, Reject, Irrelevant and NotUnderstood are
			 * valid
			 */
			if ((strcmp(val1, "None") == 0) || (strcmp(val1, "Reject") == 0) ||
			    (strcmp(val1, "Irrelevant") == 0) || (strcmp(val1, "NotUnderstood") == 0)) {
				goto value_ok;
			}
			if (strlen(param->valid) > 0) {
				for (p3 = p4 = param->valid; p4; p3 = p4 + 1) {
					if ((p4 = strchr(p3, ',')) != NULL) {
						strncpy(valid, p3, (unsigned)(p4 - p3));
						valid[(int)(p4 - p3)] = 0x0;
					} else {
						(void) strlcpy(valid, p3, ISCSI_PARAM_MAX_LEN);
					}
					if (strcmp(valid, val1) == 0) {
						goto value_ok;
					}
				}
			} else {
				iscsi_trace(TRACE_ISCSI_PARAM, "Valid list empty??\n");
				PTP_ERROR;
			}
			iscsi_err(__FILE__, __LINE__, "\"%s\" is not a valid value (must choose from \"%s\")\n", val1, param->valid);
			PTP_ERROR;
value_ok:
			(void) strlcpy(param->negotiated, val1, sizeof(param->negotiated));
			break;
		}

		iscsi_trace(TRACE_ISCSI_PARAM, "negotiated \"%s\"=\"%s\"\n", param->key, param->negotiated);

		/* For inquiries, we don't commit the value. */

		if (param->tx_offer && strcmp(param->offer_tx, "?") == 0) {
			/* we're offering an inquiry  */
			iscsi_trace(TRACE_ISCSI_PARAM, "sending an inquiry for \"%s\"\n", param->key);
			goto next;
		} else if (param->rx_offer && strcmp(param->offer_rx, "?") == 0) {
			/* we're receiving an inquiry  */
			iscsi_trace(TRACE_ISCSI_PARAM, "received an inquiry for \"%s\"\n", param->key);
			goto next;
		} else if (param->tx_answer && strcmp(param->offer_rx, "?") == 0) {
			/* we're answering an inquiry  */
			iscsi_trace(TRACE_ISCSI_PARAM, "answering an inquiry for \"%s\"\n", param->key);
			goto next;
		} else if (param->rx_answer && strcmp(param->offer_tx, "?") == 0) {
			/* we're receiving an answer for our inquiry  */
			iscsi_trace(TRACE_ISCSI_PARAM, "received an answer for inquiry on \"%s\"\n", param->key);
			goto next;
		}
		iscsi_trace(TRACE_ISCSI_PARAM, "automatically committing \"%s\"=\"%s\"\n", param->key, param->negotiated);

		c = param->negotiated[19];
		param->negotiated[19] = 0x0;
#if ISCSI_DEBUG
		printf("* %25s:%20s *\n", param->key, param->negotiated);
#endif
		param->negotiated[19] = c;

		if (param->reset) {
			iscsi_trace(TRACE_ISCSI_PARAM, "deleting value list for \"%s\"\n", param->key);
			if (param_val_delete_all(head, param->key) != 0) {
				iscsi_err(__FILE__, __LINE__, "param_val_delete_all() failed\n");
				PTP_ERROR;
			}
			param->reset = 0;
		}
		if (param->value_l) {
			if (param->type == ISCSI_PARAM_TYPE_DECLARE_MULTI) {
				for (item_ptr = param->value_l; item_ptr->next != NULL; item_ptr = item_ptr->next) {
				}
				if ((item_ptr->next = iscsi_malloc_atomic(sizeof(iscsi_parameter_value_t))) == NULL) {
					iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
					PTP_ERROR;
				}
				item_ptr = item_ptr->next;
				item_ptr->next = NULL;
			} else {
				item_ptr = param->value_l;
			}
		} else {
			iscsi_trace(TRACE_ISCSI_PARAM, "allocating value ptr\n");
			if ((param->value_l = iscsi_malloc_atomic(sizeof(iscsi_parameter_value_t))) == NULL) {
				iscsi_err(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
				PTP_ERROR;
			}
			item_ptr = param->value_l;
			item_ptr->next = NULL;
		}
		(void) strlcpy(item_ptr->value, param->negotiated, sizeof(item_ptr->value));
next:
		continue;
	}
	if (!outgoing) {
		iscsi_trace(TRACE_ISCSI_PARAM, "generated %d bytes response\n", *text_len_out);
	}
#if ISCSI_DEBUG
	printf("**************************************************\n");
#endif

	PTP_CLEANUP;
	return 0;
}

void 
set_session_parameters(iscsi_parameter_t * head,
		       iscsi_sess_param_t * sess_params)
{
	/* These parameters are standard and assuming that they are always */
	/* present in the list (head). */
	memset(sess_params, 0, sizeof(iscsi_sess_param_t));
	sess_params->max_burst_length = param_atoi(head, "MaxBurstLength");
	sess_params->first_burst_length = param_atoi(head, "FirstBurstLength");
	sess_params->max_dataseg_len =
		param_atoi(head, "MaxRecvDataSegmentLength");
	sess_params->header_digest = (param_equiv(head, "HeaderDigest", "Yes")) ? 1 : 0;
	sess_params->data_digest = (param_equiv(head, "DataDigest", "Yes")) ? 1 : 0;
	sess_params->initial_r2t = (param_equiv(head, "InitialR2T", "Yes"));
	sess_params->immediate_data = (param_equiv(head, "ImmediateData", "Yes"));
}
