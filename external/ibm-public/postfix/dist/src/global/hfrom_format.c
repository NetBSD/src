/*	$NetBSD: hfrom_format.c,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	hfrom_format 3
/* SUMMARY
/*	Parse a header_from_format setting
/* SYNOPSIS
/*	#include <hfrom_format.h>
/*
/*	int	hfrom_format_parse(
/*	const char *name,
/*	const char *value)
/*
/*	const char *str_hfrom_format_code(int code)
/* DESCRIPTION
/*	hfrom_format_parse() takes a parameter name (used for
/*	diagnostics) and value, and maps it to the corresponding
/*	code: HFROM_FORMAT_NAME_STD maps to HFROM_FORMAT_CODE_STD,
/*	and HFROM_FORMAT_NAME_OBS maps to HFROM_FORMAT_CODE_OBS.
/*
/*	str_hfrom_format_code() does the reverse mapping.
/* DIAGNOSTICS
/*	All input errors are fatal.
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
#include <name_code.h>
#include <msg.h>

 /*
  * Global library.
  */
#include <mail_params.h>

 /*
  * Application-specific.
  */
#include <hfrom_format.h>

 /*
  * Primitive dependency injection.
  */
#ifdef TEST
extern NORETURN PRINTFLIKE(1, 2) test_msg_fatal(const char *,...);

#define msg_fatal test_msg_fatal
#endif

 /*
  * The name-to-code mapping.
  */
static const NAME_CODE hfrom_format_table[] = {
    HFROM_FORMAT_NAME_STD, HFROM_FORMAT_CODE_STD,
    HFROM_FORMAT_NAME_OBS, HFROM_FORMAT_CODE_OBS,
    0, -1,
};

/* hfrom_format_parse - parse header_from_format setting */

int     hfrom_format_parse(const char *name, const char *value)
{
    int     code;

    if ((code = name_code(hfrom_format_table, NAME_CODE_FLAG_NONE, value)) < 0)
	msg_fatal("invalid setting: \"%s = %s\"", name, value);
    return (code);
}

/* str_hfrom_format_code - convert code to string */

const char *str_hfrom_format_code(int code)
{
    const char *name;

    if ((name = str_name_code(hfrom_format_table, code)) == 0)
	msg_fatal("invalid header format code: %d", code);
    return (name);
}

#ifdef TEST
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>

#define STR(x) vstring_str(x)

 /*
  * TODO(wietse) make this a proper VSTREAM interface. Instead of temporarily
  * swapping streams, we could temporarily swap the stream's write function.
  */

/* vstream_swap - kludge to capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

jmp_buf test_fatal_jbuf;

#undef msg_fatal

/* test_msg_fatal - does not return, and does not terminate */

void    test_msg_fatal(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_warn(fmt, ap);
    va_end(ap);
    longjmp(test_fatal_jbuf, 1);
}

struct name_test_case {
    const char *label;			/* identifies test case */
    const char *config;			/* configuration under test */
    const char *exp_warning;		/* expected warning or empty */
    const int exp_code;			/* expected code */
};

static struct name_test_case name_test_cases[] = {
    {"hfrom_format_parse good-standard",
	 /* config */ HFROM_FORMAT_NAME_STD,
	 /* warning */ "",
	 /* exp_code */ HFROM_FORMAT_CODE_STD
    },
    {"hfrom_format_parse good-obsolete",
	 /* config */ HFROM_FORMAT_NAME_OBS,
	 /* warning */ "",
	 /* exp_code */ HFROM_FORMAT_CODE_OBS
    },
    {"hfrom_format_parse bad",
	 /* config */ "does-not-exist",
	 /* warning */ "hfrom_format: warning: invalid setting: \"hfrom_format_parse bad = does-not-exist\"\n",
	 /* code */ 0,
    },
    {"hfrom_format_parse empty",
	 /* config */ "",
	 /* warning */ "hfrom_format: warning: invalid setting: \"hfrom_format_parse empty = \"\n",
	 /* code */ 0,
    },
    0,
};

