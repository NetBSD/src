/*	$NetBSD: hash_fnv.c,v 1.2.2.1 2023/12/25 12:43:37 martin Exp $	*/

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

#ifdef TEST
#include <stdlib.h>
#include <string.h>
#include <msg.h>

int     main(void)
{
    int     pass = 0;
    int     fail = 0;

    /*
     * Sanity check.
     */
#ifdef STRICT_FNV1A
    msg_fatal("This test requires no STRICT_FNV1A");
#endif

    /*
     * Force unseeded hash, to make tests predictable.
     */
    if (putenv("NORANDOMIZE=") != 0)
	msg_fatal("putenv(\"NORANDOMIZE=\"): %m");

    /*
     * Test: hashing produces the expected results.
     */
    {
	struct testcase {
	    HASH_FNV_T hval;
	    const char *str;
	};
	static struct testcase testcases[] =
	{
#ifdef USE_FNV_32BIT
	    0x1c00fc06UL, "overdeeply",
	    0x1c00fc06UL, "undescript",
	    0x1e1e52a4UL, "fanfold",
	    0x1e1e52a4UL, "phrensied",
#else
	    0xda19999ec0bda706ULL, "overdeeply",
	    0xd7b9e43f26396a66ULL, "undescript",
	    0xa50c585d385a2604ULL, "fanfold",
	    0x1ec3ef9bb2b734a4ULL, "phrensied",
#endif
	    0,
	};
	struct testcase *tp;
	HASH_FNV_T hval;
	int     test_failed;

	for (tp = testcases; tp->str; tp++) {
	    test_failed = 0;
	    if ((hval = hash_fnvz(tp->str)) != tp->hval) {
		msg_warn("hash_fnv(\"%s\") want %lu, got: %lu",
			 tp->str, (unsigned long) tp->hval, 
			(unsigned long) hval);
		test_failed = 1;
	    }
	    if (test_failed) {
		fail += 1;
		msg_info("FAIL:	%s", tp->str);
	    } else {
		pass += 1;
		msg_info("PASS: %s", tp->str);
	    }
	}
    }

    /*
     * Test: hash_fnvz(s) is equivalent to hash_fnv(s, strlen(s)). No need to
     * verify the actual result; we already did that above.
     */
    {
	const char *strval = "foobar";
	HASH_FNV_T h1 = hash_fnv(strval, strlen(strval));
	HASH_FNV_T h2 = hash_fnvz(strval);

	if (h1 == h2) {
	    pass += 1;
	    msg_info("PASS: hash_fnvz(\"%s\") == hash_fnv(\"%s\", %ld)",
		     strval, strval, (long) strlen(strval));
	} else {
	    fail += 1;
	    msg_info("FAIL: hash_fnvz(\"%s\") == hash_fnv(\"%s\", %ld)",
		     strval, strval, (long) strlen(strval));
	}
    }


    /*
     * Wrap up.
     */
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail != 0);
}

#endif
