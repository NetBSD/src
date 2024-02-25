/*	$NetBSD: keygen.c,v 1.6.2.1 2024/02/25 15:43:00 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include "keygen.h"
#include <stdarg.h>
#include <stdlib.h>

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>

#include <dns/keyvalues.h>
#include <dns/name.h>

#include <dst/dst.h>

#include <confgen/os.h>

#include "util.h"

/*%
 * Convert string to algorithm type.
 */
dns_secalg_t
alg_fromtext(const char *name) {
	const char *p = name;
	if (strncasecmp(p, "hmac-", 5) == 0) {
		p = &name[5];
	}

	if (strcasecmp(p, "md5") == 0) {
		return (DST_ALG_HMACMD5);
	}
	if (strcasecmp(p, "sha1") == 0) {
		return (DST_ALG_HMACSHA1);
	}
	if (strcasecmp(p, "sha224") == 0) {
		return (DST_ALG_HMACSHA224);
	}
	if (strcasecmp(p, "sha256") == 0) {
		return (DST_ALG_HMACSHA256);
	}
	if (strcasecmp(p, "sha384") == 0) {
		return (DST_ALG_HMACSHA384);
	}
	if (strcasecmp(p, "sha512") == 0) {
		return (DST_ALG_HMACSHA512);
	}
	return (DST_ALG_UNKNOWN);
}

/*%
 * Return default keysize for a given algorithm type.
 */
int
alg_bits(dns_secalg_t alg) {
	switch (alg) {
	case DST_ALG_HMACMD5:
		return (128);
	case DST_ALG_HMACSHA1:
		return (160);
	case DST_ALG_HMACSHA224:
		return (224);
	case DST_ALG_HMACSHA256:
		return (256);
	case DST_ALG_HMACSHA384:
		return (384);
	case DST_ALG_HMACSHA512:
		return (512);
	default:
		return (0);
	}
}

/*%
 * Generate a key of size 'keysize' and place it in 'key_txtbuffer'
 */
void
generate_key(isc_mem_t *mctx, dns_secalg_t alg, int keysize,
	     isc_buffer_t *key_txtbuffer) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_buffer_t key_rawbuffer;
	isc_region_t key_rawregion;
	char key_rawsecret[64];
	dst_key_t *key = NULL;

	switch (alg) {
	case DST_ALG_HMACMD5:
	case DST_ALG_HMACSHA1:
	case DST_ALG_HMACSHA224:
	case DST_ALG_HMACSHA256:
		if (keysize < 1 || keysize > 512) {
			fatal("keysize %d out of range (must be 1-512)\n",
			      keysize);
		}
		break;
	case DST_ALG_HMACSHA384:
	case DST_ALG_HMACSHA512:
		if (keysize < 1 || keysize > 1024) {
			fatal("keysize %d out of range (must be 1-1024)\n",
			      keysize);
		}
		break;
	default:
		fatal("unsupported algorithm %d\n", alg);
	}

	DO("initialize dst library", dst_lib_init(mctx, NULL));

	DO("generate key",
	   dst_key_generate(dns_rootname, alg, keysize, 0, 0, DNS_KEYPROTO_ANY,
			    dns_rdataclass_in, mctx, &key, NULL));

	isc_buffer_init(&key_rawbuffer, &key_rawsecret, sizeof(key_rawsecret));

	DO("dump key to buffer", dst_key_tobuffer(key, &key_rawbuffer));

	isc_buffer_usedregion(&key_rawbuffer, &key_rawregion);

	DO("bsse64 encode secret",
	   isc_base64_totext(&key_rawregion, -1, "", key_txtbuffer));

	if (key != NULL) {
		dst_key_free(&key);
	}

	dst_lib_destroy();
}

/*%
 * Write a key file to 'keyfile'.  If 'user' is non-NULL,
 * make that user the owner of the file.  The key will have
 * the name 'keyname' and the secret in the buffer 'secret'.
 */
void
write_key_file(const char *keyfile, const char *user, const char *keyname,
	       isc_buffer_t *secret, dns_secalg_t alg) {
	isc_result_t result;
	const char *algname = dst_hmac_algorithm_totext(alg);
	FILE *fd = NULL;

	DO("create keyfile", isc_file_safecreate(keyfile, &fd));

	if (user != NULL) {
		if (set_user(fd, user) == -1) {
			fatal("unable to set file owner\n");
		}
	}

	fprintf(fd,
		"key \"%s\" {\n\talgorithm %s;\n"
		"\tsecret \"%.*s\";\n};\n",
		keyname, algname, (int)isc_buffer_usedlength(secret),
		(char *)isc_buffer_base(secret));
	fflush(fd);
	if (ferror(fd)) {
		fatal("write to %s failed\n", keyfile);
	}
	if (fclose(fd)) {
		fatal("fclose(%s) failed\n", keyfile);
	}
}
