/*	$KAME: vendorid.c,v 1.7 2000/12/15 13:43:57 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "debug.h"

#include "localconf.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "vendorid.h"
#include "crypto_openssl.h"

/*
 * set hashed vendor id.
 * hash function is always MD5.
 */
vchar_t *
set_vendorid()
{
	vchar_t vid, *vidhash;
	char *v = VENDORID;

	vid.v = v;
	vid.l = strlen(v);

	vidhash = eay_md5_one(&vid);
	if (vidhash == NULL)
		return NULL;

	return vidhash;
}

int
check_vendorid(gen)
	struct isakmp_gen *gen;		/* points to Vendor ID payload */
{
	vchar_t *vidhash = lcconf->vendorid;

	plog(LLV_DEBUG, LOCATION, NULL, "Vender ID received\n");

	if (!gen)
		return -1;
	if (vidhash == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ignoring Vendor ID as I don't have one.\n");
		return 0;
	}

	if (vidhash->l == ntohs(gen->len) - sizeof(*gen)
	 && memcmp(vidhash->v, gen + 1, vidhash->l) == 0) {
		plog(LLV_INFO, LOCATION, NULL,
			"Vendor ID matched.\n");
		return 0;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "Vendor ID mismatch.\n");

	return -1;
}

