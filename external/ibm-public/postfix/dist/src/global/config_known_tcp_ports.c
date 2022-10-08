/*	$NetBSD: config_known_tcp_ports.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	config_known_tcp_ports 3
/* SUMMARY
/*	parse and store known TCP port configuration
/* SYNOPSIS
/*	#include <config_known_tcp_ports.h>
/*
/*	void	config_known_tcp_ports(
/*	const char *source,
/*	const char *settings);
/* DESCRIPTION
/*	config_known_tcp_ports() parses the known TCP port information
/*	in the settings argument, and reports any warnings to the standard
/*	error stream. The source argument is used to provide warning
/*	context. It typically is a configuration parameter name.
/* .SH EXPECTED SYNTAX (ABNF)
/*	configuration = empty | name-to-port *("," name-to-port)
/*	name-to-port = 1*(name "=") port
/* SH EXAMPLES
/*	In the example below, the whitespace is optional.
/*	smtp = 25, smtps = submissions = 465, submission = 587
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
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <argv.h>
#include <known_tcp_ports.h>
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>

 /*
  * Application-specific.
  */
#include <config_known_tcp_ports.h>

/* config_known_tcp_ports - parse configuration and store associations */

void    config_known_tcp_ports(const char *source, const char *settings)
{
    ARGV   *associations;
    ARGV   *association;
    char  **cpp;

    clear_known_tcp_ports();

    /*
     * The settings is in the form of associations separated by comma. Split
     * it into separate associations.
     */
    associations = argv_split(settings, ",");
    if (associations->argc == 0) {
	argv_free(associations);
	return;
    }

    /*
     * Each association is in the form of "1*(name =) port". We use
     * argv_split() to carve this up, then we use mystrtok() to validate the
     * individual fragments. But first we prepend and append space so that we
     * get sensible results when an association starts or ends in "=".
     */
    for (cpp = associations->argv; *cpp != 0; cpp++) {
	char   *temp = concatenate(" ", *cpp, " ", (char *) 0);

	association = argv_split_at(temp, '=');
	myfree(temp);

	if (association->argc == 0) {
	     /* empty, ignore */ ;
	} else if (association->argc == 1) {
	    msg_warn("%s: in \"%s\" is not in \"name = value\" form",
		     source, *cpp);
	} else {
	    char   *bp;
	    char   *lhs;
	    char   *rhs;
	    const char *err = 0;
	    int     n;

	    bp = association->argv[association->argc - 1];
	    if ((rhs = mystrtok(&bp, CHARS_SPACE)) == 0) {
		err = "missing port value after \"=\"";
	    } else if (mystrtok(&bp, CHARS_SPACE) != 0) {
		err = "whitespace in port number";
	    } else {
		for (n = 0; n < association->argc - 1; n++) {
		    const char *new_err;

		    bp = association->argv[n];
		    if ((lhs = mystrtok(&bp, CHARS_SPACE)) == 0) {
			new_err = "missing service name before \"=\"";
		    } else if (mystrtok(&bp, CHARS_SPACE) != 0) {
			new_err = "whitespace in service name";
		    } else {
			new_err = add_known_tcp_port(lhs, rhs);
		    }
		    if (new_err != 0 && err == 0)
			err = new_err;
		}
	    }
	    if (err != 0) {
		msg_warn("%s: in \"%s\": %s", source, *cpp, err);
	    }
	}
	argv_free(association);
    }
    argv_free(associations);
}

#ifdef TEST

#include <stdlib.h>
#include <string.h>
#include <msg_vstream.h>

#define STR(x) vstring_str(x)

 /* TODO(wietse) make this a proper VSTREAM interface */

/* vstream_swap - kludge to capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

struct test_case {
    const char *label;			/* identifies test case */
    const char *config;			/* configuration under test */
    const char *exp_warning;		/* expected warning or null */
    const char *exp_export;		/* expected export or null */
};

