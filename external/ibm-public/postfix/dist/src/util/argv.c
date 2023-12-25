/*	$NetBSD: argv.c,v 1.3.2.1 2023/12/25 12:43:36 martin Exp $	*/

/*++
/* NAME
/*	argv 3
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include <argv.h>
/*
/*	typedef	int (*ARGV_COMPAR_FN)(const void *, const void *);
/*
/*	ARGV	*argv_alloc(len)
/*	ssize_t	len;
/*
/*	ARGV    *argv_qsort(argvp, compar)
/*	ARGV    *argvp;
/*	ARGV_COMPAR_FN compar;
/*
/*	void	argv_uniq(argvp, compar)
/*	ARGV	*argvp;
/*	ARGV_COMPAR_FN compar;
/*
/*	ARGV	*argv_free(argvp)
/*	ARGV	*argvp;
/*
/*	void	argv_add(argvp, arg, ..., ARGV_END)
/*	ARGV	*argvp;
/*	char	*arg;
/*
/*	void	argv_addn(argvp, arg, arg_len, ..., ARGV_END)
/*	ARGV	*argvp;
/*	char	*arg;
/*	ssize_t	arg_len;
/*
/*	void	argv_terminate(argvp);
/*	ARGV	*argvp;
/*
/*	void	argv_truncate(argvp, len);
/*	ARGV	*argvp;
/*	ssize_t	len;
/*
/*	void	argv_insert_one(argvp, pos, arg)
/*	ARGV	*argvp;
/*	ssize_t	pos;
/*	const char *arg;
/*
/*	void	argv_replace_one(argvp, pos, arg)
/*	ARGV	*argvp;
/*	ssize_t	pos;
/*	const char *arg;
/*
/*	void	argv_delete(argvp, pos, how_many)
/*	ARGV	*argvp;
/*	ssize_t	pos;
/*	ssize_t	how_many;
/*
/*	void	ARGV_FAKE_BEGIN(argv, arg)
/*	const char *arg;
/*
/*	void	ARGV_FAKE_END
/* DESCRIPTION
/*	The functions in this module manipulate arrays of string
/*	pointers. An ARGV structure contains the following members:
/* .IP len
/*	The length of the \fIargv\fR array member.
/* .IP argc
/*	The number of \fIargv\fR elements used.
/* .IP argv
/*	An array of pointers to null-terminated strings.
/* .PP
/*	argv_alloc() returns an empty string array of the requested
/*	length. The result is ready for use by argv_add(). The array
/*	is null terminated.
/*
/*	argv_qsort() sorts the elements of argvp in place, and
/*	returns its first argument. If the compar argument specifies
/*	a null pointer, then argv_qsort() will use byte-by-byte
/*	comparison.
/*
/*	argv_uniq() reduces adjacent same-value elements to one
/*	element, and returns its first argument. If the compar
/*	argument specifies a null pointer, then argv_uniq() will
/*	use byte-by-byte comparison.
/*
/*	argv_add() copies zero or more strings and adds them to the
/*	specified string array. The array is null terminated.
/*	Terminate the argument list with a null pointer. The manifest
/*	constant ARGV_END provides a convenient notation for this.
/*
/*	argv_addn() is like argv_add(), but each string is followed
/*	by a string length argument.
/*
/*	argv_free() releases storage for a string array, and conveniently
/*	returns a null pointer.
/*
/*	argv_terminate() null-terminates its string array argument.
/*
/*	argv_truncate() truncates its argument to the specified
/*	number of entries, but does not reallocate memory. The
/*	result is null-terminated.
/*
/*	argv_insert_one() inserts one string at the specified array
/*	position.
/*
/*	argv_replace_one() replaces one string at the specified
/*	position. The old string is destroyed after the update is
/*	made.
/*
/*	argv_delete() deletes the specified number of elements
/*	starting at the specified array position. The result is
/*	null-terminated.
/*
/*	ARGV_FAKE_BEGIN/END are an optimization for the case where
/*	a single string needs to be passed into an ARGV-based
/*	interface.  ARGV_FAKE_BEGIN() opens a statement block and
/*	allocates a stack-based ARGV structure named after the first
/*	argument, that encapsulates the second argument.  This
/*	implementation allocates no heap memory and creates no copy
/*	of the second argument.  ARGV_FAKE_END closes the statement
/*	block and thereby releases storage.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System libraries. */

