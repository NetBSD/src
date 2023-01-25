/*	$NetBSD: pk11.c,v 1.8 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isc/log.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/stdio.h>
#include <isc/strerr.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <pk11/internal.h>
#include <pk11/pk11.h>
#include <pk11/result.h>
#include <pk11/site.h>
#include <pkcs11/pkcs11.h>

#include <dst/result.h>

/* was 32 octets, Petr Spacek suggested 1024, SoftHSMv2 uses 256... */
#ifndef PINLEN
#define PINLEN 256
#endif /* ifndef PINLEN */

#ifndef PK11_NO_LOGERR
#define PK11_NO_LOGERR 1
#endif /* ifndef PK11_NO_LOGERR */

LIBISC_EXTERNAL_DATA bool pk11_verbose_init = false;

static isc_once_t once = ISC_ONCE_INIT;
static isc_mem_t *pk11_mctx = NULL;
static int32_t allocsize = 0;
static bool initialized = false;

typedef struct pk11_session pk11_session_t;
typedef struct pk11_token pk11_token_t;
typedef ISC_LIST(pk11_session_t) pk11_sessionlist_t;

struct pk11_session {
	unsigned int magic;
	CK_SESSION_HANDLE session;
	ISC_LINK(pk11_session_t) link;
	pk11_token_t *token;
};

struct pk11_token {
	unsigned int magic;
	unsigned int operations;
	ISC_LINK(pk11_token_t) link;
	CK_SLOT_ID slotid;
	pk11_sessionlist_t sessions;
	bool logged;
	char name[32];
	char manuf[32];
	char model[16];
	char serial[16];
	char pin[PINLEN + 1];
};
static ISC_LIST(pk11_token_t) tokens;

static pk11_token_t *best_rsa_token;
static pk11_token_t *best_ecdsa_token;
static pk11_token_t *best_eddsa_token;

static isc_result_t
free_all_sessions(void);
static isc_result_t
free_session_list(pk11_sessionlist_t *slist);
static isc_result_t
setup_session(pk11_session_t *sp, pk11_token_t *token, bool rw);
static void
scan_slots(void);
static isc_result_t
token_login(pk11_session_t *sp);
static char *
percent_decode(char *x, size_t *len);
static bool
pk11strcmp(const char *x, size_t lenx, const char *y, size_t leny);
static CK_ATTRIBUTE *
push_attribute(pk11_object_t *obj, isc_mem_t *mctx, size_t len);

static isc_mutex_t alloclock;
static isc_mutex_t sessionlock;

static pk11_sessionlist_t actives;

static CK_C_INITIALIZE_ARGS pk11_init_args = {
	NULL_PTR,	   /* CreateMutex */
	NULL_PTR,	   /* DestroyMutex */
	NULL_PTR,	   /* LockMutex */
	NULL_PTR,	   /* UnlockMutex */
	CKF_OS_LOCKING_OK, /* flags */
	NULL_PTR,	   /* pReserved */
};

#ifndef PK11_LIB_LOCATION
#define PK11_LIB_LOCATION "unknown_provider"
#endif /* ifndef PK11_LIB_LOCATION */

#ifndef WIN32
static const char *lib_name = PK11_LIB_LOCATION;
#else  /* ifndef WIN32 */
static const char *lib_name = PK11_LIB_LOCATION ".dll";
#endif /* ifndef WIN32 */

void
pk11_set_lib_name(const char *name) {
	lib_name = name;
}

const char *
pk11_get_lib_name(void) {
	return (lib_name);
}

static void
initialize(void) {
	char *pk11_provider;

	isc_mutex_init(&alloclock);
	isc_mutex_init(&sessionlock);

	pk11_provider = getenv("PKCS11_PROVIDER");
	if (pk11_provider != NULL) {
		lib_name = pk11_provider;
	}
}

void *
pk11_mem_get(size_t size) {
	void *ptr;

	LOCK(&alloclock);
	if (pk11_mctx != NULL) {
		ptr = isc_mem_get(pk11_mctx, size);
	} else {
		ptr = malloc(size);
		if (ptr == NULL && size != 0) {
			char strbuf[ISC_STRERRORSIZE];
			strerror_r(errno, strbuf, sizeof(strbuf));
			isc_error_fatal(__FILE__, __LINE__, "malloc failed: %s",
					strbuf);
		}
	}
	UNLOCK(&alloclock);

	if (ptr != NULL) {
		memset(ptr, 0, size);
	}
	return (ptr);
}

