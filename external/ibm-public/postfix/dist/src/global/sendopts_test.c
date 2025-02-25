/*	$NetBSD: sendopts_test.c,v 1.2 2025/02/25 19:15:46 christos Exp $	*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>
#include <stringops.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <sendopts.h>

 /*
  * Tests and test cases.
  */
typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    int     mask;
    const char *want;
} TEST_CASE;

static const TEST_CASE test_cases[] = {
    {"SOPT_SMTPUTF8_ALL",
	SOPT_SMTPUTF8_ALL,
	"smtputf8_requested smtputf8_header smtputf8_sender smtputf8_recipient"
    },
    {"SOPT_SMTPUTF8_DERIVED",
	SOPT_SMTPUTF8_DERIVED,
	"smtputf8_header smtputf8_sender smtputf8_recipient"
    },
    {"SOPT_SMTPUTF8_REQUESTED",
	SOPT_SMTPUTF8_REQUESTED,
	"smtputf8_requested"
    },
    {"SOPT_SMTPUTF8_HEADER",
	SOPT_SMTPUTF8_HEADER,
	"smtputf8_header"
    },
    {"SOPT_SMTPUTF8_SENDER",
	SOPT_SMTPUTF8_SENDER,
	"smtputf8_sender"
    },
    {"SOPT_SMTPUTF8_RECIPIENT",
	SOPT_SMTPUTF8_RECIPIENT,
	"smtputf8_recipient"
    },
    {"SOPT_REQUIRETLS_ALL",
	SOPT_REQUIRETLS_ALL,
	"requiretls_header requiretls_esmtp"
    },
    {"SOPT_REQUIRETLS_DERIVED",
	SOPT_REQUIRETLS_DERIVED,
	"requiretls_header"
    },
    {"SOPT_REQUIRETLS_HEADER",
	SOPT_REQUIRETLS_HEADER,
	"requiretls_header"
    },
    {"SOPT_REQUIRETLS_ESMTP",
	SOPT_REQUIRETLS_ESMTP,
	"requiretls_esmtp"
    },
    {"SOPT_FLAG_ALL",
	SOPT_FLAG_ALL,
	"smtputf8_requested smtputf8_header smtputf8_sender smtputf8_recipient"
	" requiretls_header requiretls_esmtp"
    },
    {"SOPT_FLAG_DERIVED",
	SOPT_FLAG_DERIVED,
	"smtputf8_header smtputf8_sender smtputf8_recipient"
	" requiretls_header"
    },
    {0},
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;
    const char *got;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
	msg_info("RUN  %s", tp->label);
	got = sendopts_strflags(tp->mask, ' ');
	if (strcmp(got, tp->want) != 0) {
	    msg_warn("got result '%s', want: '%s'", got, tp->want);
	    fail++;
	    msg_info("FAIL %s", tp->label);
	} else {
	    msg_info("PASS %s", tp->label);
	    pass++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}
