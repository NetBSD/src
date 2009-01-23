/*	$NetBSD: vendorid.h,v 1.6 2009/01/23 08:06:56 tteras Exp $	*/

/* Id: vendorid.h,v 1.11 2006/02/17 14:09:10 vanhu Exp */

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

#ifndef _VENDORID_H
#define _VENDORID_H

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

/* The unknown vendor ID. */
#define VENDORID_UNKNOWN	-1

/* Our default vendor ID. */
#define VENDORID_DEFAULT	VENDORID_IPSEC_TOOLS

#define VENDORID_IPSEC_TOOLS	0

/* Refer to draft-ietf-ipsec-isakmp-gss-auth-06.txt. */
#define VENDORID_GSSAPI_LONG	1
#define VENDORID_GSSAPI		2
#define VENDORID_MS_NT5		3

#define VENDORID_GSSAPI_MASK	(BIT(VENDORID_GSSAPI_LONG) | \
				 BIT(VENDORID_GSSAPI) | \
				 BIT(VENDORID_MS_NT5))

/* NAT-T support */
#define VENDORID_NATT_00	4
#define VENDORID_NATT_01	5
#define VENDORID_NATT_02	6
#define VENDORID_NATT_02_N	7
#define VENDORID_NATT_03	8
#define VENDORID_NATT_04	9
#define VENDORID_NATT_05	10
#define VENDORID_NATT_06	11
#define VENDORID_NATT_07	12
#define VENDORID_NATT_08	13
#define VENDORID_NATT_RFC	14

#define VENDORID_NATT_FIRST	VENDORID_NATT_00
#define VENDORID_NATT_LAST	VENDORID_NATT_RFC

#define MAX_NATT_VID_COUNT	(VENDORID_NATT_LAST - VENDORID_NATT_FIRST + 1)

/* Hybrid auth */
#define VENDORID_XAUTH		15
#define VENDORID_UNITY		16

/* IKE fragmentation */
#define VENDORID_FRAG		17

/* Dead Peer Detection */
#define VENDORID_DPD		18


/* Other Vendors...
 * XXX: do some cleanup to have separate lists for "real" vendors (to complete)
 * and "features" VendorIDs
 */
#define VENDORID_KAME		19

struct vendor_id {
	int		id;
	const char	*string;
	vchar_t		*hash;
};

vchar_t *set_vendorid __P((int));
int handle_vendorid __P((struct ph1handle *, struct isakmp_gen *));

void compute_vendorids __P((void));
const char *vid_string_by_id __P((int id));

#endif /* _VENDORID_H */