void
pk11_mem_put(void *ptr, size_t size) {
	if (ptr != NULL) {
		memset(ptr, 0, size);
	}
	LOCK(&alloclock);
	if (pk11_mctx != NULL) {
		isc_mem_put(pk11_mctx, ptr, size);
	} else {
		if (ptr != NULL) {
			allocsize -= (int)size;
		}
		free(ptr);
	}
	UNLOCK(&alloclock);
}

isc_result_t
pk11_initialize(isc_mem_t *mctx, const char *engine) {
	isc_result_t result = ISC_R_SUCCESS;
	CK_RV rv;

	RUNTIME_CHECK(isc_once_do(&once, initialize) == ISC_R_SUCCESS);

	LOCK(&sessionlock);
	LOCK(&alloclock);
	if ((mctx != NULL) && (pk11_mctx == NULL) && (allocsize == 0)) {
		isc_mem_attach(mctx, &pk11_mctx);
	}
	UNLOCK(&alloclock);
	if (initialized) {
		goto unlock;
	} else {
		initialized = true;
	}

	ISC_LIST_INIT(tokens);
	ISC_LIST_INIT(actives);

	if (engine != NULL) {
		lib_name = engine;
	}

	/* Initialize the CRYPTOKI library */
	rv = pkcs_C_Initialize((CK_VOID_PTR)&pk11_init_args);

	if (rv == 0xfe) {
		result = PK11_R_NOPROVIDER;
		fprintf(stderr, "Can't load PKCS#11 provider: %s\n",
			pk11_get_load_error_message());
		goto unlock;
	}
	if (rv != CKR_OK) {
		result = PK11_R_INITFAILED;
		goto unlock;
	}

	scan_slots();
unlock:
	UNLOCK(&sessionlock);
	return (result);
}

isc_result_t
pk11_finalize(void) {
	pk11_token_t *token, *next;
	isc_result_t ret;

	ret = free_all_sessions();
	(void)pkcs_C_Finalize(NULL_PTR);
	token = ISC_LIST_HEAD(tokens);
	while (token != NULL) {
		next = ISC_LIST_NEXT(token, link);
		ISC_LIST_UNLINK(tokens, token, link);
		if (token == best_rsa_token) {
			best_rsa_token = NULL;
		}
		if (token == best_ecdsa_token) {
			best_ecdsa_token = NULL;
		}
		if (token == best_eddsa_token) {
			best_eddsa_token = NULL;
		}
		pk11_mem_put(token, sizeof(*token));
		token = next;
	}
	if (pk11_mctx != NULL) {
		isc_mem_detach(&pk11_mctx);
	}
	initialized = false;
	return (ret);
}

isc_result_t
pk11_get_session(pk11_context_t *ctx, pk11_optype_t optype, bool need_services,
		 bool rw, bool logon, const char *pin, CK_SLOT_ID slot) {
	pk11_token_t *token = NULL;
	pk11_sessionlist_t *freelist;
	pk11_session_t *sp;
	isc_result_t ret;
	UNUSED(need_services);

	memset(ctx, 0, sizeof(pk11_context_t));
	ctx->handle = NULL;
	ctx->session = CK_INVALID_HANDLE;

	ret = pk11_initialize(NULL, NULL);
	if (ret != ISC_R_SUCCESS) {
		return (ret);
	}

	LOCK(&sessionlock);
	/* wait for initialization to finish */
	UNLOCK(&sessionlock);

	switch (optype) {
	case OP_ANY:
		for (token = ISC_LIST_HEAD(tokens); token != NULL;
		     token = ISC_LIST_NEXT(token, link))
		{
			if (token->slotid == slot) {
				break;
			}
		}
		break;
	default:
		for (token = ISC_LIST_HEAD(tokens); token != NULL;
		     token = ISC_LIST_NEXT(token, link))
		{
			if (token->slotid == slot) {
				break;
			}
		}
		break;
	}
	if (token == NULL) {
		return (ISC_R_NOTFOUND);
	}

	/* Override the token's PIN */
	if (logon && pin != NULL && *pin != '\0') {
		if (strlen(pin) > PINLEN) {
			return (ISC_R_RANGE);
		}
		/*
		 * We want to zero out the old pin before
		 * overwriting with a new one.
		 */
		memset(token->pin, 0, sizeof(token->pin));
		strlcpy(token->pin, pin, sizeof(token->pin));
	}

	freelist = &token->sessions;

	LOCK(&sessionlock);
	sp = ISC_LIST_HEAD(*freelist);
	if (sp != NULL) {
		ISC_LIST_UNLINK(*freelist, sp, link);
		ISC_LIST_APPEND(actives, sp, link);
		UNLOCK(&sessionlock);
		if (logon) {
			ret = token_login(sp);
		}
		ctx->handle = sp;
		ctx->session = sp->session;
		return (ret);
	}
	UNLOCK(&sessionlock);

	sp = pk11_mem_get(sizeof(*sp));
	sp->magic = SES_MAGIC;
	sp->token = token;
	sp->session = CK_INVALID_HANDLE;
	ISC_LINK_INIT(sp, link);
	ret = setup_session(sp, token, rw);
	if ((ret == ISC_R_SUCCESS) && logon) {
		ret = token_login(sp);
	}
	LOCK(&sessionlock);
	ISC_LIST_APPEND(actives, sp, link);
	UNLOCK(&sessionlock);
	ctx->handle = sp;
	ctx->session = sp->session;
	return (ret);
}