#include <sys_defs.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Application-specific. */

#include "mymalloc.h"
#include "msg.h"
#include "argv.h"

#ifdef TEST
extern NORETURN PRINTFLIKE(1, 2) test_msg_panic(const char *,...);

#define msg_panic test_msg_panic
#endif

/* argv_free - destroy string array */

ARGV   *argv_free(ARGV *argvp)
{
    char  **cpp;

    for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++)
	myfree(*cpp);
    myfree((void *) argvp->argv);
    myfree((void *) argvp);
    return (0);
}

/* argv_alloc - initialize string array */

ARGV   *argv_alloc(ssize_t len)
{
    ARGV   *argvp;
    ssize_t sane_len;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
    argvp = (ARGV *) mymalloc(sizeof(*argvp));
    argvp->len = 0;
    sane_len = (len < 2 ? 2 : len);
    argvp->argv = (char **) mymalloc((sane_len + 1) * sizeof(char *));
    argvp->len = sane_len;
    argvp->argc = 0;
    argvp->argv[0] = 0;
    return (argvp);
}

static int argv_cmp(const void *e1, const void *e2)
{
    const char *s1 = *(const char **) e1;
    const char *s2 = *(const char **) e2;

    return strcmp(s1, s2);
}

/* argv_qsort - sort array in place */

ARGV   *argv_qsort(ARGV *argvp, ARGV_COMPAR_FN compar)
{
    qsort(argvp->argv, argvp->argc, sizeof(argvp->argv[0]),
	  compar ? compar : argv_cmp);
    return (argvp);
}

/* argv_sort - binary compatibility */

ARGV   *argv_sort(ARGV *argvp)
{
    qsort(argvp->argv, argvp->argc, sizeof(argvp->argv[0]), argv_cmp);
    return (argvp);
}

/* argv_uniq - deduplicate adjacent array elements */

ARGV   *argv_uniq(ARGV *argvp, ARGV_COMPAR_FN compar)
{
    char  **cpp;
    char  **prev;

    if (compar == 0)
	compar = argv_cmp;
    for (prev = 0, cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
	if (prev != 0 && compar(prev, cpp) == 0) {
	    argv_delete(argvp, cpp - argvp->argv, 1);
	    cpp = prev;
	} else {
	    prev = cpp;
	}
    }
    return (argvp);
}

/* argv_extend - extend array */

static void argv_extend(ARGV *argvp)
{
    ssize_t new_len;

    new_len = argvp->len * 2;
    argvp->argv = (char **)
	myrealloc((void *) argvp->argv, (new_len + 1) * sizeof(char *));
    argvp->len = new_len;
}

/* argv_add - add string to vector */

void    argv_add(ARGV *argvp,...)
{
    char   *arg;
    va_list ap;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
#define ARGV_SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

    va_start(ap, argvp);
    while ((arg = va_arg(ap, char *)) != 0) {
	if (ARGV_SPACE_LEFT(argvp) <= 0)
	    argv_extend(argvp);
	argvp->argv[argvp->argc++] = mystrdup(arg);
    }
    va_end(ap);
    argvp->argv[argvp->argc] = 0;
}

/* argv_addn - add string to vector */

void    argv_addn(ARGV *argvp,...)
{
    char   *arg;
    ssize_t len;
    va_list ap;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
    va_start(ap, argvp);
    while ((arg = va_arg(ap, char *)) != 0) {
	if ((len = va_arg(ap, ssize_t)) < 0)
	    msg_panic("argv_addn: bad string length %ld", (long) len);
	if (ARGV_SPACE_LEFT(argvp) <= 0)
	    argv_extend(argvp);
	argvp->argv[argvp->argc++] = mystrndup(arg, len);
    }
    va_end(ap);
    argvp->argv[argvp->argc] = 0;
}

