/*	$NetBSD: login_sender_match_test.c,v 1.1.1.1 2026/05/09 18:39:18 christos Exp $	*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <login_sender_match.h>
#include <mail_params.h>

int     main(int argc, char **argv)
{
    struct testcase {
	const char *title;
	const char *map_names;
	const char *ext_delimiters;
	const char *null_sender;
	const char *wildcard;
	const char *login_name;
	const char *sender_addr;
	int     exp_return;
    };
    struct testcase testcases[] = {
	{"wildcard works",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "root", "anything", LSM_STAT_FOUND
	},
	{"unknown user",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "toor", "anything", LSM_STAT_NOTFOUND
	},
	{"bare user",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "foo", LSM_STAT_FOUND
	},
	{"user@domain",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "foo@example.com", LSM_STAT_FOUND
	},
	{"user+ext@domain",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "foo+bar@example.com", LSM_STAT_FOUND
	},
	{"wrong sender",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "bar@example.com", LSM_STAT_NOTFOUND
	},
	{"@domain",
	    "inline:{root=*, {foo = @example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "anyone@example.com", LSM_STAT_FOUND
	},
	{"wrong @domain",
	    "inline:{root=*, {foo = @example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "anyone@example.org", LSM_STAT_NOTFOUND
	},
	{"null sender",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "bar", "", LSM_STAT_FOUND
	},
	{"wrong null sender",
	    "inline:{root=*, {foo = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "baz", "", LSM_STAT_NOTFOUND
	},
	{"error",
	    "inline:{root=*}, fail:sorry",
	    "+-", "<>", "*", "baz", "whatever", LSM_STAT_RETRY
	},
	{"no error",
	    "inline:{root=*}, fail:sorry",
	    "+-", "<>", "*", "root", "whatever", LSM_STAT_FOUND
	},
	{"unknown uid:number",
	    "inline:{root=*, {uid:12345 = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "uid:54321", "foo", LSM_STAT_NOTFOUND
	},
	{"known uid:number",
	    "inline:{root=*, {uid:12345 = foo,foo@example.com}, bar=<>}",
	    "+-", "<>", "*", "uid:12345", "foo", LSM_STAT_FOUND
	},
	{"unknown \"other last\"",
	    "inline:{root=*, {foo = \"first last\",\"first last\"@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "other last", LSM_STAT_NOTFOUND
	},
	{"bare \"first last\"",
	    "inline:{root=*, {foo = \"first last\",\"first last\"@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "first last", LSM_STAT_FOUND
	},
	{"\"first last\"@domain",
	    "inline:{root=*, {foo = \"first last\",\"first last\"@example.com}, bar=<>}",
	    "+-", "<>", "*", "foo", "first last@example.com", LSM_STAT_FOUND
	},
    };
    struct testcase *tp;
    int     act_return;
    int     pass;
    int     fail;
    LOGIN_SENDER_MATCH *lsm;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    /*
     * Fake variable settings.
     */
    var_double_bounce_sender = DEF_DOUBLE_BOUNCE;
    var_ownreq_special = DEF_OWNREQ_SPECIAL;

#define NUM_TESTS	sizeof(testcases)/sizeof(testcases[0])

    for (pass = fail = 0, tp = testcases; tp < testcases + NUM_TESTS; tp++) {
	msg_info("RUN test case %ld %s", (long) (tp - testcases), tp->title);
#if 0
	msg_info("title=%s", tp->title);
	msg_info("map_names=%s", tp->map_names);
	msg_info("ext_delimiters=%s", tp->ext_delimiters);
	msg_info("null_sender=%s", tp->null_sender);
	msg_info("wildcard=%s", tp->wildcard);
	msg_info("login_name=%s", tp->login_name);
	msg_info("sender_addr=%s", tp->sender_addr);
	msg_info("exp_return=%d", tp->exp_return);
#endif
	lsm = login_sender_create("test map", tp->map_names,
				  tp->ext_delimiters, tp->null_sender,
				  tp->wildcard);
	act_return = login_sender_match(lsm, tp->login_name, tp->sender_addr);
	if (act_return == tp->exp_return) {
	    msg_info("PASS test %ld", (long) (tp - testcases));
	    pass++;
	} else {
	    msg_info("FAIL test %ld", (long) (tp - testcases));
	    fail++;
	}
	login_sender_free(lsm);
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}