void
pk11_return_session(pk11_context_t *ctx) {
	pk11_session_t *sp = (pk11_session_t *)ctx->handle;

	if (sp == NULL) {
		return;
	}
	ctx->handle = NULL;
	ctx->session = CK_INVALID_HANDLE;

	LOCK(&sessionlock);
	ISC_LIST_UNLINK(actives, sp, link);
	UNLOCK(&sessionlock);
	if (sp->session == CK_INVALID_HANDLE) {
		pk11_mem_put(sp, sizeof(*sp));
		return;
	}

	LOCK(&sessionlock);
	ISC_LIST_APPEND(sp->token->sessions, sp, link);
	UNLOCK(&sessionlock);
}

static isc_result_t
free_all_sessions(void) {
	pk11_token_t *token;
	isc_result_t ret = ISC_R_SUCCESS;
	isc_result_t oret;

	for (token = ISC_LIST_HEAD(tokens); token != NULL;
	     token = ISC_LIST_NEXT(token, link))
	{
		oret = free_session_list(&token->sessions);
		if (oret != ISC_R_SUCCESS) {
			ret = oret;
		}
	}
	if (!ISC_LIST_EMPTY(actives)) {
		ret = ISC_R_ADDRINUSE;
		oret = free_session_list(&actives);
		if (oret != ISC_R_SUCCESS) {
			ret = oret;
		}
	}
	return (ret);
}

static isc_result_t
free_session_list(pk11_sessionlist_t *slist) {
	pk11_session_t *sp;
	CK_RV rv;
	isc_result_t ret;

	ret = ISC_R_SUCCESS;
	LOCK(&sessionlock);
	while (!ISC_LIST_EMPTY(*slist)) {
		sp = ISC_LIST_HEAD(*slist);
		ISC_LIST_UNLINK(*slist, sp, link);
		UNLOCK(&sessionlock);
		if (sp->session != CK_INVALID_HANDLE) {
			rv = pkcs_C_CloseSession(sp->session);
			if (rv != CKR_OK) {
				ret = DST_R_CRYPTOFAILURE;
			}
		}
		LOCK(&sessionlock);
		pk11_mem_put(sp, sizeof(*sp));
	}
	UNLOCK(&sessionlock);

	return (ret);
}

