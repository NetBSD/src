/*	$NetBSD: ehlo_mask_test.c,v 1.2 2026/05/09 18:49:16 christos Exp $	*/

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
#include <ehlo_mask.h>

 /*
  * Tests and test cases.
  */
typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    const char *raw;
    int     mask;
    const char *text;
} TEST_CASE;

 /*
  * Verify that each verb has its unique bit mask, and vice versa.
  */
static const TEST_CASE test_cases[] = {
    {"8BITMIME",
	EHLO_VERB_8BITMIME,
	EHLO_MASK_8BITMIME,
	"8BITMIME"
    },
    {"8bitmime",
	"8bitmime",
	EHLO_MASK_8BITMIME,
	"8BITMIME"
    },
    {"PIPELINING",
	EHLO_VERB_PIPELINING,
	EHLO_MASK_PIPELINING,
	"PIPELINING"
    },
    {"SIZE",
	EHLO_VERB_SIZE,
	EHLO_MASK_SIZE,
	"SIZE"
    },
    {"VRFY",
	EHLO_VERB_VRFY,
	EHLO_MASK_VRFY,
	"VRFY"
    },
    {"ETRN",
	EHLO_VERB_ETRN,
	EHLO_MASK_ETRN,
	"ETRN"
    },
    {"AUTH",
	EHLO_VERB_AUTH,
	EHLO_MASK_AUTH,
	"AUTH"
    },
    {"VERP",
	EHLO_VERB_VERP,
	EHLO_MASK_VERP,
	"VERP"
    },
    {"STARTTLS",
	EHLO_VERB_STARTTLS,
	EHLO_MASK_STARTTLS,
	"STARTTLS"
    },
    {"XCLIENT",
	EHLO_VERB_XCLIENT,
	EHLO_MASK_XCLIENT,
	"XCLIENT"
    },
    {"ENHANCEDSTATUSCODES",
	EHLO_VERB_ENHANCEDSTATUSCODES,
	EHLO_MASK_ENHANCEDSTATUSCODES,
	"ENHANCEDSTATUSCODES"
    },
    {"DSN",
	EHLO_VERB_DSN,
	EHLO_MASK_DSN,
	"DSN"
    },
    {"SMTPUTF8",
	EHLO_VERB_SMTPUTF8,
	EHLO_MASK_SMTPUTF8,
	"SMTPUTF8"
    },
    {"CHUNKING",
	EHLO_VERB_CHUNKING,
	EHLO_MASK_CHUNKING,
	"CHUNKING"
    },
    {"REQUIRETLS",
	EHLO_VERB_REQTLS,
	EHLO_MASK_REQTLS,
	"REQUIRETLS"
    },
    {"SILENT",
	EHLO_VERB_SILENT,
	EHLO_MASK_SILENT,
	"SILENT-DISCARD"
    },
    {0},
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;
    const char *got_text;
    int     got_mask;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    /*
     * Verify the mapping in both directions.
     */
    for (tp = test_cases; tp->label != 0; tp++) {
	msg_info("RUN  %s", tp->label);
	if ((got_mask = ehlo_mask(tp->raw)) != tp->mask) {
	    msg_warn("got mask '0x%x', want: '0x%x'", got_mask, tp->mask);
	    fail++;
	    msg_info("FAIL %s", tp->label);
	} else if ((got_text = str_ehlo_mask(tp->mask)),
		   strcmp(got_text, tp->text) != 0) {
	    msg_warn("got text '%s', want: '%s'", got_text, tp->text);
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
