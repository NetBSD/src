/*	$NetBSD: isakmp_frag.h,v 1.5 2006/09/18 20:32:40 manu Exp $	*/

/*	Id: isakmp_frag.h,v 1.3 2005/04/09 16:25:24 manubsd Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus 
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

/* These are the values from parsing "remote {}"
   block of the config file. */
#define ISAKMP_FRAG_OFF		FLASE   /* = 0 */
#define ISAKMP_FRAG_ON		TRUE    /* = 1 */
#define ISAKMP_FRAG_FORCE	2

/* IKE fragmentation capabilities */
#define VENDORID_FRAG_IDENT 	0x80000000
#define VENDORID_FRAG_BASE 	0x40000000
#define VENDORID_FRAG_AGG 	0x80000000

#define ISAKMP_FRAG_MAXLEN 552

struct isakmp_frag_item {
	int	frag_num;
	int	frag_last;
	struct isakmp_frag_item *frag_next;
	vchar_t *frag_packet;
};

int isakmp_sendfrags(struct ph1handle *, vchar_t *);
unsigned int vendorid_frag_cap(struct isakmp_gen *);
int isakmp_frag_extract(struct ph1handle *, vchar_t *);
vchar_t *isakmp_frag_reassembly(struct ph1handle *);
vchar_t *isakmp_frag_addcap(vchar_t *, int);