/* argv_terminate - terminate string array */

void    argv_terminate(ARGV *argvp)
{

    /*
     * Trust that argvp->argc < argvp->len.
     */
    argvp->argv[argvp->argc] = 0;
}

/* argv_truncate - truncate string array */

void    argv_truncate(ARGV *argvp, ssize_t len)
{
    char  **cpp;

    /*
     * Sanity check.
     */
    if (len < 0)
	msg_panic("argv_truncate: bad length %ld", (long) len);

    if (len < argvp->argc) {
	for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++)
	    myfree(*cpp);
	argvp->argc = len;
	argvp->argv[argvp->argc] = 0;
    }
}

/* argv_insert_one - insert one string into array */

void    argv_insert_one(ARGV *argvp, ssize_t where, const char *arg)
{
    ssize_t pos;

    /*
     * Sanity check.
     */
    if (where < 0 || where > argvp->argc)
	msg_panic("argv_insert_one bad position: %ld", (long) where);

    if (ARGV_SPACE_LEFT(argvp) <= 0)
	argv_extend(argvp);
    for (pos = argvp->argc; pos >= where; pos--)
	argvp->argv[pos + 1] = argvp->argv[pos];
    argvp->argv[where] = mystrdup(arg);
    argvp->argc += 1;
}

/* argv_replace_one - replace one string in array */

void    argv_replace_one(ARGV *argvp, ssize_t where, const char *arg)
{
    char   *temp;

    /*
     * Sanity check.
     */
    if (where < 0 || where >= argvp->argc)
	msg_panic("argv_replace_one bad position: %ld", (long) where);

    temp = argvp->argv[where];
    argvp->argv[where] = mystrdup(arg);
    myfree(temp);
}

/* argv_delete - remove string(s) from array */

void    argv_delete(ARGV *argvp, ssize_t first, ssize_t how_many)
{
    ssize_t pos;

    /*
     * Sanity check.
     */
    if (first < 0 || how_many < 0 || first + how_many > argvp->argc)
	msg_panic("argv_delete bad range: (start=%ld count=%ld)",
		  (long) first, (long) how_many);

    for (pos = first; pos < first + how_many; pos++)
	myfree(argvp->argv[pos]);
    for (pos = first; pos <= argvp->argc - how_many; pos++)
	argvp->argv[pos] = argvp->argv[pos + how_many];
    argvp->argc -= how_many;
}

#ifdef TEST

 /*
  * System library.
  */
#include <setjmp.h>

 /*
  * Utility library.
  */
#include <msg_vstream.h>
#include <stringops.h>

#define ARRAY_LEN	(10)

typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    const char *inputs[ARRAY_LEN];	/* input strings */
    int     terminate;			/* terminate result */
    ARGV   *(*populate_fn) (const struct TEST_CASE *, ARGV *);
    const char *exp_panic_msg;		/* expected panic */
    int     exp_argc;			/* expected array length */
    const char *exp_argv[ARRAY_LEN];	/* expected array content */
} TEST_CASE;

#define TERMINATE_ARRAY	(1)

#define	PASS	(0)
#define FAIL	(1)

VSTRING *test_panic_str;
jmp_buf test_panic_jbuf;

/* test_msg_panic - does not return, and does not terminate */

void    test_msg_panic(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    test_panic_str = vstring_alloc(100);
    vstring_vsprintf(test_panic_str, fmt, ap);
    va_end(ap);
    longjmp(test_panic_jbuf, 1);
}

/* test_argv_populate - populate result, optionally terminate */

static ARGV *test_argv_populate(const TEST_CASE *tp, ARGV *argvp)
{
    const char *const * cpp;

    for (cpp = tp->inputs; *cpp; cpp++)
	argv_add(argvp, *cpp, (char *) 0);
    if (tp->terminate)
	argv_terminate(argvp);
    return (argvp);
}

/* test_argv_sort - populate and sort result */

