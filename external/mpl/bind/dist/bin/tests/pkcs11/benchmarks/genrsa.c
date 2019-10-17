/*	$NetBSD: genrsa.c,v 1.3.4.1 2019/10/17 19:34:15 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Portions copyright (c) 2008 Nominet UK.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* genrsa [-m module] [-s $slot] [-p pin] [-t] [-b bits] [-n count] */

/*! \file */

#include <config.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <isc/commandline.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <pk11/pk11.h>
#include <pk11/result.h>

#ifndef HAVE_CLOCK_GETTIME

#include <sys/time.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

static int clock_gettime(int32_t id, struct timespec *tp);

static int
clock_gettime(int32_t id, struct timespec *tp)
{
	struct timeval tv;
	int result;

	UNUSED(id);

	result = gettimeofday(&tv, NULL);
	if (result == 0) {
		tp->tv_sec = tv.tv_sec;
		tp->tv_nsec = (long) tv.tv_usec * 1000;
	}
	return (result);
}
#endif

static CK_BBOOL truevalue = TRUE;
static CK_BBOOL falsevalue = FALSE;

int
main(int argc, char *argv[]) {
	isc_result_t result;
	CK_RV rv;
	CK_SLOT_ID slot = 0;
	CK_SESSION_HANDLE hSession = CK_INVALID_HANDLE;
	CK_MECHANISM mech = { CKM_RSA_PKCS_KEY_PAIR_GEN, NULL, 0 };
	CK_OBJECT_HANDLE *pubKey;
	CK_OBJECT_HANDLE *privKey;
	CK_OBJECT_CLASS pubClass = CKO_PUBLIC_KEY;
	CK_OBJECT_CLASS privClass = CKO_PRIVATE_KEY;
	CK_KEY_TYPE kType = CKK_RSA;
	CK_ULONG bits = 1024;
	CK_BYTE exponent[] = { 0x01, 0x00, 0x01 };
	CK_ATTRIBUTE pubTemplate[] =
	{
		{ CKA_CLASS, &pubClass, (CK_ULONG) sizeof(pubClass) },
		{ CKA_KEY_TYPE, &kType, (CK_ULONG) sizeof(kType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_VERIFY, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_MODULUS_BITS, &bits, (CK_ULONG) sizeof(bits) },
		{ CKA_PUBLIC_EXPONENT, exponent, (CK_ULONG) sizeof(exponent) }
	};
	CK_ATTRIBUTE privTemplate[] =
	{
		{ CKA_CLASS, &privClass, (CK_ULONG) sizeof(privClass) },
		{ CKA_KEY_TYPE, &kType, (CK_ULONG) sizeof(kType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_SIGN, &truevalue, (CK_ULONG) sizeof(truevalue) },
	};
	pk11_context_t pctx;
	pk11_optype_t op_type = OP_RSA;
	char *lib_name = NULL;
	char *pin = NULL;
	int error = 0;
	int c, errflg = 0;
	int ontoken  = 0;
	unsigned int count = 1000;
	unsigned int i;
	struct timespec starttime;
	struct timespec endtime;

	while ((c = isc_commandline_parse(argc, argv, ":m:s:p:tb:n:")) != -1) {
		switch (c) {
		case 'm':
			lib_name = isc_commandline_argument;
			break;
		case 's':
			slot = atoi(isc_commandline_argument);
			op_type = OP_ANY;
			break;
		case 'p':
			pin = isc_commandline_argument;
			break;
		case 't':
			ontoken = 1;
			break;
		case 'b':
			bits = (CK_ULONG)atoi(isc_commandline_argument);
			break;
		case 'n':
			count = atoi(isc_commandline_argument);
			break;
		case ':':
			fprintf(stderr,
				"Option -%c requires an operand\n",
				isc_commandline_option);
			errflg++;
			break;
		case '?':
		default:
			fprintf(stderr, "Unrecognised option: -%c\n",
				isc_commandline_option);
			errflg++;
		}
	}

	if (errflg) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr,
			"\tgenrsa [-m module] [-s slot] [-p pin] "
			"[-t] [-b bits] [-n count]\n");
		exit(1);
	}

	pk11_result_register();

	/* Allocate hanles */
	pubKey = (CK_SESSION_HANDLE *)
		malloc(count * sizeof(CK_SESSION_HANDLE));
	if (pubKey == NULL) {
		perror("malloc");
		exit(1);
	}
	privKey = (CK_SESSION_HANDLE *)
		malloc(count * sizeof(CK_SESSION_HANDLE));
	if (privKey == NULL) {
		free(pubKey);
		perror("malloc");
		exit(1);
	}
	for (i = 0; i < count; i++) {
		pubKey[i] = CK_INVALID_HANDLE;
		privKey[i] = CK_INVALID_HANDLE;
	}

	/* Initialize the CRYPTOKI library */
	if (lib_name != NULL)
		pk11_set_lib_name(lib_name);

	if (pin == NULL) {
		pin = getpass("Enter Pin: ");
	}

	result = pk11_get_session(&pctx, op_type, false, true,
				  true, (const char *) pin, slot);
	if ((result != ISC_R_SUCCESS) &&
	    (result != PK11_R_NORANDOMSERVICE) &&
	    (result != PK11_R_NODIGESTSERVICE) &&
	    (result != PK11_R_NOAESSERVICE)) {
		fprintf(stderr, "Error initializing PKCS#11: %s\n",
			isc_result_totext(result));
		exit(1);
	}

	if (pin != NULL)
		memset(pin, 0, strlen((char *)pin));

	hSession = pctx.session;

	if (ontoken) {
		pubTemplate[2].pValue = &truevalue;
		privTemplate[2].pValue = &truevalue;
	}

	if (clock_gettime(CLOCK_REALTIME, &starttime) < 0) {
		perror("clock_gettime(start)");
		goto exit_keys;
	}

	for (i = 0; i < count; i++) {
		rv = pkcs_C_GenerateKeyPair(hSession, &mech,
				       pubTemplate, 7,
				       privTemplate, 5,
				       &pubKey[i], &privKey[i]);
		if (rv != CKR_OK) {
			fprintf(stderr,
				"C_GenerateKeyPair[%u]: Error = 0x%.8lX\n",
				i, rv);
			error = 1;
			if (i == 0)
				goto exit_keys;
			break;
		}
	}

	if (clock_gettime(CLOCK_REALTIME, &endtime) < 0) {
		perror("clock_gettime(end)");
		goto exit_keys;
	}

	endtime.tv_sec -= starttime.tv_sec;
	endtime.tv_nsec -= starttime.tv_nsec;
	while (endtime.tv_nsec < 0) {
		endtime.tv_sec -= 1;
		endtime.tv_nsec += 1000000000;
	}
	printf("%u generated RSA in %ld.%09lds\n", i,
	       endtime.tv_sec, endtime.tv_nsec);
	if (i > 0)
		printf("%g generated RSA/s\n",
		       1024 * i / ((double) endtime.tv_sec +
				   (double) endtime.tv_nsec / 1000000000.));

    exit_keys:
	for (i = 0; i < count; i++) {
		/* Destroy keys */
		if (pubKey[i] == CK_INVALID_HANDLE)
			goto destroy_priv;
		rv = pkcs_C_DestroyObject(hSession, pubKey[i]);
		if ((rv != CKR_OK) && !errflg) {
			fprintf(stderr,
				"C_DestroyObject[pub%u]: Error = 0x%.8lX\n",
				i, rv);
			errflg = 1;
		}
	    destroy_priv:
		if (privKey[i] == CK_INVALID_HANDLE)
			continue;
		rv = pkcs_C_DestroyObject(hSession, privKey[i]);
		if ((rv != CKR_OK) && !errflg) {
			fprintf(stderr,
				"C_DestroyObject[priv%u]: Error = 0x%.8lX\n",
				i, rv);
			errflg = 1;
		}
	}

	free(pubKey);
	free(privKey);

	pk11_return_session(&pctx);
	(void) pk11_finalize();

	exit(error);
}
