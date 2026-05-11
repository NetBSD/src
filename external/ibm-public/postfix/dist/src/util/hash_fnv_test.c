/*	$NetBSD: hash_fnv_test.c,v 1.2.2.2 2026/05/11 17:14:03 martin Exp $	*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <hash_fnv.h>

int     main(int srgc, char **argv)
{
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

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
	    msg_info("RUN  hash_fnvz(\"%s\")", tp->str);
	    if ((hval = hash_fnvz(tp->str)) != tp->hval) {
		msg_warn("hash_fnv(\"%s\") want %lu, got: %lu",
			 tp->str, (unsigned long) tp->hval,
			 (unsigned long) hval);
		test_failed = 1;
	    }
	    if (test_failed) {
		fail += 1;
		msg_info("FAIL hash_fnvz(\"%s\")", tp->str);
	    } else {
		pass += 1;
		msg_info("PASS hash_fnvz(\"%s\")", tp->str);
	    }
	}
    }

    /*
     * Test: hash_fnvz(s) is equivalent to hash_fnv(s, strlen(s)). No need to
     * verify the actual result; we already did that above.
     */
    {
	const char *strval = "foobar";
	HASH_FNV_T h1;
	HASH_FNV_T h2;

	msg_info("RUN hash_fnvz(\"%s\") == hash_fnv(\"%s\", %ld)",
		 strval, strval, (long) strlen(strval));

	h1 = hash_fnv(strval, strlen(strval));
	h2 = hash_fnvz(strval);

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