static ARGV *test_argv_sort(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_qsort(argvp, (ARGV_COMPAR_FN) 0);
    return (argvp);
}

/* test_argv_sort_uniq - populate, sort, uniq result */

static ARGV *test_argv_sort_uniq(const TEST_CASE *tp, ARGV *argvp)
{

    /*
     * This also tests argv_delete().
     */
    test_argv_sort(tp, argvp);
    argv_uniq(argvp, (ARGV_COMPAR_FN) 0);
    return (argvp);
}

/* test_argv_good_truncate - populate and truncate to good size */

static ARGV *test_argv_good_truncate(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_truncate(argvp, tp->exp_argc);
    return (argvp);
}

/* test_argv_bad_truncate - populate and truncate to bad size */

static ARGV *test_argv_bad_truncate(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_truncate(argvp, -1);
    return (argvp);
}

/* test_argv_good_insert - populate and insert at good position */

static ARGV *test_argv_good_insert(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_insert_one(argvp, 1, "new");
    return (argvp);
}

/* test_argv_bad_insert1 - populate and insert at bad position */

static ARGV *test_argv_bad_insert1(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_insert_one(argvp, -1, "new");
    return (argvp);
}

/* test_argv_bad_insert2 - populate and insert at bad position */

static ARGV *test_argv_bad_insert2(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_insert_one(argvp, 100, "new");
    return (argvp);
}

/* test_argv_good_replace - populate and replace at good position */

static ARGV *test_argv_good_replace(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_replace_one(argvp, 1, "new");
    return (argvp);
}

/* test_argv_bad_replace1 - populate and replace at bad position */

static ARGV *test_argv_bad_replace1(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_replace_one(argvp, -1, "new");
    return (argvp);
}

/* test_argv_bad_replace2 - populate and replace at bad position */

static ARGV *test_argv_bad_replace2(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_replace_one(argvp, 100, "new");
    return (argvp);
}

/* test_argv_bad_delete1 - populate and delete at bad position */

static ARGV *test_argv_bad_delete1(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_delete(argvp, -1, 1);
    return (argvp);
}

/* test_argv_bad_delete2 - populate and delete at bad position */

static ARGV *test_argv_bad_delete2(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_delete(argvp, 0, -1);
    return (argvp);
}

/* test_argv_bad_delete3 - populate and delete at bad position */

static ARGV *test_argv_bad_delete3(const TEST_CASE *tp, ARGV *argvp)
{
    test_argv_populate(tp, argvp);
    argv_delete(argvp, 100, 1);
    return (argvp);
}

/* test_argv_verify - verify result */

static int test_argv_verify(const TEST_CASE *tp, ARGV *argvp)
{
    int     idx;

    if (tp->exp_panic_msg != 0) {
	if (test_panic_str == 0) {
	    msg_warn("test case '%s': got no panic, want: '%s'",
		     tp->label, tp->exp_panic_msg);
	    return (FAIL);
	}
	if (strcmp(vstring_str(test_panic_str), tp->exp_panic_msg) != 0) {
	    msg_warn("test case '%s': got '%s', want: '%s'",
		     tp->label, vstring_str(test_panic_str), tp->exp_panic_msg);
	    return (FAIL);
	}
	return (PASS);
    }
    if (test_panic_str != 0) {
	msg_warn("test case '%s': got '%s', want: no panic",
		 tp->label, vstring_str(test_panic_str));
	return (FAIL);
    }
    if (argvp->argc != tp->exp_argc) {
	msg_warn("test case '%s': got argc: %ld, want: %d",
		 tp->label, (long) argvp->argc, tp->exp_argc);
	return (FAIL);
    }
    if (argvp->argv[argvp->argc] != 0 && tp->terminate) {
	msg_warn("test case '%s': got unterminated, want: terminated", tp->label);
	return (FAIL);
    }
    for (idx = 0; idx < argvp->argc; idx++) {
	if (strcmp(argvp->argv[idx], tp->exp_argv[idx]) != 0) {
	    msg_warn("test case '%s': index %d: got '%s', want: '%s'",
		     tp->label, idx, argvp->argv[idx], tp->exp_argv[idx]);
	    return (FAIL);
	}
    }
    return (PASS);
}

 /*
  * The test cases. TODO: argv_addn with good and bad string length.
  */
