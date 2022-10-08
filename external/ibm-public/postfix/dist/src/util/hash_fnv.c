/*	$NetBSD: hash_fnv.c,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

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
/* DESCRIPTION
/*	hash_fnv() implements a modified FNV type 1a hash function.
/*
/*	To thwart collision attacks, the hash function is seeded
/*	once from /dev/urandom, and if that is unavailable, from
/*	wallclock time, monotonic system clocks, and the process
/*	ID. To disable seeding (typically, for regression tests),
/*	specify the NORANDOMIZE environment variable; the value
/*	does not matter.
/*
/*	This function implements a workaround for a "sticky state"
/*	problem with FNV hash functions: when an input produces a
/*	zero intermediate hash state, and the next input byte is
/*	zero, then the operations "hash ^= 0" and "hash *= FNV_prime"
/*	would not change the hash value. To avoid this, hash_fnv()
/*	adds 1 to each input byte. Compile with -DSTRICT_FNV1A to
/*	get the standard behavior.
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

/* hash_fnv - modified FNV 1a hash */

HASH_FNV_T hash_fnv(const void *src, size_t len)
{
    static HASH_FNV_T basis = FNV_offset_basis;
    static int randomize = 1;
    HASH_FNV_T hash;

    /*
     * Initialize.
     */
    if (randomize) {
	if (!getenv("NORANDOMIZE")) {
	    HASH_FNV_T seed;

	    ldseed(&seed, sizeof(seed));
	    basis ^= seed;
	}
	randomize = 0;
    }

#ifdef STRICT_FNV1A
#define FNV_NEXT_BYTE(s) ((HASH_FNV_T) * (const unsigned char *) s++)
#else
#define FNV_NEXT_BYTE(s) (1 + (HASH_FNV_T) * (const unsigned char *) s++)
#endif

    hash = basis;
    while (len-- > 0) {
	hash ^= FNV_NEXT_BYTE(src);
	hash *= FNV_prime;
    }
    return (hash);
}