static isc_result_t
setup_session(pk11_session_t *sp, pk11_token_t *token, bool rw) {
	CK_RV rv;
	CK_FLAGS flags = CKF_SERIAL_SESSION;

	if (rw) {
		flags += CKF_RW_SESSION;
	}

	rv = pkcs_C_OpenSession(token->slotid, flags, NULL_PTR, NULL_PTR,
				&sp->session);
	if (rv != CKR_OK) {
		return (DST_R_CRYPTOFAILURE);
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
token_login(pk11_session_t *sp) {
	CK_RV rv;
	pk11_token_t *token = sp->token;
	isc_result_t ret = ISC_R_SUCCESS;

	LOCK(&sessionlock);
	if (!token->logged) {
		rv = pkcs_C_Login(sp->session, CKU_USER,
				  (CK_UTF8CHAR_PTR)token->pin,
				  (CK_ULONG)strlen(token->pin));
		if (rv != CKR_OK) {
#if PK11_NO_LOGERR
			pk11_error_fatalcheck(__FILE__, __LINE__,
					      "pkcs_C_Login", rv);
#else  /* if PK11_NO_LOGERR */
			ret = ISC_R_NOPERM;
#endif /* if PK11_NO_LOGERR */
		} else {
			token->logged = true;
		}
	}
	UNLOCK(&sessionlock);
	return (ret);
}

#define PK11_TRACE(fmt)        \
	if (pk11_verbose_init) \
	fprintf(stderr, fmt)
#define PK11_TRACE1(fmt, arg)  \
	if (pk11_verbose_init) \
	fprintf(stderr, fmt, arg)
#define PK11_TRACE2(fmt, arg1, arg2) \
	if (pk11_verbose_init)       \
	fprintf(stderr, fmt, arg1, arg2)
#define PK11_TRACEM(mech)      \
	if (pk11_verbose_init) \
	fprintf(stderr, #mech ": 0x%lx\n", rv)

static void
scan_slots(void) {
	CK_MECHANISM_INFO mechInfo;
	CK_TOKEN_INFO tokenInfo;
	CK_RV rv;
	CK_SLOT_ID slot;
	CK_SLOT_ID_PTR slotList;
	CK_ULONG slotCount;
	pk11_token_t *token;
	unsigned int i;
	bool bad;

	slotCount = 0;
	PK11_FATALCHECK(pkcs_C_GetSlotList, (CK_FALSE, NULL_PTR, &slotCount));
	PK11_TRACE1("slotCount=%lu\n", slotCount);
	/* it's not an error if we didn't find any providers */
	if (slotCount == 0) {
		return;
	}
	slotList = pk11_mem_get(sizeof(CK_SLOT_ID) * slotCount);
	PK11_FATALCHECK(pkcs_C_GetSlotList, (CK_FALSE, slotList, &slotCount));

	for (i = 0; i < slotCount; i++) {
		slot = slotList[i];
		PK11_TRACE2("slot#%u=0x%lx\n", i, slot);

		rv = pkcs_C_GetTokenInfo(slot, &tokenInfo);
		if (rv != CKR_OK) {
			continue;
		}
		token = pk11_mem_get(sizeof(*token));
		token->magic = TOK_MAGIC;
		token->slotid = slot;
		ISC_LINK_INIT(token, link);
		ISC_LIST_INIT(token->sessions);
		memmove(token->name, tokenInfo.label, 32);
		memmove(token->manuf, tokenInfo.manufacturerID, 32);
		memmove(token->model, tokenInfo.model, 16);
		memmove(token->serial, tokenInfo.serialNumber, 16);
		ISC_LIST_APPEND(tokens, token, link);

		/* Check for RSA support */
		bad = false;
		rv = pkcs_C_GetMechanismInfo(slot, CKM_RSA_PKCS_KEY_PAIR_GEN,
					     &mechInfo);
		if ((rv != CKR_OK) ||
		    ((mechInfo.flags & CKF_GENERATE_KEY_PAIR) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_RSA_PKCS_KEY_PAIR_GEN);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_MD5_RSA_PKCS, &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_MD5_RSA_PKCS);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_SHA1_RSA_PKCS,
					     &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_SHA1_RSA_PKCS);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_SHA256_RSA_PKCS,
					     &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_SHA256_RSA_PKCS);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_SHA512_RSA_PKCS,
					     &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_SHA512_RSA_PKCS);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_RSA_PKCS, &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_RSA_PKCS);
		}
		if (!bad) {
			token->operations |= 1 << OP_RSA;
			if (best_rsa_token == NULL) {
				best_rsa_token = token;
			}
		}

		/* Check for ECDSA support */
		bad = false;
		rv = pkcs_C_GetMechanismInfo(slot, CKM_EC_KEY_PAIR_GEN,
					     &mechInfo);
		if ((rv != CKR_OK) ||
		    ((mechInfo.flags & CKF_GENERATE_KEY_PAIR) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_EC_KEY_PAIR_GEN);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_ECDSA, &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_ECDSA);
		}
		if (!bad) {
			token->operations |= 1 << OP_ECDSA;
			if (best_ecdsa_token == NULL) {
				best_ecdsa_token = token;
			}
		}

		/* Check for EDDSA support */
		bad = false;
		rv = pkcs_C_GetMechanismInfo(slot, CKM_EC_EDWARDS_KEY_PAIR_GEN,
					     &mechInfo);
		if ((rv != CKR_OK) ||
		    ((mechInfo.flags & CKF_GENERATE_KEY_PAIR) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_EC_EDWARDS_KEY_PAIR_GEN);
		}
		rv = pkcs_C_GetMechanismInfo(slot, CKM_EDDSA, &mechInfo);
		if ((rv != CKR_OK) || ((mechInfo.flags & CKF_SIGN) == 0) ||
		    ((mechInfo.flags & CKF_VERIFY) == 0))
		{
			bad = true;
			PK11_TRACEM(CKM_EDDSA);
		}
		if (!bad) {
			token->operations |= 1 << OP_EDDSA;
			if (best_eddsa_token == NULL) {
				best_eddsa_token = token;
			}
		}
	}

	if (slotList != NULL) {
		pk11_mem_put(slotList, sizeof(CK_SLOT_ID) * slotCount);
	}
}

