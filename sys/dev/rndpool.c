/*      $NetBSD: rndpool.c,v 1.4.2.2 1997/10/14 10:22:11 thorpej Exp $        */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/md5.h>

#include <sys/rnd.h>

/*
 * The random pool "taps"
 */
#define TAP1   13
#define TAP2  113
#define TAP3  230
#define TAP4  412

static rndpool_t _global_rndpool;

static inline void rndpool_add_one_word(rndpool_t *, u_int32_t);

void
rndpool_init(rp)
	rndpool_t *rp;
{
	rp->cursor = RND_POOLWORDS - 1;
	rp->entropy = 0;
	rp->rotate = 0;
}

void
rndpool_init_global(void)
{
	rndpool_init(&_global_rndpool);
}

u_int32_t
rndpool_get_entropy_count(rp)
	rndpool_t *rp;
{
	if (rp == NULL)
		rp = &_global_rndpool;

	return rp->entropy;
}

void
rndpool_set_entropy_count(rp, entropy)
	rndpool_t *rp;
	u_int32_t  entropy;
{
	if (rp == NULL)
		rp = &_global_rndpool;

	rp->entropy = entropy;
	if (rp->entropy > RND_POOLBITS)
		rp->entropy = RND_POOLBITS;
}

void
rndpool_increment_entropy_count(rp, entropy)
	rndpool_t *rp;
	u_int32_t  entropy;
{
	if (rp == NULL)
		rp = &_global_rndpool;

	rp->entropy += entropy;
	if (rp->entropy > RND_POOLBITS)
		rp->entropy = RND_POOLBITS;
}

u_int32_t *
rndpool_get_pool(rp)
	rndpool_t *rp;
{
	if (rp == NULL)
		rp = &_global_rndpool;

	return (rp->pool);
}

u_int32_t
rndpool_get_poolsize(void)
{
	return (RND_POOLWORDS);
}

/*
 * Add one word to the pool, rotating the input as needed.
 */
static inline void
rndpool_add_one_word(rp, val)
	rndpool_t *rp;
	u_int32_t  val;
{
	if (rp == NULL)
		rp = &_global_rndpool;

	/*
	 * Steal some values out of the pool, and xor them into the
	 * word we were given.
	 *
	 * Store the new value into the pool using xor.  This will
	 * prevent the actual values from being known to the caller
	 * since the previous values are assumed to be unknown as well.
	 */
	val ^= rp->pool[(rp->cursor + TAP1) & (RND_POOLWORDS - 1)];
	val ^= rp->pool[(rp->cursor + TAP2) & (RND_POOLWORDS - 1)];
	val ^= rp->pool[(rp->cursor + TAP3) & (RND_POOLWORDS - 1)];
	val ^= rp->pool[(rp->cursor + TAP4) & (RND_POOLWORDS - 1)];
	rp->pool[rp->cursor++] ^=
	  ((val << rp->rotate) | (val >> (31 - rp->rotate)));

	/*
	 * If we have looped around the pool, increment the rotate
	 * variable so the next value will get xored in slightly
	 * rotated.  Increment by a value that is relativly prime to
	 * the word size to try to spread the bits throughout the pool
	 * quickly when the pool is empty.
	 */
	if (rp->cursor == RND_POOLWORDS) {
		rp->cursor = 0;
		rp->rotate = (rp->rotate + 7) & 31;
	}
}

/*
 * Add one byte to the pool.  Update entropy estimate if an estimate
 * was given.
 */
void
rndpool_add_uint32(rp, val, entropy)
	rndpool_t *rp;
	u_int32_t  val;
	u_int32_t  entropy;
{
  	if (rp == NULL)
		rp = &_global_rndpool;

	val = (val << rp->rotate) | (val >> rp->rotate);
	rp->rotate = (rp->rotate + 1) & 0x07;
	
	rndpool_add_one_word(rp, val);
	
	if (entropy) {
		rp->entropy += entropy;
		if (rp->entropy > RND_POOLBITS)
			rp->entropy = RND_POOLBITS;
	}
}

/*
 * add a buffer's worth of data to the pool.
 */
void
rndpool_add_data(rp, p, len, entropy)
	rndpool_t *rp;
	void      *p;
	u_int32_t  len;
	u_int32_t  entropy;
{
	u_int32_t  val;
	u_int8_t  *buf;
	
	if (rp == NULL)
		rp = &_global_rndpool;

	buf = p;
	
	for (; len > 3 ; len -= 4) {
		val = *((u_int32_t *)buf);

		rndpool_add_one_word(rp, val);
		buf += 4;
	}

	val = 0;

	if (len != 0) {
		switch (len) {
		case 3:
			val = *buf++;
		case 2:
			val = val << 8 | *buf++;
		case 1:
			val = val << 8 | *buf++;
		}

		rndpool_add_one_word(rp, val);
	}

	if (entropy) {
		rp->entropy += entropy;
		if (rp->entropy > RND_POOLBITS)
			rp->entropy = RND_POOLBITS;
	}
}

/*
 * Extract some number of bytes from the random pool, decreasing the
 * estimate of randomness as each byte is extracted.
 *
 * Do this by stiring the pool and returning a part of hash as randomness.
 * Note that no secrets are given away here since parts of the hash are
 * xored together before returned.
 *
 * Honor the request from the caller to only return good data, any data,
 * etc.  Note that we must have at least 64 bits of entropy in the pool
 * before we return anything in the high-quality modes.
 */
int
rndpool_extract_data(rp, p, len, mode)
	rndpool_t *rp;
	void      *p;
	u_int32_t  len;
	u_int32_t  mode;
{
	u_int      i;
	MD5_CTX    md5;
	u_int32_t  digest[4];
	u_int32_t  remain;
	u_int8_t  *buf;
	int        good;

	if (rp == NULL)
		rp = &_global_rndpool;

	buf = p;
	remain = len;

	if (mode == RND_EXTRACT_ANY)
		good = 1;
	else
		good = (rp->entropy >= 64);  /* size of hash returned */

	/*
	 * While bytes are requested, stir the pool with a hash function and
	 * copy some of the bytes from that hash out, preserving the secret
	 * hash value itself.
	 */
	while (good && (remain != 0)) {
		MD5Init(&md5);
		MD5Update(&md5, (u_int8_t *)rp->pool, RND_POOLWORDS * 4);
		MD5Final((u_int8_t *)digest, &md5);
    
		/*
		 * Add the hash into the pool.  This helps stir the pool a
		 * bit, and also guarantees that the next hash will generate
		 * a different value if no new values were added to the
		 * pool.
		 */
		for (i = 0 ; i < 4 ; i++)
			rndpool_add_one_word(rp, digest[i]);
		
		/*
		 * copy out the bytes, but xor two bytes from the hash
		 * together before returning them.  This allows us (with
		 * MD5 as the hash function) to return 8 bytes per hash
		 * call and not give out any information to the caller.
		 */
		digest[0] ^= digest[2];
		digest[1] ^= digest[3];

		if (remain < 8) {
			bcopy(digest, buf, remain);
			buf += remain;
			if (rp->entropy >= remain * 8)
				rp->entropy -= remain * 8;
			else
				rp->entropy = 0;
			remain = 0;
		} else {
			bcopy(digest, buf, 8);
			buf += 8;
			remain -= 8;
			if (rp->entropy >= 64)
				rp->entropy -= 64;
			else
				rp->entropy = 0;
		}

		if (mode == RND_EXTRACT_GOOD)
			good = (rp->entropy >= 64);
	}
	
	bzero(&md5, sizeof(MD5_CTX));
	bzero(digest, sizeof(digest));

	return (len - remain);
}
