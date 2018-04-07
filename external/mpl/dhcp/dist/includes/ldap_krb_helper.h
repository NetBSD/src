/*	$NetBSD: ldap_krb_helper.h,v 1.1.1.1 2018/04/07 22:34:26 christos Exp $	*/

/* ldap_krb_helper.h

   Helper routings for allowing LDAP to read configuration with GSSAPI/krb auth */

/*
 * Copyright (c) 2015 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2014 William B.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This helper was written by William Brown <william@adelaide.edu.au>, 
 * inspired by krb5_helper.c from bind-dyndb-ldap by Simo Sorce (Redhat)
 */
#ifndef LDAP_KRB5_HELPER
#define LDAP_KRB5_HELPER

#if defined(LDAP_USE_GSSAPI)
#include <krb5.h>

extern isc_result_t krb5_get_tgt(const char *, const char *);
#endif

#endif /* LDAP_KRB5_HELPER */
