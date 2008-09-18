/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "random.h"
#include "log.h"

int32_t _a[56];
int32_t *_r;

static inline int32_t _mod_diff(int32_t x, int32_t y)
{
	return (x - y) & 0x7fffffff;
}

static int32_t _flip_cycle(void)
{
	int32_t *ii, *jj;
	for (ii = _a + 1, jj = _a + 32; jj <= _a + 55; ii++, jj++)
		*ii = _mod_diff(*ii, *jj);

	for (jj = _a + 1; ii <= _a + 55; ii++, jj++)
		*ii = _mod_diff(*ii, *jj);

	_r = _a + 54;
	return _a[55];
}

static void rand_init(int32_t seed)
{
	int64_t i;
	int64_t prev = seed, next = 1;

	seed = prev = _mod_diff(prev, 0); /* strip the sign */
	_a[55] = prev;
	for (i = 21; i; i = (i + 21) % 55) {
		_a[i] = next;
		next = _mod_diff(prev, next);
		if(seed & 1)
			seed = 0x40000000L + (seed >> 1);
		else
			seed >>= 1;

		next = _mod_diff(next, seed);
		prev = _a[i];
	}

	_flip_cycle();
	_flip_cycle();
	_flip_cycle();
	_flip_cycle();
	_flip_cycle();
}

/*
 * FIXME: move this to be an inline in the
 * header.
 */
int32_t rand_get(void)
{
	return  (*_r >= 0) ? *_r-- : _flip_cycle();
}


/*
 * just used by rand_check
 */
#define t31 0x80000000
static int32_t _uniform(int32_t m)
{
	uint32_t t =  t31 - (t31 % m);
	int32_t r;

	do
		r = next_rand(sc);

	while (t <= (uint32_t) r);

	return r % m;
}

/*
 * Checks I've copied the code correctly.
 */
int rand_check(void)
{
	int j;

	rand_init(-314159L);

	if (next_rand(sc) != 119318998)	{
		log_err("Random number generator failed check 1");
		return 0;
	}

	for(j = 1; j <= 133; j++)
		rand_get();

	if (_uniform(0x55555555L) != 748103812) {
		log_err("Random number generator failed check 2");
		return 0;
	}

	return 1;
}
