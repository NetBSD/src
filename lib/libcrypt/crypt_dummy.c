/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tom Truscott.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
/*static char *sccsid = "from: @(#)crypt.c	5.11 (Berkeley) 6/25/91";*/
static char *rcsid = "$Id: crypt_dummy.c,v 1.2 1993/10/07 01:43:14 cgd Exp $";
#endif /* LIBC_SCCS and not lint */

#include <unistd.h>
#include <stdio.h>

/*
 * UNIX password, and DES, encryption.
 *
 * since this is non-exportable, this is just a dummy.  if you want
 * real encryption, make sure you've got libcrypt.a around.
 */

static char     cryptresult[1+4+4+11+1];        /* "encrypted" result */

char *
crypt(key, setting)
	register const char *key;
	register const char *setting;
{
	static int warned;

	if (!warned) {
		fprintf(stderr, "WARNING!  crypt(3) not present in the system!\n");
		warned = 1;
	}
	strncpy(cryptresult, key, sizeof cryptresult);
	cryptresult[sizeof cryptresult - 1] = '\0';
	return(cryptresult);
}


des_setkey(key)
	register const char *key;
{
	static int warned;

	if (!warned) {
        	fprintf(stderr, "WARNING!  des_setkey(3) not present in the system!\n");
		warned = 1;
	}
	return(0);
}

des_cipher(in, out, salt, num_iter)
	const char *in;
	char *out;
	long salt;
	int num_iter;
{
	static int warned;

	if (!warned) {
        	fprintf(stderr, "WARNING!  des_cipher(3) not present in the system!\n");
		warned = 1;
	}
	bcopy(in, out, 8);
	return(0);
}

setkey(key)
	register const char *key;
{
	static int warned;

	if (!warned) {
        	fprintf(stderr, "WARNING!  setkey(3) not present in the system!\n");
		warned = 1;
	}
	return(0);
}

encrypt(block, flag)
	register char *block;
	int flag;
{
	static int warned;

	if (!warned) {
	        fprintf(stderr, "WARNING!  encrypt(3) not present in the system!\n");
		warned = 1;
	}
	return(0);
}