static struct test_case test_cases[] = {
    {"good",
	 /* config */ "smtp = 25, smtps = submissions = 465, lmtp = 24",
	 /* warning */ "",
	 /* export */ "lmtp=24 smtp=25 smtps=465 submissions=465"
    },
    {"equal-equal",
	 /* config */ "smtp = 25, smtps == submissions = 465, lmtp = 24",
	 /* warning */ "config_known_tcp_ports: warning: equal-equal: "
	"in \" smtps == submissions = 465\": missing service name before "
	"\"=\"\n",
	 /* export */ "lmtp=24 smtp=25 smtps=465 submissions=465"
    },
    {"port test 1",
	 /* config */ "smtps = submission =",
	 /* warning */ "config_known_tcp_ports: warning: port test 1: "
	"in \"smtps = submission =\": missing port value after \"=\"\n",
	 /* export */ ""
    },
    {"port test 2",
	 /* config */ "smtps = submission = 4 65",
	 /* warning */ "config_known_tcp_ports: warning: port test 2: "
	"in \"smtps = submission = 4 65\": whitespace in port number\n",
	 /* export */ ""
    },
    {"port test 3",
	 /* config */ "lmtp = 24, smtps = submission = foo",
	 /* warning */ "config_known_tcp_ports: warning: port test 3: "
	"in \" smtps = submission = foo\": non-numerical service port\n",
	 /* export */ "lmtp=24"
    },
    {"service name test 1",
	 /* config */ "smtps = sub mission = 465",
	 /* warning */ "config_known_tcp_ports: warning: service name test 1: "
	"in \"smtps = sub mission = 465\": whitespace in service name\n",
	 /* export */ "smtps=465"
    },
    {"service name test 2",
	 /* config */ "lmtp = 24, smtps = 1234 = submissions = 465",
	 /* warning */ "config_known_tcp_ports: warning: service name test 2: "
	"in \" smtps = 1234 = submissions = 465\": numerical service name\n",
	 /* export */ "lmtp=24 smtps=465 submissions=465"
    },
    0,
};

int     main(int argc, char **argv)
{
    VSTRING *export_buf;
    struct test_case *tp;
    int     pass = 0;
    int     fail = 0;
    int     test_failed;
    const char *export;
    VSTRING *msg_buf;
    VSTREAM *memory_stream;

#define STRING_OR_NULL(s) ((s) ? (s) : "(null)")

    msg_vstream_init("config_known_tcp_ports", VSTREAM_ERR);

    export_buf = vstring_alloc(100);
    msg_buf = vstring_alloc(100);
    for (tp = test_cases; tp->label != 0; tp++) {
	test_failed = 0;
	if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
	    msg_fatal("open memory stream: %m");
	vstream_swap(VSTREAM_ERR, memory_stream);
	config_known_tcp_ports(tp->label, tp->config);
	vstream_swap(memory_stream, VSTREAM_ERR);
	if (vstream_fclose(memory_stream))
	    msg_fatal("close memory stream: %m");
	if (strcmp(STR(msg_buf), tp->exp_warning) != 0) {
	    msg_warn("test case %s: got error: \"%s\", want: \"%s\"",
		     tp->label, STR(msg_buf),
		     STRING_OR_NULL(tp->exp_warning));
	    test_failed = 1;
	} else {
	    export = export_known_tcp_ports(export_buf);
	    if (strcmp(export, tp->exp_export) != 0) {
		msg_warn("test case %s: got export: \"%s\", want: \"%s\"",
			 tp->label, export, tp->exp_export);
		test_failed = 1;
	    }
	    clear_known_tcp_ports();
	    VSTRING_RESET(msg_buf);
	    VSTRING_TERMINATE(msg_buf);
	}
	if (test_failed) {
	    msg_info("%s: FAIL", tp->label);
	    fail++;
	} else {
	    msg_info("%s: PASS", tp->label);
	    pass++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    vstring_free(msg_buf);
    vstring_free(export_buf);
    exit(fail != 0);
}

#endif