static const TEST_CASE test_cases[] = {
    {"multiple strings, unterminated array",
	{"foo", "baz", "bar", 0}, 0, test_argv_populate,
	0, 3, {"foo", "baz", "bar", 0}
    },
    {"multiple strings, terminated array",
	{"foo", "baz", "bar", 0}, TERMINATE_ARRAY, test_argv_populate,
	0, 3, {"foo", "baz", "bar", 0}
    },
    {"distinct strings, sorted array",
	{"foo", "baz", "bar", 0}, 0, test_argv_sort,
	0, 3, {"bar", "baz", "foo", 0}
    },
    {"duplicate strings, sorted array",
	{"foo", "baz", "baz", "bar", 0}, 0, test_argv_sort,
	0, 4, {"bar", "baz", "baz", "foo", 0}
    },
    {"duplicate strings, sorted, uniqued-middle elements",
	{"foo", "baz", "baz", "bar", 0}, 0, test_argv_sort_uniq,
	0, 3, {"bar", "baz", "foo", 0}
    },
    {"duplicate strings, sorted, uniqued-first elements",
	{"foo", "bar", "baz", "bar", 0}, 0, test_argv_sort_uniq,
	0, 3, {"bar", "baz", "foo", 0}
    },
    {"duplicate strings, sorted, uniqued-last elements",
	{"foo", "foo", "baz", "bar", 0}, 0, test_argv_sort_uniq,
	0, 3, {"bar", "baz", "foo", 0}
    },
    {"multiple strings, truncate array by one",
	{"foo", "baz", "bar", 0}, 0, test_argv_good_truncate,
	0, 2, {"foo", "baz", 0}
    },
    {"multiple strings, truncate whole array",
	{"foo", "baz", "bar", 0}, 0, test_argv_good_truncate,
	0, 0, {"foo", "baz", 0}
    },
    {"multiple strings, bad truncate",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_truncate,
	"argv_truncate: bad length -1"
    },
    {"multiple strings, insert one at good position",
	{"foo", "baz", "bar", 0}, 0, test_argv_good_insert,
	0, 4, {"foo", "new", "baz", "bar", 0}
    },
    {"multiple strings, insert one at bad position",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_insert1,
	"argv_insert_one bad position: -1"
    },
    {"multiple strings, insert one at bad position",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_insert2,
	"argv_insert_one bad position: 100"
    },
    {"multiple strings, replace one at good position",
	{"foo", "baz", "bar", 0}, 0, test_argv_good_replace,
	0, 3, {"foo", "new", "bar", 0}
    },
    {"multiple strings, replace one at bad position",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_replace1,
	"argv_replace_one bad position: -1"
    },
    {"multiple strings, replace one at bad position",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_replace2,
	"argv_replace_one bad position: 100"
    },
    {"multiple strings, delete one at negative position",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_delete1,
	"argv_delete bad range: (start=-1 count=1)"
    },
    {"multiple strings, delete with bad range end",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_delete2,
	"argv_delete bad range: (start=0 count=-1)"
    },
    {"multiple strings, delete at too large position",
	{"foo", "baz", "bar", 0}, 0, test_argv_bad_delete3,
	"argv_delete bad range: (start=100 count=1)"
    },
    0,
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
	int     test_failed;
	ARGV   *argvp;

	argvp = argv_alloc(1);
	if (setjmp(test_panic_jbuf) == 0)
	    tp->populate_fn(tp, argvp);
	test_failed = test_argv_verify(tp, argvp);
	if (test_failed) {
	    msg_info("%s: FAIL", tp->label);
	    fail++;
	} else {
	    msg_info("%s: PASS", tp->label);
	    pass++;
	}
	argv_free(argvp);
	if (test_panic_str) {
	    vstring_free(test_panic_str);
	    test_panic_str = 0;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}

#endif
