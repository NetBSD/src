/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#if ! defined (_CC_VBL_H)
#define _CC_VBL_H

#include "cc.h"

#if ! defined (CC_VBL_C)
/* structure for a vbl_function. */
struct vbl_node {
    dll_node_t resv0;				  /* Private. */
    short   resv1;				  /* Private. */
    short   resv2;				  /* Private. */
    void  (*function)(void *);			  /* put your function pointer here. */
    void   *resv3;	
};
#endif /* CC_MISC_C */

void cc_init_vbl (void);

#if defined (AMIGA_TEST)
void cc_deinit_vbl (void);
#endif

/* function only returns after it has added the vbl function. */
void add_vbl_function (struct vbl_node *n, short priority, void *data);

/* this functon returns immediatly. */
void remove_vbl_function (struct vbl_node *n);

/* this function returns when the function has actually been removed. */
void wait_for_vbl_function_removal (struct vbl_node *n);

/* this function returns when vbl function is off. */
void turn_vbl_function_off (struct vbl_node *n);

/* this function returns immediatley. */
void turn_vbl_function_on (struct vbl_node *n);

#endif /* _CC_MISC_H */
