/*	$KAME: vendorid.c,v 1.8 2001/03/27 02:39:57 thorpej Exp $	*/

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

const char *vendorid_strings[] = VENDORID_STRINGS;

/*
 * set hashed vendor id.
 * hash function is always MD5.
 */
vchar_t *
set_vendorid(int vendorid)
{
	vchar_t vid, *vidhash;

	if (vendorid == VENDORID_UNKNOWN) {
		/*
		 * The default unknown ID gets translated to
		 * KAME/racoon.
		 */
		vendorid = VENDORID_KAME;
	}

	if (vendorid < 0 || vendorid >= NUMVENDORIDS) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "invalid vendor ID index: %d\n", vendorid);
		return (NULL);
	}

	/* XXX Cast away const. */
	vid.v = (char *) vendorid_strings[vendorid];
	vid.l = strlen(vendorid_strings[vendorid]);

	vidhash = eay_md5_one(&vid);
	if (vidhash == NULL)
		plog(LLV_ERROR, LOCATION, NULL,
		    "unable to hash vendor ID string\n");

	return vidhash;
}

/*
 * Check the vendor ID payload -- return the vendor ID index
 * if we find a recognized one, or UNKNOWN if we don't.
 */
int
check_vendorid(gen)
	struct isakmp_gen *gen;		/* points to Vendor ID payload */
{
	vchar_t vid, *vidhash;
	int i, vidlen;

	if (gen == NULL)
		return (VENDORID_UNKNOWN);

	vidlen = ntohs(gen->len) - sizeof(*gen);

	for (i = 0; i < NUMVENDORIDS; i++) {
		/* XXX Cast away const. */
		vid.v = (char *) vendorid_strings[i];
		vid.l = strlen(vendorid_strings[i]);

		vidhash = eay_md5_one(&vid);
		if (vidhash == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "unable to hash vendor ID string\n");
			return (VENDORID_UNKNOWN);
		}

		/*
		 * XXX THIS IS NOT QUITE RIGHT!
		 *
		 * But we need to be able to recognize
		 * Windows 2000's ID, which is the MD5
		 * has of a known string + 4 bytes of
		 * what appears to be version info.
		 */
		if (vidhash->l <= vidlen &&
		    memcmp(vidhash->v, gen + 1, vidhash->l) == 0) {
			plog(LLV_INFO, LOCATION, NULL,
			    "received Vendor ID: %s\n",
			    vendorid_strings[i]);
			vfree(vidhash);
			return (i);
		}
		vfree(vidhash);
	}

	plog(LLV_DEBUG, LOCATION, NULL, "received unknown Vendor ID\n");
	return (VENDORID_UNKNOWN);
}
