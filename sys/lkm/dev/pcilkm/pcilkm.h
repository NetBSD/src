/*	$NetBSD: pcilkm.h,v 1.3.2.3 2004/09/18 14:54:09 skrll Exp $	*/

/*
 *  Copyright (c) 2004 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PCILKM_H_
#define _PCILKM_H_

struct pcilkm_driver {
	struct lkm_any	*pld_module;
	struct cfattach	*pld_cfa;
	struct cfdriver	*pld_cfd;
	struct cftable	pld_cft;
};

int pcilkm_hostlkmentry(struct lkm_table *, int, int, struct pcilkm_driver *);
extern const struct cfparent	pcilkm_pspec;
extern int			pcilkm_loc[];
extern const char * const	pcilkmcf_locnames[];

#define PCILKM_DECLARE(name, class, attrs)			\
	CFDRIVER_DECL(name, class, attrs);			\
	struct cfdata __CONCAT(name,_cfdata) [] = {		\
		{ __STRING(name), __STRING(name), 0,		\
		FSTATE_STAR, pcilkm_loc, 0,			\
		 &pcilkm_pspec,	pcilkmcf_locnames },		\
		{ 0 } };					\
	struct pcilkm_driver __CONCAT(name,_pld) = {		\
		(struct lkm_any *)& _module,			\
		& __CONCAT(name,_ca),				\
		& __CONCAT(name,_cd),				\
		{ __CONCAT(name,_cfdata) } };			\
	int __CONCAT(name,_lkmentry) (struct lkm_table *,	\
		int, int);					\
	int __CONCAT(name,_lkmentry) (struct lkm_table *lkmtp,	\
		int cmd, int ver)				\
	{ return pcilkm_hostlkmentry(lkmtp, cmd, ver,		\
		& __CONCAT(name,_pld)); }

#endif /* !_PCILKM_H */
