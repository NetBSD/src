/*	$NetBSD: mpaux.c,v 1.1.1.1 2000/09/28 22:10:04 thorpej Exp $	*/

/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * This file contains various auxiliary functions related to multiple
 * precision integers.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

/* from OpenBSD: mpaux.c,v 1.14 2000/09/07 20:27:52 deraadt Exp */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mpaux.c,v 1.1.1.1 2000/09/28 22:10:04 thorpej Exp $");
#endif

#include "includes.h"

#include <openssl/bn.h>
#include "getput.h"
#include "xmalloc.h"

#include <openssl/md5.h>

#include "mpaux.h"

void
compute_session_id(unsigned char session_id[16],
    unsigned char cookie[8],
    BIGNUM* host_key_n,
    BIGNUM* session_key_n)
{
	unsigned int host_key_bytes = BN_num_bytes(host_key_n);
	unsigned int session_key_bytes = BN_num_bytes(session_key_n);
	unsigned int bytes = host_key_bytes + session_key_bytes;
	unsigned char *buf = xmalloc(bytes);
	MD5_CTX md;

	BN_bn2bin(host_key_n, buf);
	BN_bn2bin(session_key_n, buf + host_key_bytes);
	MD5_Init(&md);
	MD5_Update(&md, buf, bytes);
	MD5_Update(&md, cookie, 8);
	MD5_Final(session_id, &md);
	memset(buf, 0, bytes);
	xfree(buf);
}
