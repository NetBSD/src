/* $NetBSD: pckbcvar.h,v 1.1 1998/03/22 15:25:15 drochner Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
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
 *
 */

#ifndef _DEV_ISA_PCKBCVAR_H_
#define _DEV_ISA_PCKBCVAR_H_

typedef void *pckbc_tag_t;
typedef int pckbc_slot_t;
#define	PCKBC_KBD_SLOT	0
#define	PCKBC_AUX_SLOT	1

struct pckbc_attach_args {
	pckbc_tag_t pa_tag;
	pckbc_slot_t pa_slot;
};

typedef void (*pckbc_inputfcn) __P((void *, int));

void pckbc_set_inputhandler __P((pckbc_tag_t, pckbc_slot_t,
				 pckbc_inputfcn, void *));

void pckbc_flush __P((pckbc_tag_t, pckbc_slot_t));
int pckbc_poll_cmd __P((pckbc_tag_t, pckbc_slot_t, u_char *, int,
			int, u_char *, int));
int pckbc_enqueue_cmd __P((pckbc_tag_t, pckbc_slot_t, u_char *, int,
			   int, int, u_char *));
int pckbc_poll_data __P((pckbc_tag_t, pckbc_slot_t));
void pckbc_set_poll __P((pckbc_tag_t, pckbc_slot_t, int));
int pckbc_xt_translation __P((pckbc_tag_t, pckbc_slot_t, int));
void pckbc_slot_enable __P((pckbc_tag_t, pckbc_slot_t, int));

int pckbc_cnattach __P((bus_space_tag_t, pckbc_slot_t));

/* md hook for use without mi wscons */
int pckbc_machdep_cnattach __P((pckbc_tag_t, pckbc_slot_t));

#endif
