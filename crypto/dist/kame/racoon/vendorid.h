/*	$KAME: vendorid.h,v 1.6 2001/03/27 02:39:58 thorpej Exp $	*/

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

/* The unknown vendor ID. */
#define	VENDORID_UNKNOWN	-1

/* Our default vendor ID. */
#define	VENDORID_KAME		0

/*
 * Refer to draft-ietf-ipsec-isakmp-gss-auth-06.txt.
 */
#define	VENDORID_GSSAPI_LONG	1
#define	VENDORID_GSSAPI		2
#define	VENDORID_MS_NT5		3
#define	VENDOR_SUPPORTS_GSSAPI(x)					\
	((x) == VENDORID_GSSAPI_LONG ||					\
	 (x) == VENDORID_GSSAPI ||					\
	 (x) == VENDORID_MS_NT5)

#define	NUMVENDORIDS		4

#define	VENDORID_STRINGS						\
{									\
	"KAME/racoon",							\
	"A GSS-API Authentication Method for IKE",			\
	"GSSAPI",							\
	"MS NT5 ISAKMPOAKLEY",						\
}

extern const char *vendorid_strings[];

vchar_t *set_vendorid __P((int));
int check_vendorid __P((struct isakmp_gen *));
