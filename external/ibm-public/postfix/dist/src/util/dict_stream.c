/*	$NetBSD: dict_stream.c,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	dict_stream 3
/* SUMMARY
/*
/* SYNOPSIS
/*	#include <dict.h>
/*
/*	VSTREAM *dict_stream_open(
/*	const char *dict_type,
/*	const char *mapname,
/*	int	open_flags,
/*	int	dict_flags,
/*	struct stat * st,
/*	VSTRING **why)
/* DESCRIPTION
/*	dict_stream_open() opens a dictionary, which can be specified
/*	as a file name, or as inline text enclosed with {}. If successful,
/*	dict_stream_open() returns a non-null VSTREAM pointer. Otherwise,
/*	it returns an error text through the why argument, allocating
/*	storage for the error text if the why argument points to a
/*	null pointer.
/*
/*	When the dictionary file is specified inline, dict_stream_open()
/*	removes the outer {} from the mapname value, and removes leading
/*	or trailing comma or whitespace from the result. It then expects
/*	to find zero or more rules enclosed in {}, separated by comma
/*	and/or whitespace. dict_stream() writes each rule as one text
/*	line to an in-memory stream, without its enclosing {} and without
/*	leading or trailing whitespace. The result value is a VSTREAM
/*	pointer for the in-memory stream that can be read as a regular
/*	file.
/* .sp
/*	inline-file = "{" 0*(0*wsp-comma rule-spec) 0*wsp-comma "}"
/* .sp
/*	rule-spec = "{" 0*wsp rule-text 0*wsp "}"
/* .sp
/*	rule-text = any text containing zero or more balanced {}
/* .sp
/*	wsp-comma = wsp | ","
/* .sp
/*	wsp = whitespace
/*
/*	Arguments:
/* .IP dict_type
/* .IP open_flags
/* .IP dict_flags
/*	The same as with dict_open(3).
/* .IP mapname
/*	Pathname of a file with dictionary content, or inline dictionary
/*	content as specified above.
/* .IP st
/*	File metadata with the file owner, or fake metadata with the
/*	real UID and GID of the dict_stream_open() caller. This is 
/*	used for "taint" tracking (zero=trusted, non-zero=untrusted).
/* IP why
/*	Pointer to pointer to error message storage. dict_stream_open()
/*	updates this storage when reporting an error, and allocates
/*	memory if why points to a null pointer.
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
#include <dict.h>
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstring.h>

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* dict_inline_to_multiline - convert inline map spec to multiline text */

static char *dict_inline_to_multiline(VSTRING *vp, const char *mapname)
{
    char   *saved_name = mystrdup(mapname);
    char   *bp = saved_name;
    char   *cp;
    char   *err = 0;

    VSTRING_RESET(vp);
    /* Strip the {} from the map "name". */
    err = extpar(&bp, CHARS_BRACE, EXTPAR_FLAG_NONE);
    /* Extract zero or more rules inside {}. */
    while (err == 0 && (cp = mystrtokq(&bp, CHARS_COMMA_SP, CHARS_BRACE)) != 0)
	if ((err = extpar(&cp, CHARS_BRACE, EXTPAR_FLAG_STRIP)) == 0)
	    /* Write rule to in-memory file. */
	    vstring_sprintf_append(vp, "%s\n", cp);
    VSTRING_TERMINATE(vp);
    myfree(saved_name);
    return (err);
}

/* dict_stream_open - open inline configuration or configuration file */

VSTREAM *dict_stream_open(const char *dict_type, const char *mapname,
			          int open_flags, int dict_flags,
			          struct stat * st, VSTRING **why)
{
    VSTRING *inline_buf = 0;
    VSTREAM *map_fp;
    char   *err = 0;

#define RETURN_0_WITH_REASON(...) do { \
	if (*why == 0) \
	    *why = vstring_alloc(100); \
	vstring_sprintf(*why, __VA_ARGS__); \
	if (inline_buf != 0) \
	   vstring_free(inline_buf); \
	if (err != 0) \
	    myfree(err); \
	return (0); \
    } while (0)

    if (mapname[0] == CHARS_BRACE[0]) {
	inline_buf = vstring_alloc(100);
	if ((err = dict_inline_to_multiline(inline_buf, mapname)) != 0)
	    RETURN_0_WITH_REASON("%s map: %s", dict_type, err);
	map_fp = vstream_memopen(inline_buf, O_RDONLY);
	vstream_control(map_fp, VSTREAM_CTL_OWN_VSTRING, VSTREAM_CTL_END);
	st->st_uid = getuid();			/* geteuid()? */
	st->st_gid = getgid();			/* getegid()? */
	return (map_fp);
    } else {
	if ((map_fp = vstream_fopen(mapname, open_flags, 0)) == 0)
	    RETURN_0_WITH_REASON("open %s: %m", mapname);
	if (fstat(vstream_fileno(map_fp), st) < 0)
	    msg_fatal("fstat %s: %m", mapname);
	return (map_fp);
    }
}

