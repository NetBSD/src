/*	$OpenBSD: tcp_subr.c,v 1.98 2007/06/25 12:17:43 markus Exp $	*/
/*	$NetBSD: tcp_rndiss.c,v 1.2.34.1 2012/04/17 00:08:15 yamt Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)COPYRIGHT	1.1 (NRL) 17 January 1995
 *
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 	This product includes software developed at the Information
 * 	Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcp_rndiss.c,v 1.2.34.1 2012/04/17 00:08:15 yamt Exp $");

#include <sys/param.h>
#include <sys/cprng.h>

#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_rndiss.h>

#define TCP_RNDISS_ROUNDS	16
#define TCP_RNDISS_OUT		7200
#define TCP_RNDISS_MAX		30000

u_int8_t tcp_rndiss_sbox[128];
u_int16_t tcp_rndiss_msb;
u_int16_t tcp_rndiss_cnt;
long tcp_rndiss_reseed;

u_int16_t
tcp_rndiss_encrypt(u_int16_t val)
{
	u_int16_t sum = 0, i;

	for (i = 0; i < TCP_RNDISS_ROUNDS; i++) {
		sum += 0x79b9;
		val ^= ((u_int16_t)tcp_rndiss_sbox[(val^sum) & 0x7f]) << 7;
		val = ((val & 0xff) << 7) | (val >> 8);
	}

	return val;
}

void
tcp_rndiss_init(void)
{
	cprng_strong(kern_cprng, tcp_rndiss_sbox, sizeof(tcp_rndiss_sbox), 0);

	tcp_rndiss_reseed = time_second + TCP_RNDISS_OUT;
	tcp_rndiss_msb = tcp_rndiss_msb == 0x8000 ? 0 : 0x8000;
	tcp_rndiss_cnt = 0;
}

tcp_seq
tcp_rndiss_next(void)
{
        if (tcp_rndiss_cnt >= TCP_RNDISS_MAX ||
	    time_second > tcp_rndiss_reseed)
		tcp_rndiss_init();

	/* (arc4random() & 0x7fff) ensures a 32768 byte gap between ISS */
	return ((tcp_rndiss_encrypt(tcp_rndiss_cnt++) | tcp_rndiss_msb) <<16) |
		(cprng_fast32() & 0x7fff);
}
