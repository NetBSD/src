/*      $NetBSD: rnd_private.h,v 1.4 2013/08/27 19:30:10 riastradh Exp $     */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.  This code uses ideas and
 * algorithms from the Linux driver written by Ted Ts'o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_RNDPRIVATE_H
#define _DEV_RNDPRIVATE_H
/*
 * Number of bytes returned per hash.  This value is used in both
 * rnd.c and rndpool.c to decide when enough entropy exists to do a
 * hash to extract it.
 */
#define RND_ENTROPY_THRESHOLD   10

/*
 * Size of the event queue.  This _MUST_ be a power of 2.
 */
#ifndef RND_EVENTQSIZE
#define RND_EVENTQSIZE  128
#endif

/*
 * Used by rnd_extract_data() and rndpool_extract_data() to describe how
 * "good" the data has to be.
 */
#define RND_EXTRACT_ANY		0  /* extract anything, even if no entropy */
#define RND_EXTRACT_GOOD	1  /* return as many good bytes
				      (short read ok) */

uint32_t        rnd_extract_data(void *, uint32_t, uint32_t);
#endif