#ifdef TEST

#include <string.h>

int     main(int argc, char **argv)
{
    struct testcase {
	const char *title;
	const char *mapname;		/* starts with brace */
	const char *expect_err;		/* null or message */
	const char *expect_cont;	/* null or content */
    };

#define EXP_NOERR	0
#define EXP_NOCONT	0

#define STRING_OR(s, text_if_null) ((s) ? (s) : (text_if_null))
#define DICT_TYPE_TEST	"test"

    const char rule_spec_error[] = DICT_TYPE_TEST " map: "
    "syntax error after '}' in \"{blah blah}x\"";
    const char inline_config_error[] = DICT_TYPE_TEST " map: "
    "syntax error after '}' in \"{{foo bar}, {blah blah}}x\"";
    struct testcase testcases[] = {
	{"normal",
	    "{{foo bar}, {blah blah}}", EXP_NOERR, "foo bar\nblah blah\n"
	},
	{"trims leading/trailing wsp around rule-text",
	    "{{ foo bar }, { blah blah }}", EXP_NOERR, "foo bar\nblah blah\n"
	},
	{"trims leading/trailing comma-wsp around rule-spec",
	    "{, ,{foo bar}, {blah blah}, ,}", EXP_NOERR, "foo bar\nblah blah\n"
	},
	{"empty inline-file",
	    "{, }", EXP_NOERR, ""
	},
	{"propagates extpar error for inline-file",
	    "{{foo bar}, {blah blah}}x", inline_config_error, EXP_NOCONT
	},
	{"propagates extpar error for rule-spec",
	    "{{foo bar}, {blah blah}x}", rule_spec_error, EXP_NOCONT
	},
	0,
    };
    struct testcase *tp;
    VSTRING *act_err = 0;
    VSTRING *act_cont = vstring_alloc(100);
    VSTREAM *fp;
    struct stat st;
    ssize_t exp_len;
    ssize_t act_len;
    int     pass;
    int     fail;

    for (pass = fail = 0, tp = testcases; tp->title; tp++) {
	int     test_passed = 0;

	msg_info("RUN test case %ld %s", (long) (tp - testcases), tp->title);

#if 0
	msg_info("title=%s", tp->title);
	msg_info("mapname=%s", tp->mapname);
	msg_info("expect_err=%s", STRING_OR_NULL(tp->expect_err));
	msg_info("expect_cont=%s", STRINGOR_NULL(tp->expect_cont));
#endif

	if (act_err)
	    VSTRING_RESET(act_err);
	fp = dict_stream_open(DICT_TYPE_TEST, tp->mapname, O_RDONLY,
			      0, &st, &act_err);
	if (fp) {
	    if (tp->expect_err) {
		msg_warn("test case %s: got stream, expected error", tp->title);
	    } else if (!tp->expect_err && act_err && LEN(act_err) > 0) {
		msg_warn("test case %s: got error '%s', expected noerror",
			 tp->title, STR(act_err));
	    } else if (!tp->expect_cont) {
		msg_warn("test case %s: got stream, expected nostream",
			 tp->title);
	    } else {
		exp_len = strlen(tp->expect_cont);
		if ((act_len = vstream_fread_buf(fp, act_cont, 2 * exp_len)) < 0) {
		    msg_warn("test case %s: content read error", tp->title);
		} else {
		    VSTRING_TERMINATE(act_cont);
		    if (strcmp(tp->expect_cont, STR(act_cont)) != 0) {
			msg_warn("test case %s: got content '%s', expected '%s'",
				 tp->title, STR(act_cont), tp->expect_cont);
		    } else {
			test_passed = 1;
		    }
		}
	    }
	} else {
	    if (!tp->expect_err) {
		msg_warn("test case %s: got nostream, expected noerror",
			 tp->title);
	    } else if (tp->expect_cont) {
		msg_warn("test case %s: got nostream, expected stream",
			 tp->title);
	    } else if (strcmp(STR(act_err), tp->expect_err) != 0) {
		msg_warn("test case %s: got error '%s', expected '%s'",
			 tp->title, STR(act_err), tp->expect_err);
	    } else {
		test_passed = 1;
	    }

	}
	if (test_passed) {
	    msg_info("PASS test %ld", (long) (tp - testcases));
	    pass++;
	} else {
	    msg_info("FAIL test %ld", (long) (tp - testcases));
	    fail++;
	}
	if (fp)
	    vstream_fclose(fp);
    }
    if (act_err)
	vstring_free(act_err);
    vstring_free(act_cont);
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}

#endif					/* TEST */
