/*
 * Arc4 random number generator for OpenBSD.
 * Copyright 1996 David Mazieres <dm@lcs.mit.edu>.
 *
 * Modification and redistribution in source and binary forms is
 * permitted provided that due credit is given to the author and the
 * OpenBSD project by leaving this copyright notice intact.
 */

/*
 * This code is derived from section 17.1 of Applied Cryptography,
 * second edition, which describes a stream cipher allegedly
 * compatible with RSA Labs "RC4" cipher (the actual description of
 * which is a trade secret).  The same algorithm is used as a stream
 * cipher called "arcfour" in Tatu Ylonen's ssh package.
 *
 * Here the stream cipher has been modified always to include the time
 * when initializing the state.  That makes it impossible to
 * regenerate the same random sequence twice, so this can't be used
 * for encryption, but will generate good random numbers.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

#include <sys/time.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "arc4random.h"

struct arc4_stream {
	uint8_t i;
	uint8_t j;
	uint8_t s[256];
	size_t count;
	pid_t stir_pid;
};

#define S(n) (n)
#define S4(n) S(n), S(n + 1), S(n + 2), S(n + 3)
#define S16(n) S4(n), S4(n + 4), S4(n + 8), S4(n + 12)
#define S64(n) S16(n), S16(n + 16), S16(n + 32), S16(n + 48)
#define S256 S64(0), S64(64), S64(128), S64(192)

static struct arc4_stream rs = { .i = 0xff, .j = 0, .s = { S256 },
                    .count = 0, .stir_pid = 0 };

#undef S
#undef S4
#undef S16
#undef S64
#undef S256

static void
arc4_addrandom(struct arc4_stream *as, unsigned char *dat, int datlen)
{
	int n;
	uint8_t si;

	as->i--;
	for (n = 0; n < 256; n++) {
		as->i = (uint8_t)(as->i + 1);
		si = as->s[as->i];
		as->j = (uint8_t)(as->j + si + dat[n % datlen]);
		as->s[as->i] = as->s[as->j];
		as->s[as->j] = si;
	}
	as->j = as->i;
}

static uint8_t
arc4_getbyte(struct arc4_stream *as)
{
	uint8_t si, sj;

	as->i = (uint8_t)(as->i + 1);
	si = as->s[as->i];
	as->j = (uint8_t)(as->j + si);
	sj = as->s[as->j];
	as->s[as->i] = sj;
	as->s[as->j] = si;
	return (as->s[(si + sj) & 0xff]);
}

static uint32_t
arc4_getword(struct arc4_stream *as)
{
	int val;

	val = arc4_getbyte(as) << 24;
	val |= arc4_getbyte(as) << 16;
	val |= arc4_getbyte(as) << 8;
	val |= arc4_getbyte(as);
	return (uint32_t)val;
}

/* We don't care about any error on read, just use what we have
 * on the stack. So mask off this GCC warning. */
#pragma GCC diagnostic ignored "-Wunused-result"
static void
arc4_stir(struct arc4_stream *as)
{
	int fd;
	struct {
		struct timeval tv;
		unsigned int rnd[(128 - sizeof(struct timeval)) /
			sizeof(unsigned int)];
	}       rdat;
	size_t n;

	gettimeofday(&rdat.tv, NULL);
	fd = open("/dev/urandom", O_RDONLY);
	if (fd != -1) {
		/* If there is an error reading, just use what is
		 * on the stack. */
		/* coverity[check_return] */
		(void)read(fd, rdat.rnd, sizeof(rdat.rnd));
		close(fd);
	}

	/* fd < 0?  Ah, what the heck. We'll just take
	 * whatever was on the stack... */
	/* coverity[uninit_use_in_call] */
	arc4_addrandom(as, (void *) &rdat, sizeof(rdat));

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  (N = 256 in our case.)
	 */
	for (n = 0; n < 256 * sizeof(uint32_t); n++)
		arc4_getbyte(as);
	as->count = 1600000;
}

static void
arc4_stir_if_needed(struct arc4_stream *as)
{
	pid_t pid;

	pid = getpid();
	if (as->count <= sizeof(uint32_t) || as->stir_pid != pid) {
		as->stir_pid = pid;
		arc4_stir(as);
	} else
		as->count -= sizeof(uint32_t);
}

uint32_t
arc4random()
{

	arc4_stir_if_needed(&rs);
	return arc4_getword(&rs);
}
