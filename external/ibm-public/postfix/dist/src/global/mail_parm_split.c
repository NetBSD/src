/*	$NetBSD: mail_parm_split.c,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	mail_parm_split 3
/* SUMMARY
/*	split parameter list value
/* SYNOPSIS
/*	#include <mail_parm_split.h>
/*
/*	ARGV	*mail_parm_split(
/*	const char *name,
/*	const char *value)
/* DESCRIPTION
/*	mail_parm_split() splits a parameter list value into its
/*	elements, and extracts text from elements that are entirely
/*	enclosed in {}. It uses CHARS_COMMA_SP as list element
/*	delimiters, and CHARS_BRACE for grouping.
/*
/*	Arguments:
/* .IP name
/*	Parameter name. This is used to provide context for
/*	error messages.
/* .IP value
/*	Parameter value.
/* DIAGNOSTICS
/*	fatal: syntax error while extracting text from {}, such as:
/*	missing closing brace, or text after closing brace.
/* SEE ALSO
/*	argv_splitq(3), string array utilities
/*	extpar(3), extract text from parentheses
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_parm_split.h>

 /*
  * While testing, do not terminate the program after a syntax error.
  */
#ifdef TEST
#undef msg_fatal
#define msg_fatal msg_warn
#endif

/* mail_parm_split - split list, extract {text}, errors are fatal */

ARGV   *mail_parm_split(const char *name, const char *value)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(value);
    char   *bp = saved_string;
    char   *arg;
    const char *err;

    /*
     * The code that detects the error shall either signal or handle the
     * error. In this case, mystrtokq() detects no error, extpar() signals
     * the error to its caller, and this function handles the error.
     */
    while ((arg = mystrtokq(&bp, CHARS_COMMA_SP, CHARS_BRACE)) != 0) {
	if (*arg == CHARS_BRACE[0]
	    && (err = extpar(&arg, CHARS_BRACE, EXTPAR_FLAG_STRIP)) != 0)
	    msg_fatal("%s: %s", name, err);
	argv_add(argvp, arg, (char *) 0);
    }
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

#ifdef TEST

 /*
  * This function is security-critical so it better have a unit-test driver.
  */
#include <string.h>
#include <vstream.h>
#include <vstream.h>
#include <vstring_vstream.h>

int     main(void)
{
    VSTRING *vp = vstring_alloc(100);
    ARGV   *argv;
    char   *start;
    char   *str;
    char  **cpp;

    while (vstring_fgets_nonl(vp, VSTREAM_IN) && VSTRING_LEN(vp) > 0) {
	start = vstring_str(vp);
	vstream_printf("Input:\t>%s<\n", start);
	vstream_fflush(VSTREAM_OUT);
	argv = mail_parm_split("stdin", start);
	for (cpp = argv->argv; (str = *cpp) != 0; cpp++)
	    vstream_printf("Output:\t>%s<\n", str);
	argv_free(argv);
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(vp);
    return (0);
}

#endif
