/*	$NetBSD: pkcs11-keygen.c,v 1.7 2023/01/25 21:43:23 christos Exp $	*/

/*
 * Copyright (C) 2009, 2012, 2015 Internet Systems Consortium, Inc. ("ISC")
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

/* pkcs11-keygen - PKCS#11 key generator
 *
 * Create a key in the keystore of an HSM
 *
 * The calculation of key tag is left to the script
 * that converts the key into a DNSKEY RR and inserts
 * it into a zone file.
 *
 * usage:
 * pkcs11-keygen [-P] [-m module] [-s slot] [-e] [-b keysize]
 *               [-i id] [-p pin] -l label
 *
 */

/*! \file */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <isc/commandline.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <pk11/constants.h>
#include <pk11/pk11.h>
#include <pk11/result.h>

/* Define static key template values */
static CK_BBOOL truevalue = TRUE;
static CK_BBOOL falsevalue = FALSE;

/* Static arrays of data used for key template initialization */
static CK_BYTE pk11_ecc_prime256v1[] = PK11_ECC_PRIME256V1;
static CK_BYTE pk11_ecc_secp384r1[] = PK11_ECC_SECP384R1;
static CK_BYTE pk11_ecx_ed25519[] = PK11_ECX_ED25519;
static CK_BYTE pk11_ecx_ed448[] = PK11_ECX_ED448;

/* Key class: RSA, ECC, ECX, or unknown */
typedef enum { key_unknown, key_rsa, key_ecc, key_ecx } key_class_t;

/*
 * Private key template
 */
#define PRIVATE_LABEL	    0
#define PRIVATE_SIGN	    1
#define PRIVATE_TOKEN	    2
#define PRIVATE_PRIVATE	    3
#define PRIVATE_SENSITIVE   4
#define PRIVATE_EXTRACTABLE 5
#define PRIVATE_ID	    6
#define PRIVATE_ATTRS	    7
static CK_ATTRIBUTE private_template[] = {
	{ CKA_LABEL, NULL_PTR, 0 },
	{ CKA_SIGN, &truevalue, sizeof(truevalue) },
	{ CKA_TOKEN, &truevalue, sizeof(truevalue) },
	{ CKA_PRIVATE, &truevalue, sizeof(truevalue) },
	{ CKA_SENSITIVE, &truevalue, sizeof(truevalue) },
	{ CKA_EXTRACTABLE, &falsevalue, sizeof(falsevalue) },
	{ CKA_ID, NULL_PTR, 0 }
};

/*
 * Public key template for RSA keys
 */
#define RSA_LABEL	    0
#define RSA_VERIFY	    1
#define RSA_TOKEN	    2
#define RSA_PRIVATE	    3
#define RSA_MODULUS_BITS    4
#define RSA_PUBLIC_EXPONENT 5
#define RSA_ID		    6
#define RSA_ATTRS	    7
static CK_ATTRIBUTE rsa_template[] = {
	{ CKA_LABEL, NULL_PTR, 0 },
	{ CKA_VERIFY, &truevalue, sizeof(truevalue) },
	{ CKA_TOKEN, &truevalue, sizeof(truevalue) },
	{ CKA_PRIVATE, &falsevalue, sizeof(falsevalue) },
	{ CKA_MODULUS_BITS, NULL_PTR, 0 },
	{ CKA_PUBLIC_EXPONENT, NULL_PTR, 0 },
	{ CKA_ID, NULL_PTR, 0 }
};

/*
 * Public key template for ECC/ECX keys
 */
#define ECC_LABEL   0
#define ECC_VERIFY  1
#define ECC_TOKEN   2
#define ECC_PRIVATE 3
#define ECC_PARAMS  4
#define ECC_ID	    5
#define ECC_ATTRS   6
static CK_ATTRIBUTE ecc_template[] = {
	{ CKA_LABEL, NULL_PTR, 0 },
	{ CKA_VERIFY, &truevalue, sizeof(truevalue) },
	{ CKA_TOKEN, &truevalue, sizeof(truevalue) },
	{ CKA_PRIVATE, &falsevalue, sizeof(falsevalue) },
	{ CKA_EC_PARAMS, NULL_PTR, 0 },
	{ CKA_ID, NULL_PTR, 0 }
};

/*
 * Convert from text to key class.  Accepts the names of DNSSEC
 * signing algorithms, so e.g., ECDSAP256SHA256 maps to ECC and
 * NSEC3RSASHA1 maps to RSA.
 */
static key_class_t
keyclass_fromtext(const char *name) {
	if (name == NULL) {
		return (key_unknown);
	}

	if (strncasecmp(name, "rsa", 3) == 0 ||
	    strncasecmp(name, "nsec3rsa", 8) == 0)
	{
		return (key_rsa);
	} else if (strncasecmp(name, "ecc", 3) == 0 ||
		   strncasecmp(name, "ecdsa", 5) == 0)
	{
		return (key_ecc);
	} else if (strncasecmp(name, "ecx", 3) == 0 ||
		   strncasecmp(name, "ed", 2) == 0)
	{
		return (key_ecx);
	} else {
		return (key_unknown);
	}
}