CK_SLOT_ID
pk11_get_best_token(pk11_optype_t optype) {
	pk11_token_t *token = NULL;

	switch (optype) {
	case OP_RSA:
		token = best_rsa_token;
		break;
	case OP_ECDSA:
		token = best_ecdsa_token;
		break;
	case OP_EDDSA:
		token = best_eddsa_token;
		break;
	default:
		break;
	}
	if (token == NULL) {
		return (0);
	}
	return (token->slotid);
}

isc_result_t
pk11_numbits(CK_BYTE_PTR data, unsigned int bytecnt, unsigned int *bits) {
	unsigned int bitcnt, i;
	CK_BYTE top;

	if (bytecnt == 0) {
		*bits = 0;
		return (ISC_R_SUCCESS);
	}
	bitcnt = bytecnt * 8;
	for (i = 0; i < bytecnt; i++) {
		top = data[i];
		if (top == 0) {
			bitcnt -= 8;
			continue;
		}
		if (top & 0x80) {
			*bits = bitcnt;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x40) {
			*bits = bitcnt - 1;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x20) {
			*bits = bitcnt - 2;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x10) {
			*bits = bitcnt - 3;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x08) {
			*bits = bitcnt - 4;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x04) {
			*bits = bitcnt - 5;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x02) {
			*bits = bitcnt - 6;
			return (ISC_R_SUCCESS);
		}
		if (top & 0x01) {
			*bits = bitcnt - 7;
			return (ISC_R_SUCCESS);
		}
		break;
	}
	return (ISC_R_RANGE);
}

CK_ATTRIBUTE *
pk11_attribute_first(const pk11_object_t *obj) {
	return (obj->repr);
}

CK_ATTRIBUTE *
pk11_attribute_next(const pk11_object_t *obj, CK_ATTRIBUTE *attr) {
	CK_ATTRIBUTE *next;

	next = attr + 1;
	if ((next - obj->repr) >= obj->attrcnt) {
		return (NULL);
	}
	return (next);
}

CK_ATTRIBUTE *
pk11_attribute_bytype(const pk11_object_t *obj, CK_ATTRIBUTE_TYPE type) {
	CK_ATTRIBUTE *attr;

	for (attr = pk11_attribute_first(obj); attr != NULL;
	     attr = pk11_attribute_next(obj, attr))
	{
		if (attr->type == type) {
			return (attr);
		}
	}
	return (NULL);
}

static char *
percent_decode(char *x, size_t *len) {
	char *p, *c;
	unsigned char v = 0;

	INSIST(len != NULL);

	for (p = c = x; p[0] != '\0'; p++, c++) {
		switch (p[0]) {
		case '%':
			switch (p[1]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				v = (p[1] - '0') << 4;
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				v = (p[1] - 'A' + 10) << 4;
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				v = (p[1] - 'a' + 10) << 4;
				break;
			default:
				return (NULL);
			}
			switch (p[2]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				v |= (p[2] - '0') & 0x0f;
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				v = (p[2] - 'A' + 10) & 0x0f;
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				v = (p[2] - 'a' + 10) & 0x0f;
				break;
			default:
				return (NULL);
			}
			p += 2;
			*c = (char)v;
			(*len)++;
			break;
		default:
			*c = *p;
			(*len)++;
		}
	}
	return (x);
}

static bool
pk11strcmp(const char *x, size_t lenx, const char *y, size_t leny) {
	char buf[32];

	INSIST((leny == 32) || (leny == 16));

	memset(buf, ' ', 32);
	if (lenx > leny) {
		lenx = leny;
	}
	memmove(buf, x, lenx);
	return (memcmp(buf, y, leny) == 0);
}

