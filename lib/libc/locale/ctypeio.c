/*	$NetBSD: ctypeio.c,v 1.1 1997/06/02 09:52:47 kleink Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#define _CTYPE_PRIVATE
#include <ctype.h>
#include "ctypeio.h"

int
__loadctype(name)
	const char *name;
{
	FILE *fp;
	char id[sizeof(_CTYPE_ID) - 1];
	u_int32_t i, len;
	unsigned char *new_ctype = NULL;
	short *new_toupper = NULL, *new_tolower = NULL;

	if ((fp = fopen(name, "r")) == NULL)
		return 0;

	if (fread(id, sizeof(id), 1, fp) != 1)
		goto bad;

	if (memcmp(id, _CTYPE_ID, sizeof(id)) != 0)
		goto bad;

	if (fread(&i, sizeof(u_int32_t), 1, fp) != 1) 
		goto bad;

	if ((i = ntohl(i)) != _CTYPE_REV)
		goto bad;

	if (fread(&len, sizeof(u_int32_t), 1, fp) != 1)
		goto bad;

	if ((len = ntohl(len)) != _CTYPE_NUM_CHARS)
		goto bad;

	if ((new_ctype = malloc(sizeof(u_int8_t) * (1 + len))) == NULL)
		goto bad;

	new_ctype[0] = 0;
	if (fread(&new_ctype[1], sizeof(u_int8_t), len, fp) != len)
		goto bad;

	if ((new_toupper = malloc(sizeof(int16_t) * (1 + len))) == NULL)
		goto bad;

	new_toupper[0] = EOF;
	if (fread(&new_toupper[1], sizeof(int16_t), len, fp) != len)
		goto bad;

	if ((new_tolower = malloc(sizeof(int16_t) * (1 + len))) == NULL)
		goto bad;

	new_tolower[0] = EOF;
	if (fread(&new_tolower[1], sizeof(int16_t), len, fp) != len)
		goto bad;

#if BYTE_ORDER == LITTLE_ENDIAN
	for (i = 1; i <= len; i++) {
		new_toupper[i] = ntohs(new_toupper[i]);
		new_tolower[i] = ntohs(new_tolower[i]);
	}
#endif

	(void) fclose(fp);
	if (_ctype_ != _C_ctype_)
		free((void *) _ctype_);
	_ctype_ = new_ctype;
	if (_toupper_tab_ != _C_toupper_)
		free((void *) _toupper_tab_);
	_toupper_tab_ = new_toupper;
	if (_tolower_tab_ != _C_tolower_)
		free((void *) _tolower_tab_);
	_tolower_tab_ = new_tolower;

	return 1;
bad:
	free(new_tolower);
	free(new_toupper);
	free(new_ctype);
	(void) fclose(fp);
	return 0;
}

int
__savectype(name, new_ctype, new_toupper, new_tolower)
	const char *name;
	unsigned char *new_ctype;
	short *new_toupper, *new_tolower;
{
	FILE *fp;
	u_int32_t i, len = _CTYPE_NUM_CHARS;

	if ((fp = fopen(name, "w")) == NULL)
		return 0;

	if (fwrite(_CTYPE_ID, sizeof(_CTYPE_ID) - 1, 1, fp) != 1)
		goto bad;

	i = htonl(_CTYPE_REV);
	if (fwrite(&i, sizeof(u_int32_t), 1, fp) != 1) 
		goto bad;

	i = htonl(len);
	if (fwrite(&i, sizeof(u_int32_t), 1, fp) != 1)
		goto bad;

	if (fwrite(&new_ctype[1], sizeof(u_int8_t), len, fp) != len)
		goto bad;

#if BYTE_ORDER == LITTLE_ENDIAN
	for (i = 1; i <= len; i++) {
		new_toupper[i] = htons(new_toupper[i]);
		new_tolower[i] = htons(new_tolower[i]);
	}
#endif
	if (fwrite(&new_toupper[1], sizeof(int16_t), len, fp) != len)
		goto bad;

	if (fwrite(&new_tolower[1], sizeof(int16_t), len, fp) != len)
		goto bad;


	(void) fclose(fp);
	return 1;
bad:
	(void) fclose(fp);
	return 0;
}
