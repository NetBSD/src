/*	$NetBSD: clockvar.h,v 1.1.4.2 2002/02/11 20:08:59 jdolecek Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct clockfns {
	void (*cf_init)(struct device *);
	void (*cf_get)(struct device *, struct clock_ymdhms *);
	void (*cf_set)(struct device *, struct clock_ymdhms *);
};

void clockattach(struct device *, const struct clockfns *);

#define IRIX_CLOCK_BASE 1940

/*
 * If year < 1985, store (year - 1970), else (year - 1940). This
 * matches IRIX semantics.
 */
#define TO_IRIX_YEAR(a)	((a) < 1985) ? ((a) - (30 + IRIX_CLOCK_BASE))	\
    : ((a) - IRIX_CLOCK_BASE)

/* RTC base on IRIX is 1940, offsets < 45 are from 1970 */
#define FROM_IRIX_YEAR(a) ((a) < 45) ? ((a) + 30 + IRIX_CLOCK_BASE)	\
    : ((a) + IRIX_CLOCK_BASE)
