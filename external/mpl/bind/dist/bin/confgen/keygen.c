/*	$NetBSD: keygen.c,v 1.2 2018/08/12 13:02:27 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <stdlib.h>
#include <stdarg.h>

#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/entropy.h>
#include <isc/file.h>
#include <isc/keyboard.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>

#include <pk11/site.h>

#include <dns/keyvalues.h>
#include <dns/name.h>

#include <dst/dst.h>
#include <confgen/os.h>

#include "util.h"
#include "keygen.h"

/*%
 * Convert algorithm type to string.
 */
const char *
alg_totext(dns_secalg_t alg) {
	switch (alg) {
#ifndef PK11_MD5_DISABLE
	    case DST_ALG_HMACMD5:
		return "hmac-md5";
#endif
	    case DST_ALG_HMACSHA1:
		return "hmac-sha1";
	    case DST_ALG_HMACSHA224:
		return "hmac-sha224";
	    case DST_ALG_HMACSHA256:
		return "hmac-sha256";
	    case DST_ALG_HMACSHA384:
		return "hmac-sha384";
	    case DST_ALG_HMACSHA512:
		return "hmac-sha512";
	    default:
		return "(unknown)";
	}
}

/*%
 * Convert string to algorithm type.
 */
dns_secalg_t
alg_fromtext(const char *name) {
	const char *p = name;
	if (strncasecmp(p, "hmac-", 5) == 0)
		p = &name[5];

#ifndef PK11_MD5_DISABLE
	if (strcasecmp(p, "md5") == 0)
		return DST_ALG_HMACMD5;
#endif
	if (strcasecmp(p, "sha1") == 0)
		return DST_ALG_HMACSHA1;
	if (strcasecmp(p, "sha224") == 0)
		return DST_ALG_HMACSHA224;
	if (strcasecmp(p, "sha256") == 0)
		return DST_ALG_HMACSHA256;
	if (strcasecmp(p, "sha384") == 0)
		return DST_ALG_HMACSHA384;
	if (strcasecmp(p, "sha512") == 0)
		return DST_ALG_HMACSHA512;
	return DST_ALG_UNKNOWN;
}

/*%
 * Return default keysize for a given algorithm type.
 */
int
alg_bits(dns_secalg_t alg) {
	switch (alg) {
	    case DST_ALG_HMACMD5:
		return 128;
	    case DST_ALG_HMACSHA1:
		return 160;
	    case DST_ALG_HMACSHA224:
		return 224;
	    case DST_ALG_HMACSHA256:
		return 256;
	    case DST_ALG_HMACSHA384:
		return 384;
	    case DST_ALG_HMACSHA512:
		return 512;
	    default:
		return 0;
	}
}

/*%
 * Generate a key of size 'keysize' using entropy source 'randomfile',
 * and place it in 'key_txtbuffer'
 */
void
generate_key(isc_mem_t *mctx, const char *randomfile, dns_secalg_t alg,
	     int keysize, isc_buffer_t *key_txtbuffer) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_entropysource_t *entropy_source = NULL;
	int open_keyboard = ISC_ENTROPY_KEYBOARDMAYBE;
	int entropy_flags = 0;
	isc_entropy_t *ectx = NULL;
	isc_buffer_t key_rawbuffer;
	isc_region_t key_rawregion;
	char key_rawsecret[64];
	dst_key_t *key = NULL;

	switch (alg) {
#ifndef PK11_MD5_DISABLE
	    case DST_ALG_HMACMD5:
#endif
	    case DST_ALG_HMACSHA1:
	    case DST_ALG_HMACSHA224:
	    case DST_ALG_HMACSHA256:
		if (keysize < 1 || keysize > 512)
			fatal("keysize %d out of range (must be 1-512)\n",
			      keysize);
		break;
	    case DST_ALG_HMACSHA384:
	    case DST_ALG_HMACSHA512:
		if (keysize < 1 || keysize > 1024)
			fatal("keysize %d out of range (must be 1-1024)\n",
			      keysize);
		break;
	    default:
		fatal("unsupported algorithm %d\n", alg);
	}


	DO("create entropy context", isc_entropy_create(mctx, &ectx));

#ifdef ISC_PLATFORM_CRYPTORANDOM
	if (randomfile == NULL) {
		isc_entropy_usehook(ectx, ISC_TRUE);
	}
#endif
	if (randomfile != NULL && strcmp(randomfile, "keyboard") == 0) {
		randomfile = NULL;
		open_keyboard = ISC_ENTROPY_KEYBOARDYES;
	}
	DO("start entropy source", isc_entropy_usebestsource(ectx,
							     &entropy_source,
							     randomfile,
							     open_keyboard));

	entropy_flags = ISC_ENTROPY_BLOCKING | ISC_ENTROPY_GOODONLY;

	DO("initialize dst library", dst_lib_init(mctx, ectx, entropy_flags));

	DO("generate key", dst_key_generate(dns_rootname, alg,
					    keysize, 0, 0,
					    DNS_KEYPROTO_ANY,
					    dns_rdataclass_in, mctx, &key));

	isc_buffer_init(&key_rawbuffer, &key_rawsecret, sizeof(key_rawsecret));

	DO("dump key to buffer", dst_key_tobuffer(key, &key_rawbuffer));

	isc_buffer_usedregion(&key_rawbuffer, &key_rawregion);

	DO("bsse64 encode secret", isc_base64_totext(&key_rawregion, -1, "",
						     key_txtbuffer));

	/*
	 * Shut down the entropy source now so the "stop typing" message
	 * does not muck with the output.
	 */
	if (entropy_source != NULL)
		isc_entropy_destroysource(&entropy_source);

	if (key != NULL)
		dst_key_free(&key);

	isc_entropy_detach(&ectx);
	dst_lib_destroy();
}

/*%
 * Write a key file to 'keyfile'.  If 'user' is non-NULL,
 * make that user the owner of the file.  The key will have
 * the name 'keyname' and the secret in the buffer 'secret'.
 */
void
write_key_file(const char *keyfile, const char *user,
	       const char *keyname, isc_buffer_t *secret,
	       dns_secalg_t alg) {
	isc_result_t result;
	const char *algname = alg_totext(alg);
	FILE *fd = NULL;

	DO("create keyfile", isc_file_safecreate(keyfile, &fd));

	if (user != NULL) {
		if (set_user(fd, user) == -1)
			fatal("unable to set file owner\n");
	}

	fprintf(fd, "key \"%s\" {\n\talgorithm %s;\n"
		"\tsecret \"%.*s\";\n};\n",
		keyname, algname,
		(int)isc_buffer_usedlength(secret),
		(char *)isc_buffer_base(secret));
	fflush(fd);
	if (ferror(fd))
		fatal("write to %s failed\n", keyfile);
	if (fclose(fd))
		fatal("fclose(%s) failed\n", keyfile);
	fprintf(stderr, "wrote key file \"%s\"\n", keyfile);
}