struct code_test_case {
    const char *label;			/* identifies test case */
    int     code;			/* code under test */
    const char *exp_warning;		/* expected warning or empty */
    const char *exp_name;		/* expected namme */
};

static struct code_test_case code_test_cases[] = {
    {"str_hfrom_format_code good-standard",
	 /* code */ HFROM_FORMAT_CODE_STD,
	 /* warning */ "",
	 /* exp_name */ HFROM_FORMAT_NAME_STD
    },
    {"str_hfrom_format_code good-obsolete",
	 /* code */ HFROM_FORMAT_CODE_OBS,
	 /* warning */ "",
	 /* exp_name */ HFROM_FORMAT_NAME_OBS
    },
    {"str_hfrom_format_code bad",
	 /* config */ 12345,
	 /* warning */ "hfrom_format: warning: invalid header format code: 12345\n",
	 /* exp_name */ 0
    },
    0,
};

int     main(int argc, char **argv)
{
    struct name_test_case *np;
    int     code;
    struct code_test_case *cp;
    const char *name;
    int     pass = 0;
    int     fail = 0;
    int     test_failed;
    VSTRING *msg_buf;
    VSTREAM *memory_stream;

    msg_vstream_init("hfrom_format", VSTREAM_ERR);
    msg_buf = vstring_alloc(100);

    for (np = name_test_cases; np->label != 0; np++) {
	VSTRING_RESET(msg_buf);
	VSTRING_TERMINATE(msg_buf);
	test_failed = 0;
	if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
	    msg_fatal("open memory stream: %m");
	vstream_swap(VSTREAM_ERR, memory_stream);
	if (setjmp(test_fatal_jbuf) == 0)
	    code = hfrom_format_parse(np->label, np->config);
	vstream_swap(memory_stream, VSTREAM_ERR);
	if (vstream_fclose(memory_stream))
	    msg_fatal("close memory stream: %m");
	if (strcmp(STR(msg_buf), np->exp_warning) != 0) {
	    msg_warn("test case %s: got error: \"%s\", want: \"%s\"",
		     np->label, STR(msg_buf), np->exp_warning);
	    test_failed = 1;
	}
	if (*np->exp_warning == 0) {
	    if (code != np->exp_code) {
		msg_warn("test case %s: got code: \"%d\", want: \"%d\"(%s)",
			 np->label, code, np->exp_code,
			 str_hfrom_format_code(np->exp_code));
		test_failed = 1;
	    }
	}
	if (test_failed) {
	    msg_info("%s: FAIL", np->label);
	    fail++;
	} else {
	    msg_info("%s: PASS", np->label);
	    pass++;
	}
    }

    for (cp = code_test_cases; cp->label != 0; cp++) {
	VSTRING_RESET(msg_buf);
	VSTRING_TERMINATE(msg_buf);
	test_failed = 0;
	if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
	    msg_fatal("open memory stream: %m");
	vstream_swap(VSTREAM_ERR, memory_stream);
	if (setjmp(test_fatal_jbuf) == 0)
	    name = str_hfrom_format_code(cp->code);
	vstream_swap(memory_stream, VSTREAM_ERR);
	if (vstream_fclose(memory_stream))
	    msg_fatal("close memory stream: %m");
	if (strcmp(STR(msg_buf), cp->exp_warning) != 0) {
	    msg_warn("test case %s: got error: \"%s\", want: \"%s\"",
		     cp->label, STR(msg_buf), cp->exp_warning);
	    test_failed = 1;
	} else if (*cp->exp_warning == 0) {
	    if (strcmp(name, cp->exp_name)) {
		msg_warn("test case %s: got name: \"%s\", want: \"%s\"",
			 cp->label, name, cp->exp_name);
		test_failed = 1;
	    }
	}
	if (test_failed) {
	    msg_info("%s: FAIL", cp->label);
	    fail++;
	} else {
	    msg_info("%s: PASS", cp->label);
	    pass++;
	}
    }

    msg_info("PASS=%d FAIL=%d", pass, fail);
    vstring_free(msg_buf);
    exit(fail != 0);
}

#endif
