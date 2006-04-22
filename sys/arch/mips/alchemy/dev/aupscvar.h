/* $NetBSD: aupscvar.h,v 1.2.6.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MIPS_ALCHEMY_DEV_AUPSCVAR_H_
#define	_MIPS_ALCHEMY_DEV_AUPSCVAR_H_

struct aupsc_controller {
	bus_space_tag_t		psc_bust;	/* Bus space tag */
	bus_space_handle_t	psc_bush;	/* Bus space handle */
	int *			psc_sel;	/* current protocol selection */
	void			(*psc_enable)(void *, int);
	void			(*psc_disable)(void *);
	void			(*psc_suspend)(void *);
};

struct aupsc_attach_args {
	const char *		aupsc_name;
	struct aupsc_controller	aupsc_ctrl;
};

struct aupsc_protocol_device {
	struct device		sc_dev;
	struct aupsc_controller	sc_ctrl;
};

#endif	/* _MIPS_ALCHEMY_DEV_AUPSCVAR_H_ */
