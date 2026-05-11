/*	$NetBSD: hash_fnv.c,v 1.4.2.1 2026/05/11 17:14:02 martin Exp $	*/

/*++
/* NAME
/*	hash_fnv 3
/* SUMMARY
/*	Fowler/Noll/Vo hash function
/* SYNOPSIS
/*	#include <hash_fnv.h>
/*
/*	HASH_FNV_T hash_fnv(
/*	const void *src,
/*	size_t	len)
/*
/*	HASH_FNV_T hash_fnvz(
/*	const char *src)
/* DESCRIPTION
/*	hash_fnv() implements a modified FNV type 1a hash function.
/*
/*	hash_fnvz() provides the same functionality for null-terminated
/*	strings, avoiding an unnecessary strlen() call.
/*
/*	To thwart collision attacks, the hash function is seeded
/*	once with ldseed(). To disable seeding (typically, to make
/*	tests predictable), specify the NORANDOMIZE environment
/*	variable; the value does not matter.
/*
/*	This implementation works around a "sticky state" problem
/*	with FNV hash functions: when an input produces a zero hash
/*	state, and the next input byte is zero, then the hash state
/*	would not change. To avoid this, hash_fnv() adds 1 to each
/*	input value. Compile with -DSTRICT_FNV1A to get the standard
/*	behavior.
/*
/*	The default HASH_FNV_T result type is uint64_t. When compiled
/*	with -DUSE_FNV_32BIT, the result type is uint32_t. On ancient
/*	systems without <stdint.h>, define HASH_FNV_T on the compiler
/*	command line as an unsigned 32-bit or 64-bit integer type,
/*	and specify -DUSE_FNV_32BIT when HASH_FNV_T is a 32-bit type.
/* SEE ALSO
/*	http://www.isthe.com/chongo/tech/comp/fnv/index.html
/*	https://softwareengineering.stackexchange.com/questions/49550/
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <ldseed.h>
#include <hash_fnv.h>

 /*
  * Application-specific.
  */
#ifdef USE_FNV_32BIT
#define FNV_prime 		0x01000193UL
#define FNV_offset_basis	0x811c9dc5UL
#else
#define FNV_prime		0x00000100000001B3ULL
#define FNV_offset_basis	0xcbf29ce484222325ULL
#endif

 /*
  * Workaround for the sticky all-zero hash state: when the next input byte
  * is zero, then the operations "hash ^= 0" and "hash *= FNV_prime" would
  * not change the hash state. To avoid that, add 1 to the every input value.
  */
#ifdef STRICT_FNV1A
#define HASH_FNV_NEW_BITS(new_bits) (new_bits)
#else
#define HASH_FNV_NEW_BITS(new_bits) (1 + (new_bits))
#endif

static HASH_FNV_T hash_fnv_basis = FNV_offset_basis;
static int hash_fnv_must_init = 1;

/* hash_fnv_init - seed the hash */

static void hash_fnv_init(void)
{
    HASH_FNV_T seed;

    if (!getenv("NORANDOMIZE")) {
	ldseed(&seed, sizeof(seed));
	hash_fnv_basis ^= seed;
    }
    hash_fnv_must_init = 0;
}

/* hash_fnv - modified FNV 1a hash */

HASH_FNV_T hash_fnv(const void *src, size_t len)
{
    HASH_FNV_T hash;
    HASH_FNV_T new_bits;

    if (hash_fnv_must_init)
	hash_fnv_init();

    hash = hash_fnv_basis;
    while (len-- > 0) {
	new_bits = *(unsigned char *) src++;
	hash ^= HASH_FNV_NEW_BITS(new_bits);
	hash *= FNV_prime;
    }
    return (hash);
}

/* hash_fnvz - modified FNV 1a hash for null-terminated strings */

HASH_FNV_T hash_fnvz(const char *src)
{
    HASH_FNV_T hash;
    HASH_FNV_T new_bits;

    if (hash_fnv_must_init)
	hash_fnv_init();

    hash = hash_fnv_basis;
    while ((new_bits = *(unsigned char *) src++) != 0) {
	hash ^= HASH_FNV_NEW_BITS(new_bits);
	hash *= FNV_prime;
    }
    return (hash);
}
