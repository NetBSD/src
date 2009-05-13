/*	$NetBSD: selectiveencode.h,v 1.1.1.1.2.2 2009/05/13 18:50:24 jym Exp $	*/

/* Id: selectiveencode.h,v 1.1.1.1 2003/06/04 00:27:08 marka Exp */
/*
 * Copyright (c) 2000,2002 Japan Network Information Center.
 * All rights reserved.
 *  
 * By using this file, you agree to the terms and conditions set forth bellow.
 * 
 * 			LICENSE TERMS AND CONDITIONS 
 * 
 * The following License Terms and Conditions apply, unless a different
 * license is obtained from Japan Network Information Center ("JPNIC"),
 * a Japanese association, Kokusai-Kougyou-Kanda Bldg 6F, 2-3-4 Uchi-Kanda,
 * Chiyoda-ku, Tokyo 101-0047, Japan.
 * 
 * 1. Use, Modification and Redistribution (including distribution of any
 *    modified or derived work) in source and/or binary forms is permitted
 *    under this License Terms and Conditions.
 * 
 * 2. Redistribution of source code must retain the copyright notices as they
 *    appear in each source code file, this License Terms and Conditions.
 * 
 * 3. Redistribution in binary form must reproduce the Copyright Notice,
 *    this License Terms and Conditions, in the documentation and/or other
 *    materials provided with the distribution.  For the purposes of binary
 *    distribution the "Copyright Notice" refers to the following language:
 *    "Copyright (c) 2000-2002 Japan Network Information Center.  All rights reserved."
 * 
 * 4. The name of JPNIC may not be used to endorse or promote products
 *    derived from this Software without specific prior written approval of
 *    JPNIC.
 * 
 * 5. Disclaimer/Limitation of Liability: THIS SOFTWARE IS PROVIDED BY JPNIC
 *    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *    PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL JPNIC BE LIABLE
 *    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef IDN_SELECTIVENCODE_H
#define IDN_SELECTIVENCODE_H 1

/*
 * Find where to convert.
 *
 * Find a substring for which IDN domain name codeset conversion is necessary.
 */

#include <idn/result.h>

/*
 * Scan the given string 's' (in UTF-8 encoding) looking for a substring
 * made of
 *   + at least one non-ascii character and
 *   + zero or more legitimate domain-name characters (i.e. ascii alphabets,
 *     digits and hyphens) and periods.
 *
 * Returns:
 *	idn_success		-- ok, found.
 *	idn_notfound		-- not found.
 */
extern idn_result_t
idn_selectiveencode_findregion(const char *s, char **startp, char **endp);

#endif /* IDN_SELECTIVENCODE_H */
