/*	$NetBSD: romvec.h,v 1.1.6.2 2013/02/25 00:28:49 tls Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)romvec.h	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1992 OMRON Corporation.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)romvec.h	8.1 (Berkeley) 6/10/93
 */

/* romvec.h Oct-22-1991 */


#define RVPtr	((struct romvec *) 0x41000000)

#define ROM_memsize	(*((int *) *RVPtr->vec03))
#define	ROM_getchar	(*RVPtr->vec06)
#define	ROM_putchar	(*RVPtr->vec07)
#define	ROM_abort	(*RVPtr->vec25)
#define ROM_plane	(*((int *) *RVPtr->vec46))

struct romvec {
    int     (*vec00)(void); /* 00 [00] - Cold Boot Entry */
    int     (*vec01)(void); /* 01 [04] */
    int     (*vec02)(void); /* 02 [08] */
    int     (*vec03)(void); /* 03 [0C] - memsize : Memory Size */
    int     (*vec04)(void); /* 04 [10] */
    int     (*vec05)(void); /* 05 [14] */
    int     (*vec06)(void); /* 06 [18] - getchar : get 1 charactor from console	*/
    int     (*vec07)(int);  /* 07 [1C] - putchar : put 1 charactor to console		*/
    int     (*vec08)(void); /* 08 [20] */
    int     (*vec09)(void); /* 09 [24] */
    int     (*vec10)(void); /* 10 [28] */
    int     (*vec11)(void); /* 11 [2C] */
    int     (*vec12)(void); /* 12 [30] */
    int     (*vec13)(void); /* 13 [34] */
    int     (*vec14)(void); /* 14 [38] */
    int     (*vec15)(void); /* 15 [3C] */
    int     (*vec16)(void); /* 16 [40] */
    int     (*vec17)(void); /* 17 [44] */
    int     (*vec18)(void); /* 18 [48] */
    int     (*vec19)(void); /* 19 [4C] */
    int     (*vec20)(void); /* 20 [50] */
    int     (*vec21)(void); /* 21 [54] */
    int     (*vec22)(void); /* 22 [58] */
    int     (*vec23)(void); /* 23 [5C] */
    int     (*vec24)(void); /* 24 [60] */
    int     (*vec25)(void); /* 25 [64] - abort : back to ROM Monitor */
    int     (*vec26)(void); /* 26 [68] */
    int     (*vec27)(void); /* 27 [6C] */
    int     (*vec28)(void); /* 28 [70] */
    int     (*vec29)(void); /* 29 [74] */
    int     (*vec30)(void); /* 30 [78] */
    int     (*vec31)(void); /* 31 [7C] */
    int     (*vec32)(void); /* 32 [80] */
    int     (*vec33)(void); /* 33 [84] */
    int     (*vec34)(void); /* 34 [88] */
    int     (*vec35)(void); /* 35 [8C] */
    int     (*vec36)(void); /* 36 [90] */
    int     (*vec37)(void); /* 37 [94] */
    int     (*vec38)(void); /* 38 [98] */
    int     (*vec39)(void); /* 39 [9C] */
    int     (*vec40)(void); /* 40 [A0] */
    int     (*vec41)(void); /* 41 [A4] */
    int     (*vec42)(void); /* 42 [A8] */
    int     (*vec43)(void); /* 43 [AC] */
    int     (*vec44)(void); /* 44 [B0] */
    int     (*vec45)(void); /* 45 [B4] */
    int     (*vec46)(void); /* 46 [B8] -- number of plane */
    int     (*vec47)(void); /* 47 [BC] */
    int     (*vec48)(void); /* 48 [C0] */
    int     (*vec49)(void); /* 49 [C4] */
    int     (*vec50)(void); /* 50 [C8] */
    int     (*vec51)(void); /* 51 [CC] */
    int     (*vec52)(void); /* 52 [D0] */
    int     (*vec53)(void); /* 53 [D4] */
    int     (*vec54)(void); /* 54 [D8] */
    int     (*vec55)(void); /* 55 [DC] */
    int     (*vec56)(void); /* 56 [E0] */
    int     (*vec57)(void); /* 57 [E4] */
    int     (*vec58)(void); /* 58 [E8] */
    int     (*vec59)(void); /* 59 [EC] */
    int     (*vec60)(void); /* 60 [F0] */
    int     (*vec61)(void); /* 61 [F4] */
    int     (*vec62)(void); /* 62 [F8] */
    int     (*vec63)(void); /* 63 [FC] */
};


