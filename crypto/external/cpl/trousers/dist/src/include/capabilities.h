
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _CAPABILITIES_H_
#define _CAPABILITIES_H_

/* This header has all of the software capabilities that are returned either via
 * Tspi_Context_GetCapability or TCS_GetCapability.
 */

/* TSP */
/* BOOL */
#define INTERNAL_CAP_TSP_ALG_RSA		TRUE
#define INTERNAL_CAP_TSP_ALG_SHA		TRUE
#define INTERNAL_CAP_TSP_ALG_3DES		FALSE
#define INTERNAL_CAP_TSP_ALG_DES		FALSE
#define INTERNAL_CAP_TSP_ALG_HMAC		TRUE
#define INTERNAL_CAP_TSP_ALG_AES		TRUE
#define INTERNAL_CAP_TSP_PERSSTORAGE		TRUE

/* UINT32 */
#define INTERNAL_CAP_TSP_ALG_DEFAULT		TSS_ALG_RSA
#define INTERNAL_CAP_TSP_ALG_DEFAULT_SIZE	TSS_KEY_SIZEVAL_2048BIT

/* 1 indicates byte-stream return values, 0 indicates ASN.1 encoded return values */
#define INTERNAL_CAP_TSP_RETURNVALUE_INFO	1

/* 0 is unknown platform version/type. Currently the spec is too vague on possible values for this
 * information to define anything here. */
#define INTERNAL_CAP_TSP_PLATFORM_VERSION	0
#define INTERNAL_CAP_TSP_PLATFORM_TYPE		0

/* TCS */
/* BOOL */
#define INTERNAL_CAP_TCS_ALG_RSA		FALSE
#define INTERNAL_CAP_TCS_ALG_AES		FALSE
#define INTERNAL_CAP_TCS_ALG_3DES		FALSE
#define INTERNAL_CAP_TCS_ALG_DES		FALSE
#define INTERNAL_CAP_TCS_ALG_SHA		TRUE
#define INTERNAL_CAP_TCS_ALG_HMAC		FALSE
#define INTERNAL_CAP_TCS_PERSSTORAGE		TRUE
#define INTERNAL_CAP_TCS_CACHING_KEYCACHE	FALSE
#define INTERNAL_CAP_TCS_CACHING_AUTHCACHE	TRUE

/* UINT32 */
#define INTERNAL_CAP_TCS_ALG_DEFAULT		TSS_ALG_RSA
#define INTERNAL_CAP_TCS_ALG_DEFAULT_SIZE	TSS_KEY_SIZEVAL_2048BIT

/* Common between both TSP and TCS */
#define INTERNAL_CAP_VERSION			{ 1, 2, TSS_VER_MAJOR, TSS_VER_MINOR }

#define INTERNAL_CAP_MANUFACTURER_ID		0x49424D00
#define INTERNAL_CAP_MANUFACTURER_STR		{ 'I', 0, 'B', 0, 'M', 0, 0, 0 }
#define INTERNAL_CAP_MANUFACTURER_STR_LEN	8

#endif
