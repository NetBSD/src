/* $Id: mech_crammd5.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

/* Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mateusz Kocielski.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.	IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <saslc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saslc_private.h"
#include "mech.h"
#include "crypto.h"

/* local headers */

/* properties */
#define SASLC_CRAM_MD5_AUTHID		"AUTHID"
#define SASLC_CRAM_MD5_PASSWORD		"PASSWD"

static int saslc__mech_crammd5_cont(saslc_sess_t *, const void *, size_t,
    void **, size_t *);

/**
 * @brief doing one step of the sasl authentication
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out place to store output data
 * @param outlen output data length
 * @return MECH_OK - success,
 * MECH_STEP - more steps are needed,
 * MECH_ERROR - error
 */

static int
saslc__mech_crammd5_cont(saslc_sess_t *sess, const void *in, size_t inlen,
    void **out, size_t *outlen)
{
	const char *authid, *passwd;
	char *digest, *name;
	int len;

	/* server is doing first step, but some clients may call this function
	 * before getting data from server */
	if (inlen == 0) {
		*out = NULL;
		*outlen = 0;
		return MECH_STEP;
	}

	if ((authid = saslc_sess_getprop(sess, SASLC_CRAM_MD5_AUTHID))
	    == NULL) {
		saslc__error_set(ERR(sess), ERROR_MECH,
		    "authid is required for an authentication");
		return MECH_ERROR;
	}

	if ((passwd = saslc_sess_getprop(sess, SASLC_CRAM_MD5_PASSWORD))
	    == NULL) {
		saslc__error_set(ERR(sess), ERROR_MECH,
		    "passwd is required for an authentication");
		return MECH_ERROR;
	}


	digest = saslc__crypto_hmac_md5((const unsigned char *)passwd,
	    strlen(passwd), in, inlen);

	if ((len = asprintf(&name, "%s %s", authid, digest)) == -1) {
		free(digest);
		saslc__error_set_errno(ERR(sess), ERROR_NOMEM);
		return MECH_ERROR;
	}
	*out = name;
	*outlen = len + 1;

	free(digest);

	return MECH_OK;
}

/* mechanism definition */
const saslc__mech_t saslc__mech_crammd5 = {
	"CRAM-MD5", /* name */
	FLAG_DICTIONARY, /* flags */
	saslc__mech_generic_create, /* create */
	saslc__mech_crammd5_cont, /* step */
	NULL, /* encode */
	NULL, /* decode */
	saslc__mech_generic_destroy /* destroy */
};
