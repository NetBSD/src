/*	$NetBSD: rtc.c,v 1.1 2003/10/06 18:15:08 shige Exp $	*/
/*	Original:	src/sys/arch/acorn26/ioc/rtc.c	*/
/*	Original Tag:	rtc.c,v 1.7 2003/09/30 00:35:30 thorpej Exp	*/

/*
 * Copyright (c) 2000 Ben Harris
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.1 2003/10/06 18:15:08 shige Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <dev/clock_subr.h>

todr_chip_handle_t todr_handle;

void
todr_attach(todr_chip_handle_t todr)
{

	if (todr_handle)
		panic("todr_attach: rtc already configured");
	todr_handle = todr;
}

void
inittodr(time_t base)
{
	int check;
	struct timeval todrtime;

	check = 0;
	if (todr_handle == NULL) {
		printf("inittodr: rtc not present");
		time.tv_sec = base;
		time.tv_usec = 0;
		check = 1;
	} else {
		if (todr_gettime(todr_handle, &todrtime) != 0) {
			printf("inittodr: Error reading clock");
			time.tv_sec = base;
			time.tv_usec = 0;
			check = 1;
		} else {
			time = todrtime;
			if (time.tv_sec > base + 3 * SECDAY) {
				printf("inittodr: Clock has gained %ld days",
				       (time.tv_sec - base) / SECDAY);
				check = 1;
			} else if (time.tv_sec + SECDAY < base) {
				printf("inittodr: Clock has lost %ld day(s)",
				       (base - time.tv_sec) / SECDAY);
				check = 1;
			}
		}
	}
	if (check)
		printf(" - CHECK AND RESET THE DATE.\n");
}

void
resettodr(void)
{

	if (time.tv_sec == 0)
		return;

	if (todr_handle != NULL &&
	    todr_settime(todr_handle, (struct timeval *)&time) != 0)
		printf("resettodr: failed to set time\n");
}
