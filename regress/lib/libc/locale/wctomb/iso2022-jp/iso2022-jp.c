/*	$Id: iso2022-jp.c,v 1.2 2004/09/26 03:50:16 yamt Exp $	*/

/*-
 * Copyright (c)2004 Citrus Project,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

const char teststring[] =
	"\x1b$B"	/* JIS X 0208-1983 */
	"\x46\x7c\x4b\x5c\x38\x6c" /* "nihongo" */
	"\x1b(B"	/* ISO 646 */
	"ABC"
	"\x1b(I"	/* JIS X 0201 katakana */
	"\xb1\xb2\xb3"	/* "aiu" */
	"\x1b(B"	/* ISO 646 */
	;
const int teststring_wclen = 3 + 3 + 3;
const char teststring_loc[] = "ja_JP.ISO2022-JP";
const int teststring_mblen[] = { 3+2, 2, 2, 3+1, 1, 1, 3+1, 1, 1, 3+1 };

#include "test.c"
