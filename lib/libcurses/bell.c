/*	$NetBSD: bell.c,v 1.1 1999/04/13 14:08:17 mrg Exp $	*/

/*
 * Copyright (c) 1999 Julian. D. Coleman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "curses.h"

/*
 * beep
 *	Ring the terminal bell
 */
int
beep(void)
{
	if (BL != NULL) {
#ifdef DEBUG
		__CTRACE("beep: bl\n");
#endif
		tputs(BL, 0, __cputchar);
	} else if (VB != NULL) {
#ifdef DEBUG
		__CTRACE("beep: vb\n");
#endif
		tputs(VB, 0, __cputchar);
	}
	return (1);
}

/*
 * flash
 *	Flash the terminal screen
 */
int
flash(void)
{
	if (VB != NULL) {
#ifdef DEBUG
		__CTRACE("flash: vb\n");
#endif
		tputs(VB, 0, __cputchar);
	} else if (BL != NULL) {
#ifdef DEBUG
		__CTRACE("flash: bl\n");
#endif
		tputs(BL, 0, __cputchar);
	}
	return (1);
}