static CK_ATTRIBUTE *
push_attribute(pk11_object_t *obj, isc_mem_t *mctx, size_t len) {
	CK_ATTRIBUTE *old = obj->repr;
	CK_ATTRIBUTE *attr;
	CK_BYTE cnt = obj->attrcnt;

	REQUIRE(old != NULL || cnt == 0);

	obj->repr = isc_mem_get(mctx, (cnt + 1) * sizeof(*attr));
	memset(obj->repr, 0, (cnt + 1) * sizeof(*attr));
	if (old != NULL) {
		memmove(obj->repr, old, cnt * sizeof(*attr));
	}
	attr = obj->repr + cnt;
	attr->ulValueLen = (CK_ULONG)len;
	attr->pValue = isc_mem_get(mctx, len);
	memset(attr->pValue, 0, len);
	if (old != NULL) {
		memset(old, 0, cnt * sizeof(*attr));
		isc_mem_put(mctx, old, cnt * sizeof(*attr));
	}
	obj->attrcnt++;
	return (attr);
}

#define DST_RET(a)        \
	{                 \
		ret = a;  \
		goto err; \
	}

isc_result_t
pk11_parse_uri(pk11_object_t *obj, const char *label, isc_mem_t *mctx,
	       pk11_optype_t optype) {
	CK_ATTRIBUTE *attr;
	pk11_token_t *token = NULL;
	char *uri, *p, *a, *na, *v;
	size_t len, l;
	FILE *stream = NULL;
	char pin[PINLEN + 1];
	bool gotpin = false;
	isc_result_t ret;

	/* get values to work on */
	len = strlen(label) + 1;
	uri = isc_mem_get(mctx, len);
	memmove(uri, label, len);

	/* get the URI scheme */
	p = strchr(uri, ':');
	if (p == NULL) {
		DST_RET(PK11_R_NOPROVIDER);
	}
	*p++ = '\0';
	if (strcmp(uri, "pkcs11") != 0) {
		DST_RET(PK11_R_NOPROVIDER);
	}

	/* get attributes */
	for (na = p; na != NULL;) {
		a = na;
		p = strchr(a, ';');
		if (p == NULL) {
			/* last attribute */
			na = NULL;
		} else {
			*p++ = '\0';
			na = p;
		}
		p = strchr(a, '=');
		if (p != NULL) {
			*p++ = '\0';
			v = p;
		} else {
			v = a;
		}
		l = 0;
		v = percent_decode(v, &l);
		if (v == NULL) {
			DST_RET(PK11_R_NOPROVIDER);
		}
		if ((a == v) || (strcmp(a, "object") == 0)) {
			/* object: CKA_LABEL */
			attr = pk11_attribute_bytype(obj, CKA_LABEL);
			if (attr != NULL) {
				DST_RET(PK11_R_NOPROVIDER);
			}
			attr = push_attribute(obj, mctx, l);
			if (attr == NULL) {
				DST_RET(ISC_R_NOMEMORY);
			}
			attr->type = CKA_LABEL;
			memmove(attr->pValue, v, l);
		} else if (strcmp(a, "token") == 0) {
			/* token: CK_TOKEN_INFO label */
			if (token == NULL) {
				for (token = ISC_LIST_HEAD(tokens);
				     token != NULL;
				     token = ISC_LIST_NEXT(token, link))
				{
					if (pk11strcmp(v, l, token->name, 32)) {
						break;
					}
				}
			}
		} else if (strcmp(a, "manufacturer") == 0) {
			/* manufacturer: CK_TOKEN_INFO manufacturerID */
			if (token == NULL) {
				for (token = ISC_LIST_HEAD(tokens);
				     token != NULL;
				     token = ISC_LIST_NEXT(token, link))
				{
					if (pk11strcmp(v, l, token->manuf, 32))
					{
						break;
					}
				}
			}
		} else if (strcmp(a, "serial") == 0) {
			/* serial: CK_TOKEN_INFO serialNumber */
			if (token == NULL) {
				for (token = ISC_LIST_HEAD(tokens);
				     token != NULL;
				     token = ISC_LIST_NEXT(token, link))
				{
					if (pk11strcmp(v, l, token->serial, 16))
					{
						break;
					}
				}
			}
		} else if (strcmp(a, "model") == 0) {
			/* model: CK_TOKEN_INFO model */
			if (token == NULL) {
				for (token = ISC_LIST_HEAD(tokens);
				     token != NULL;
				     token = ISC_LIST_NEXT(token, link))
				{
					if (pk11strcmp(v, l, token->model, 16))
					{
						break;
					}
				}
			}
		} else if (strcmp(a, "library-manufacturer") == 0) {
			/* ignored */
		} else if (strcmp(a, "library-description") == 0) {
			/* ignored */
		} else if (strcmp(a, "library-version") == 0) {
			/* ignored */
		} else if (strcmp(a, "object-type") == 0) {
			/* object-type: CKA_CLASS */
			/* only private makes sense */
			if (strcmp(v, "private") != 0) {
				DST_RET(PK11_R_NOPROVIDER);
			}
		} else if (strcmp(a, "id") == 0) {
			/* id: CKA_ID */
			attr = pk11_attribute_bytype(obj, CKA_ID);
			if (attr != NULL) {
				DST_RET(PK11_R_NOPROVIDER);
			}
			attr = push_attribute(obj, mctx, l);
			if (attr == NULL) {
				DST_RET(ISC_R_NOMEMORY);
			}
			attr->type = CKA_ID;
			memmove(attr->pValue, v, l);
		} else if (strcmp(a, "pin-source") == 0) {
			/* pin-source: PIN */
			ret = isc_stdio_open(v, "r", &stream);
			if (ret != ISC_R_SUCCESS) {
				goto err;
			}
			memset(pin, 0, PINLEN + 1);
			ret = isc_stdio_read(pin, 1, PINLEN + 1, stream, &l);
			if ((ret != ISC_R_SUCCESS) && (ret != ISC_R_EOF)) {
				goto err;
			}
			if (l > PINLEN) {
				DST_RET(ISC_R_RANGE);
			}
			ret = isc_stdio_close(stream);
			stream = NULL;
			if (ret != ISC_R_SUCCESS) {
				goto err;
			}
			gotpin = true;
		} else {
			DST_RET(PK11_R_NOPROVIDER);
		}
	}

	if ((pk11_attribute_bytype(obj, CKA_LABEL) == NULL) &&
	    (pk11_attribute_bytype(obj, CKA_ID) == NULL))
	{
		DST_RET(ISC_R_NOTFOUND);
	}

	if (token == NULL) {
		if (optype == OP_RSA) {
			token = best_rsa_token;
		} else if (optype == OP_ECDSA) {
			token = best_ecdsa_token;
		} else if (optype == OP_EDDSA) {
			token = best_eddsa_token;
		}
	}
	if (token == NULL) {
		DST_RET(ISC_R_NOTFOUND);
	}
	obj->slot = token->slotid;
	if (gotpin) {
		memmove(token->pin, pin, PINLEN + 1);
		obj->reqlogon = true;
	}

	ret = ISC_R_SUCCESS;

err:
	if (stream != NULL) {
		(void)isc_stdio_close(stream);
	}
	isc_mem_put(mctx, uri, len);
	return (ret);
}