static void
usage(void) {
	fprintf(stderr, "Usage:\n"
			"\tpkcs11-keygen -a algorithm -b keysize -l label\n"
			"\t              [-P] [-m module] "
			"[-s slot] [-e] [-S] [-i id] [-p PIN]\n");
	exit(2);
}

int
main(int argc, char *argv[]) {
	isc_result_t result;
	CK_RV rv;
	CK_SLOT_ID slot = 0;
	CK_MECHANISM mech;
	CK_SESSION_HANDLE hSession;
	char *lib_name = NULL;
	char *pin = NULL;
	CK_ULONG bits = 0;
	CK_CHAR *label = NULL;
	CK_OBJECT_HANDLE privatekey, publickey;
	CK_BYTE exponent[5];
	CK_ULONG expsize = 0;
	pk11_context_t pctx;
	int error = 0;
	int c, errflg = 0;
	int hide = 1, quiet = 0;
	int idlen = 0, id_offset = 0;
	unsigned long id = 0;
	CK_BYTE idbuf[4];
	CK_ULONG ulObjectCount;
	CK_ATTRIBUTE search_template[] = { { CKA_LABEL, NULL_PTR, 0 } };
	CK_ATTRIBUTE *public_template = NULL;
	CK_ULONG public_attrcnt = 0, private_attrcnt = PRIVATE_ATTRS;
	key_class_t keyclass = key_rsa;
	pk11_optype_t op_type = OP_ANY;

#define OPTIONS ":a:b:ei:l:m:Pp:qSs:"
	while ((c = isc_commandline_parse(argc, argv, OPTIONS)) != -1) {
		switch (c) {
		case 'a':
			keyclass = keyclass_fromtext(isc_commandline_argument);
			break;
		case 'P':
			hide = 0;
			break;
		case 'm':
			lib_name = isc_commandline_argument;
			break;
		case 's':
			slot = atoi(isc_commandline_argument);
			break;
		case 'e':
			expsize = 5;
			break;
		case 'b':
			bits = atoi(isc_commandline_argument);
			break;
		case 'l':
			/* -l option is retained for backward compatibility * */
			label = (CK_CHAR *)isc_commandline_argument;
			break;
		case 'i':
			id = strtoul(isc_commandline_argument, NULL, 0);
			idlen = 4;
			break;
		case 'p':
			pin = isc_commandline_argument;
			break;
		case 'q':
			quiet = 1;
			break;
		case ':':
			fprintf(stderr, "Option -%c requires an operand\n",
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

	if (label == NULL && isc_commandline_index < argc) {
		label = (CK_CHAR *)argv[isc_commandline_index];
	}

	if (errflg || (label == NULL)) {
		usage();
	}

	if (expsize != 0 && keyclass != key_rsa) {
		fprintf(stderr, "The -e option is only compatible "
				"with RSA key generation\n");
		exit(2);
	}

	switch (keyclass) {
	case key_rsa:
		op_type = OP_RSA;
		if (expsize == 0) {
			expsize = 3;
		}
		if (bits == 0) {
			usage();
		}

		mech.mechanism = CKM_RSA_PKCS_KEY_PAIR_GEN;
		mech.pParameter = NULL;
		mech.ulParameterLen = 0;

		public_template = rsa_template;
		public_attrcnt = RSA_ATTRS;
		id_offset = RSA_ID;

		/* Set public exponent to F4 or F5 */
		exponent[0] = 0x01;
		exponent[1] = 0x00;
		if (expsize == 3) {
			exponent[2] = 0x01;
		} else {
			exponent[2] = 0x00;
			exponent[3] = 0x00;
			exponent[4] = 0x01;
		}

		public_template[RSA_MODULUS_BITS].pValue = &bits;
		public_template[RSA_MODULUS_BITS].ulValueLen = sizeof(bits);
		public_template[RSA_PUBLIC_EXPONENT].pValue = &exponent;
		public_template[RSA_PUBLIC_EXPONENT].ulValueLen = expsize;
		break;
	case key_ecc:
		op_type = OP_ECDSA;
		if (bits == 0) {
			bits = 256;
		} else if (bits != 256 && bits != 384) {
			fprintf(stderr, "ECC keys only support bit sizes of "
					"256 and 384\n");
			exit(2);
		}

		mech.mechanism = CKM_EC_KEY_PAIR_GEN;
		mech.pParameter = NULL;
		mech.ulParameterLen = 0;

		public_template = ecc_template;
		public_attrcnt = ECC_ATTRS;
		id_offset = ECC_ID;

		if (bits == 256) {
			public_template[4].pValue = pk11_ecc_prime256v1;
			public_template[4].ulValueLen =
				sizeof(pk11_ecc_prime256v1);
		} else {
			public_template[4].pValue = pk11_ecc_secp384r1;
			public_template[4].ulValueLen =
				sizeof(pk11_ecc_secp384r1);
		}

		break;
	case key_ecx:
		op_type = OP_EDDSA;
		if (bits == 0) {
			bits = 256;
		} else if (bits != 256 && bits != 456) {
			fprintf(stderr, "ECX keys only support bit sizes of "
					"256 and 456\n");
			exit(2);
		}

		mech.mechanism = CKM_EC_EDWARDS_KEY_PAIR_GEN;
		mech.pParameter = NULL;
		mech.ulParameterLen = 0;

		public_template = ecc_template;
		public_attrcnt = ECC_ATTRS;
		id_offset = ECC_ID;

		if (bits == 256) {
			public_template[4].pValue = pk11_ecx_ed25519;
			public_template[4].ulValueLen =
				sizeof(pk11_ecx_ed25519);
		} else {
			public_template[4].pValue = pk11_ecx_ed448;
			public_template[4].ulValueLen = sizeof(pk11_ecx_ed448);
		}

		break;
	case key_unknown:
		usage();
	}

	search_template[0].pValue = label;
	search_template[0].ulValueLen = strlen((char *)label);
	public_template[0].pValue = label;
	public_template[0].ulValueLen = strlen((char *)label);
	private_template[0].pValue = label;
	private_template[0].ulValueLen = strlen((char *)label);

	if (idlen == 0) {
		public_attrcnt--;
		private_attrcnt--;
	} else {
		if (id <= 0xffff) {
			idlen = 2;
			idbuf[0] = (CK_BYTE)(id >> 8);
			idbuf[1] = (CK_BYTE)id;
		} else {
			idbuf[0] = (CK_BYTE)(id >> 24);
			idbuf[1] = (CK_BYTE)(id >> 16);
			idbuf[2] = (CK_BYTE)(id >> 8);
			idbuf[3] = (CK_BYTE)id;
		}

		public_template[id_offset].pValue = idbuf;
		public_template[id_offset].ulValueLen = idlen;
		private_template[PRIVATE_ID].pValue = idbuf;
		private_template[PRIVATE_ID].ulValueLen = idlen;
	}

	pk11_result_register();

	/* Initialize the CRYPTOKI library */
	if (lib_name != NULL) {
		pk11_set_lib_name(lib_name);
	}

	if (pin == NULL) {
		pin = getpass("Enter Pin: ");
	}

	result = pk11_get_session(&pctx, op_type, false, true, true,
				  (const char *)pin, slot);
	if (result == PK11_R_NORANDOMSERVICE ||
	    result == PK11_R_NODIGESTSERVICE || result == PK11_R_NOAESSERVICE)
	{
		fprintf(stderr, "Warning: %s\n", isc_result_totext(result));
		fprintf(stderr, "This HSM will not work with BIND 9 "
				"using native PKCS#11.\n");
	} else if (result != ISC_R_SUCCESS) {
		fprintf(stderr,
			"Unrecoverable error initializing "
			"PKCS#11: %s\n",
			isc_result_totext(result));
		exit(1);
	}

	memset(pin, 0, strlen(pin));

	hSession = pctx.session;

	/* check if a key with the same id already exists */
	rv = pkcs_C_FindObjectsInit(hSession, search_template, 1);
	if (rv != CKR_OK) {
		fprintf(stderr, "C_FindObjectsInit: Error = 0x%.8lX\n", rv);
		error = 1;
		goto exit_session;
	}
	rv = pkcs_C_FindObjects(hSession, &privatekey, 1, &ulObjectCount);
	if (rv != CKR_OK) {
		fprintf(stderr, "C_FindObjects: Error = 0x%.8lX\n", rv);
		error = 1;
		goto exit_search;
	}
	if (ulObjectCount != 0) {
		fprintf(stderr, "Key already exists.\n");
		error = 1;
		goto exit_search;
	}

	/* Set attributes if the key is not to be hidden */
	if (!hide) {
		private_template[4].pValue = &falsevalue;
		private_template[5].pValue = &truevalue;
	}

	/* Generate Key pair for signing/verifying */
	rv = pkcs_C_GenerateKeyPair(hSession, &mech, public_template,
				    public_attrcnt, private_template,
				    private_attrcnt, &publickey, &privatekey);

	if (rv != CKR_OK) {
		fprintf(stderr, "C_GenerateKeyPair: Error = 0x%.8lX\n", rv);
		error = 1;
	} else if (!quiet) {
		printf("Key pair generation complete.\n");
	}

exit_search:
	rv = pkcs_C_FindObjectsFinal(hSession);
	if (rv != CKR_OK) {
		fprintf(stderr, "C_FindObjectsFinal: Error = 0x%.8lX\n", rv);
		error = 1;
	}

exit_session:
	pk11_return_session(&pctx);
	(void)pk11_finalize();

	exit(error);
}