void
pk11_error_fatalcheck(const char *file, int line, const char *funcname,
		      CK_RV rv) {
	isc_error_fatal(file, line, "%s: Error = 0x%.8lX\n", funcname, rv);
}

void
pk11_dump_tokens(void) {
	pk11_token_t *token;
	bool first;

	printf("DEFAULTS\n");
	printf("\tbest_rsa_token=%p\n", best_rsa_token);
	printf("\tbest_ecdsa_token=%p\n", best_ecdsa_token);
	printf("\tbest_eddsa_token=%p\n", best_eddsa_token);

	for (token = ISC_LIST_HEAD(tokens); token != NULL;
	     token = ISC_LIST_NEXT(token, link))
	{
		printf("\nTOKEN\n");
		printf("\taddress=%p\n", token);
		printf("\tslotID=%lu\n", token->slotid);
		printf("\tlabel=%.32s\n", token->name);
		printf("\tmanufacturerID=%.32s\n", token->manuf);
		printf("\tmodel=%.16s\n", token->model);
		printf("\tserialNumber=%.16s\n", token->serial);
		printf("\tsupported operations=0x%x (", token->operations);
		first = true;
		if (token->operations & (1 << OP_RSA)) {
			first = false;
			printf("RSA");
		}
		if (token->operations & (1 << OP_ECDSA)) {
			if (!first) {
				printf(",");
			}
			printf("EC");
		}
		printf(")\n");
	}
}
