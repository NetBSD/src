/*	$NetBSD: rfc2047_code.c,v 1.2 2025/02/25 19:15:46 christos Exp $	*/

/*++
/* NAME
/*	rfc2047_code 3
/* SUMMARY
/*	non-ASCII header content encoding
/* SYNOPSIS
/*	#include <rfc2047_code.h>
/*
/*	int	rfc2047_encode(
/*	VSTRING	*result,
/*	int	header_context,
/*	const char *charset,
/*	const char *in,
/*	ssize_t	len,
/*	const char *out_separator)
/* DESCRIPTION
/*	rfc2047_encode() encodes the input for the specified header
/*	context, producing one or more encoded-word instances. The
/*	result is the string value of the result buffer, or null in case
/*	of error.
/*
/*	rfc2047_encode() uses quoted-printable if the input is shorter
/*	than 20 bytes, or if fewer than 1/2 of the input bytes need to
/*	be encoded; otherwise it uses base64.
/*
/*	rfc2047_encode() limits the length of an encoded-word as required
/*	by RFC 2047, and produces as many encoded-word instances as
/*	needed, separated with a caller-specified separator.
/*
/*	Arguments:
/* .IP result
/*	The buffer that the output will overwrite. The result is
/*	null-terminated.
/* .IP header_context
/*	Specify one of:
/* .RS
/* .IP RFC2047_HEADER_CONTEXT_COMMENT
/*	The result will be used as 'comment' text, i.e. the result will
/*	later be enclosed with "(" and ")".
/* .IP RFC2047_HEADER_CONTEXT_PHRASE
/*	The result will be used as 'phrase' text, for example, the full
/*	name for an email address. The input must not be a quoted string.
/*	rfc2047_encode() logs a warning and returns null if the input
/*	begins with a double quote.
/* .RE
/* .IP charset
/*	Null-terminated character array with a valid character set name.
/*	rfc2047_encode() logs a warning and returns null if a charset
/*	value does not meet RFC 2047 syntax requirements.
/* .IP in
/*	Pointer to input storage.
/* .IP len
/*	The number of input bytes. rfc2047_encode() logs a warning and
/*	returns null if this length is < 1.
/* .IP out_separator
/*	Folding white-space text that will be inserted between
/*	multiple encoded-word instances in rfc2047_encode()
/*	output. rfc2047_encode() logs a warning and returns null if this
/*	argument does not specify whitespace text.
/* DIAGNOSTICS
/*	rfc2047_encode() returns null in case of error. See above for
/*	specific error cases.
/* BUGS
/*	The thresholds for switching from quoted-printable to base64
/*	encoding are arbitrary. What is optimal for one observer may be
/*	suspicious for a different one.
/* SEE ALSO
/*	RFC 2047, Message Header Extensions for Non-ASCII Text
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>
#include <ctype.h>

 /*
  * Utility library.
  */
#include <stringops.h>
#include <base64_code.h>
#include <msg.h>

 /*
  * Global library.
  */
#include <lex_822.h>
#include <rfc2047_code.h>

 /*
  * The general form of an encoded-word is =?charset?encoding?encoded-text?=
  * where charset and encoding are case insensitive, and encoding is B or Q.
  */

 /*
  * What ASCII characters are allowed in the 'charset' or 'encoding' tokens
  * of an encoded-word.
  */
#define RFC2047_ESPECIALS_STR	"()<>@,;:\\\"/[]?.="
#define RFC2047_ALLOWED_TOKEN_CHAR(c) \
    (isascii(c) && !iscntrl(c) && ch != ' ' \
	&& !strchr(RFC2047_ESPECIALS_STR, ch))

 /*
  * Common definitions for the base64 and quoted-printable encoders.
  */
#define ENC_WORD_MAX_LEN	75	/* =?charset?encoding?encoded?= */
#define ENC_WORD_PROLOG_FMT	"=?%s?%c?"	/* =?charset?encoding? */
#define ENC_WORD_EPILOG_STR	"?="
#define ENC_WORD_EPILOG_LEN	(sizeof(ENC_WORD_EPILOG_STR) - 1)
#define ENC_WORD_ENCODING_B64	'B'
#define ENC_WORD_ENCODING_QP	'Q'

 /*
  * Per RFC 2047 section 1, an encoded-word contains only printable ASCII
  * characters. Therefore, the quoted-printable encoder must always encode
  * ASCII SPACE, ASCII control characters, and non-ASCII byte values.
  * 
  * Per RFC 2047 section 4.2.(2), the quoted-printable encoder must always
  * encode the "=", "?" and "_" characters.
  * 
  * Per RFC 2047 section 5.(1) these rules are sufficient for header fields that
  * contain unstructured text such as Subject, X- custom headers, etc.
  */
#define QP_ENCODE_ASCII_NON_CNTRL	" =?_"

 /*
  * Per RFC 2047 section 5.(2) the quoted-printable encoder for comment text
  * also needs to encode "(", ")", and "\".
  */
#define QP_ENCODE_ASCII_NON_CNTRL_COMMENT_FIELD \
    (QP_ENCODE_ASCII_NON_CNTRL "()\\")

 /*
  * Per RFC 2047 section 5.(3) as amended by erratum(4): when used as a
  * replacement for a 'word' entity within a 'phrase', a "Q"-encoded
  * encoded-text can contain only alpha digit "!", "*", "+", "-", "/", "="
  * (only if followed by two hexadecimal digits), and "_". I.e not " # $ % &
  * ' ( ) , . : ; < > ? [ \ ] ^ ` { | } ~.
  * 
  * In other words they require encoding not only RFC 5322 specials and
  * characters reserved for encoded-word formatting, but they also disallow
  * characters that RFC 5322 allows in atext: # $ % & ' ^ ` { | } ~.
  */
#define QP_ENCODE_ASCII_NON_CNTRL_PHRASE_WORD \
    (QP_ENCODE_ASCII_NON_CNTRL LEX_822_SPECIALS "#$%&'?^`{|}~")

 /*
  * Considering the above discussion, why don't we just always base64 encode
  * the input? Because results from a web search suggest that some spam
  * filters penalize base64 encoding in headers.
  */

/* rfc204_b64_encode_append - encode header_entity with base64 */

static char *rfc204_b64_encode(VSTRING *result,
			               const char *charset,
			               const char *in, ssize_t len,
			               const char *out_separator)
{
    const char *cp;
    const char *in_end;

    /*
     * Convert the input into one or more encoded-word's of at most 75 bytes,
     * separated with a caller-specified output separator.
     */
    VSTRING_RESET(result);
    for (cp = in, in_end = in + len; cp < in_end; /* see below */ ) {
	ssize_t enc_word_start, enc_word_prolog_len, space_avail, todo;

	/* Start an encoded-word. */
	enc_word_start = VSTRING_LEN(result);
	vstring_sprintf_append(result, ENC_WORD_PROLOG_FMT, charset,
			       ENC_WORD_ENCODING_B64);
	enc_word_prolog_len = VSTRING_LEN(result) - enc_word_start;
	space_avail = ENC_WORD_MAX_LEN - ENC_WORD_EPILOG_LEN
	    - enc_word_prolog_len;

	/* Fill in the encoded-text portion. */
	todo = 3 * (space_avail >> 2);
	if (todo > in_end - cp)
	    todo = in_end - cp;
	(void) base64_encode_opt(result, cp, todo, BASE64_FLAG_APPEND);

	/* Finish the encoded-word. */
	(void) vstring_strcat(result, ENC_WORD_EPILOG_STR);

	/* Are we done yet? */
	if ((cp += todo) < in_end)
	    vstring_strcat(result, out_separator);
    }
    return (vstring_str(result));
}

/* rfc2047_qp_encode - encode header_entity with quoted-printable */

static char *rfc2047_qp_encode(VSTRING *result,
			               const char *charset,
			               const char *specials,
			               const char *in, ssize_t len,
			               const char *out_separator)
{
    const char *cp;
    const char *in_end;

    VSTRING_RESET(result);
    for (cp = in, in_end = in + len; cp < in_end; /* see below */ ) {
	ssize_t enc_word_start, enc_word_prolog_len, space_avail;
	int     ch;

	/* Start an encoded-word. */
	enc_word_start = VSTRING_LEN(result);
	vstring_sprintf_append(result, ENC_WORD_PROLOG_FMT, charset,
			       ENC_WORD_ENCODING_QP);
	enc_word_prolog_len = VSTRING_LEN(result) - enc_word_start;
	space_avail = ENC_WORD_MAX_LEN - ENC_WORD_EPILOG_LEN
	    - enc_word_prolog_len;

	/* Fill in the encoded-text portion. */
	for ( /* void */ ; cp < in_end && space_avail > 0; cp++) {
	    ch = *(unsigned char *) cp;
	    if (ch == 0x20) {
		VSTRING_ADDCH(result, '_');
		space_avail -= 1;
	    } else if (!isascii(ch) || iscntrl(ch) || strchr(specials, ch)) {
		if (space_avail < 3)
		    /* Try again in the next encoded-word. */
		    break;
		vstring_sprintf_append(result, "=%02X", ch);
		space_avail -= 3;
	    } else {
		VSTRING_ADDCH(result, ch);
		space_avail -= 1;
	    }
	}

	/* Finish the encoded-word and null-terminate the result. */
	(void) vstring_strcat(result, ENC_WORD_EPILOG_STR);

	/* Are we done yet? */
	if (cp < in_end)
	    vstring_strcat(result, out_separator);
    }
    return (vstring_str(result));
}

/* rfc2047_encode - encode header text field */

char   *rfc2047_encode(VSTRING *result, int header_context,
		               const char *charset,
		               const char *in, ssize_t len,
		               const char *out_separator)
{
    const char myname[] = "rfc2047_encode";
    const unsigned char *cp;
    int     ch;
    const char *qp_encoding_specials;

    /*
     * Sanity check the null-terminated charset name. This content is
     * configurable in Postfix, but there is no need to terminate the
     * process.
     */
    if (*charset == 0) {
	msg_warn("%s: encoder called with empty charset name", myname);
	return (0);
    }
    for (cp = (const unsigned char *) charset; (ch = *cp) != 0; cp++) {
	if (!RFC2047_ALLOWED_TOKEN_CHAR(ch)) {
	    msg_warn("%s: invalid character: 0x%x in charset name: '%s'",
		     myname, ch, charset);
	    return (0);
	}
    }

    /*
     * Sanity check the input size. This may be partly user controlled, so
     * don't panic.
     */
    if (len < 1) {
	msg_warn("%s: encoder called with empty input", myname);
	return (0);
    }
    if (!allspace(out_separator)) {
	msg_warn("%s: encoder called with non-whitespace separator: '%s'",
		 myname, out_separator);
	return (0);
    }

    /*
     * The RFC 2047 rules for quoted-printable encoding differ for comment
     * text and phrase text.
     */
    switch (header_context) {
    case RFC2047_HEADER_CONTEXT_COMMENT:
	qp_encoding_specials = QP_ENCODE_ASCII_NON_CNTRL_COMMENT_FIELD;
	break;
    case RFC2047_HEADER_CONTEXT_PHRASE:
	qp_encoding_specials = QP_ENCODE_ASCII_NON_CNTRL_PHRASE_WORD;
	if (*in == '"') {
	    msg_warn("%s: encoder called with quoted word as input: '%s'",
		     myname, in);
	    return (0);
	}
	break;
    default:
	msg_panic("%s: unexpected header_context: 0x%x",
		  myname, header_context);
    }

    /*
     * Choose between quoted-printable or base64 encoding. 
     *
     * Header strings are short, so making multiple passes over the input is
     * not a disaster. How many bytes would the encoder produce using
     * quoted-printable? We don't optimize for the shortest encoding but for
     * compromised readability. If the input is not short, and more than 1/2
     * of the input bytes need to be encoded, then the content is mostly not
     * printable ASCII, and quoted-printable output is mostly not readable.
     * But, naive spam filters may treat base64 in headers with suspicion.
     */
    if (len >= 20) {
	int     need_to_qp = 0;
	int     threshold = len / 2;
	const unsigned char *end = (const unsigned char *) in + len;

	for (cp = (const unsigned char *) in; cp < end; cp++) {
	    ch = *cp;
	    need_to_qp += (!isascii(ch) || isspace(ch) || iscntrl(ch)
			   || strchr(qp_encoding_specials, ch));
	    if (need_to_qp >= threshold)
		return (rfc204_b64_encode(result, charset, in, len,
					  out_separator));
	}
    }
    return (rfc2047_qp_encode(result, charset, qp_encoding_specials,
			      in, len, out_separator));
}

#ifdef TEST
#include <sys/select.h>			/* fd_set */
#include <stdlib.h>
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>

 /*
  * TODO(wietse) make this a proper VSTREAM interface. Instead of temporarily
  * swapping streams, we could temporarily swap the stream's write function.
  */

/* vstream_swap - capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

/* override the printable() call in msg output, it breaks our tests */

char   *printable_except(char *string, int replacement, const char *except)
{
    return (string);
}

 /*
  * Test structure. Some tests generate their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
    int     context;
    const char *charset;
    const char *input;
    ssize_t in_len;
    const char *out_separator;
    const char *exp_output;
    const char *exp_warning;
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

#define NO_OUTPUT	((char *) 0)

static int test_rfc2047_encode(const TEST_CASE *tp)
{
    static VSTRING *msg_buf;
    static VSTRING *result;
    VSTREAM *memory_stream;
    const char *got;

    if (msg_buf == 0) {
	msg_buf = vstring_alloc(100);
	result = vstring_alloc(100);
    }
    /* Run the test with custom STDERR stream. */
    VSTRING_RESET(msg_buf);
    VSTRING_TERMINATE(msg_buf);
    if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
	msg_fatal("open memory stream: %m");
    vstream_swap(VSTREAM_ERR, memory_stream);
    got = rfc2047_encode(result, tp->context, tp->charset, tp->input,
			 tp->in_len != 0 ? tp->in_len : strlen(tp->input),
			 tp->out_separator);
    vstream_swap(memory_stream, VSTREAM_ERR);
    if (vstream_fclose(memory_stream))
	msg_fatal("close memory stream: %m");

    /* Verify the results. */
    if (tp->exp_warning == 0 && VSTRING_LEN(msg_buf) > 0) {
	msg_warn("got warning ``%s'', want ``null''", vstring_str(msg_buf));
	return (FAIL);
    }
    if (tp->exp_warning != 0
	&& strcmp(vstring_str(msg_buf), tp->exp_warning) != 0) {
	msg_warn("got warning ``%s'', want ``%s''",
		 vstring_str(msg_buf), tp->exp_warning);
	return (FAIL);
    }
    if (!got == !tp->exp_warning) {
	msg_warn("got result ``%s'', and warning ``%s''",
		 got ? got : "null",
		 tp->exp_warning ? tp->exp_warning : "null");
	return (FAIL);
    }
    if (!got != !tp->exp_output) {
	msg_warn("got result ``%s'', want ``%s''",
		 got ? got : "null",
		 tp->exp_output ? tp->exp_output : "null");
	return (FAIL);
    }
    if (got && strcmp(got, tp->exp_output) != 0) {
	msg_warn("got result ``%s'', want ``%s''", got, tp->exp_output);
	return (FAIL);
    }
    return (PASS);
}

 /*
  * Input/output templates for charset tests.
  */
#define CHARSET_INPUT_TEMPLATE		"testme"
#define CHARSET_OUTPUT_SEPARATOR	" "
#define CHARSET_CHARSET_TEMPLATE	"utf-8%c"
#define CHARSET_ALLOW_TEMPLATE		"=?utf-8%c?Q?testme?="
#define CHARSET_REJECT_TEMPLATE \
	    "rfc2047_code: warning: rfc2047_encode: invalid character: " \
	    "0x%x in charset name: 'utf-8%c'\n"

static int validates_charset(const TEST_CASE *tp)
{
    VSTRING *charset;
    VSTRING *exp_output;
    VSTRING *exp_warning;
    TEST_CASE test;
    int     ch;
    int     ret;
    fd_set  asciimap;

#define SET_RANGE(map, min, max) do { \
	int _n; \
	for (_n = (min); _n <= (max); _n++) \
	    FD_SET(_n, (map)); \
    } while (0);
#define SET_SINGLE(map, n) do { \
	FD_SET(n, (map)); \
    } while (0);

    /*
     * Initialize a bitmap with characters that are allowed in a charset
     * name.
     */
    FD_ZERO(&asciimap);
    SET_SINGLE(&asciimap, '!');
    SET_RANGE(&asciimap, '#', '\'');
    SET_RANGE(&asciimap, '*', '+');
    SET_SINGLE(&asciimap, '-');
    SET_RANGE(&asciimap, '0', '9');
    SET_RANGE(&asciimap, 'A', 'Z');
    SET_RANGE(&asciimap, '^', '~');

    charset = vstring_alloc(10);
    exp_output = vstring_alloc(100);
    exp_warning = vstring_alloc(100);

    test.label = tp->label;
    test.action = 0;
    test.context = RFC2047_HEADER_CONTEXT_COMMENT;
    test.input = CHARSET_INPUT_TEMPLATE;
    test.in_len = 0;
    test.out_separator = CHARSET_OUTPUT_SEPARATOR;

    /* TODO: 0x0 in charset. */
    for (ret = 0, ch = 0x1; ret == 0 && ch <= 0xff; ch++) {
	vstring_sprintf(charset, CHARSET_CHARSET_TEMPLATE, ch);
	test.charset = vstring_str(charset);
	if (FD_ISSET(ch, &asciimap)) {
	    vstring_sprintf(exp_output, CHARSET_ALLOW_TEMPLATE, ch);
	    test.exp_output = vstring_str(exp_output);
	    test.exp_warning = 0;
	} else {
	    test.exp_output = 0;
	    vstring_sprintf(exp_warning, CHARSET_REJECT_TEMPLATE, ch, ch);
	    test.exp_warning = vstring_str(exp_warning);
	}
	ret = test_rfc2047_encode(&test);
    }

    vstring_free(charset);
    vstring_free(exp_output);
    vstring_free(exp_warning);
    return (ret == 0 ? PASS : FAIL);
}

 /*
  * Specify an explicit input length, so that we can input a null byte.
  */
#define ENCODE_INPUT_TEMPLATE	"testme %c"
#define ENCODE_INPUT_LEN	(sizeof(ENCODE_INPUT_TEMPLATE) - 2)
#define ENCODE_OUTPUT_SEPARATOR	"\n"
#define ENCODE_CHARSET		"utf-8"
#define ENCODE_AS_SELF_TEMPLATE	"=?utf-8?Q?testme_%c?="
#define ENCODE_AS_QP_TEMPLATE	"=?utf-8?Q?testme_=%02X?="
#define ENCODE_AS_CTRL_TEMPLATE	"=?utf-8?Q?testme_?="

 /*
  * TODO(wietse) split into 'encodes non-control text' and 'rejects control
  * text'1
  */

static int encodes_text(const char *label,
			        int context,
			        const char *specials)
{
    VSTRING *input;
    VSTRING *exp_output;
    TEST_CASE test;
    int     ch;
    int     ret;
    fd_set  asciimap;
    const char *cp;

    /*
     * Initialize a bitmap with characters that need no encoding.
     */
    FD_ZERO(&asciimap);
    SET_RANGE(&asciimap, 0x20, 0x7e);		/* ASCII non-controls */
    for (cp = specials; (ch = *(unsigned char *) cp) != 0; cp++)
	FD_CLR(ch, &asciimap);

    input = vstring_alloc(100);
    exp_output = vstring_alloc(100);

    /*
     * TODO(wietse) We don't use VSTRING_LEN(input) to determine how much
     * output was written. When the output contains a null byte,
     * VSTRING_LEN(input) will be smaller than ENCODE_INPUT_LEN because
     * vbuf_print() still supports systems without snprintf() and relies on
     * the equivalent of strlen() to find out how much was written.
     */
    test.label = label;
    test.action = 0;
    test.context = context;
    test.charset = "utf-8";
    test.in_len = ENCODE_INPUT_LEN;
    test.out_separator = ENCODE_OUTPUT_SEPARATOR;
    test.exp_warning = 0;

    for (ret = 0, ch = 0x0; ret == 0 && ch <= 0xff; ch++) {
	vstring_sprintf(input, ENCODE_INPUT_TEMPLATE, ch);
	test.input = vstring_str(input);
	if (FD_ISSET(ch, &asciimap)) {
	    vstring_sprintf(exp_output, ENCODE_AS_SELF_TEMPLATE, ch);
	} else if (ch == ' ') {
	    vstring_sprintf(exp_output, ENCODE_AS_SELF_TEMPLATE, '_');
	} else {
	    vstring_sprintf(exp_output, ENCODE_AS_QP_TEMPLATE, ch);
	}
	test.exp_output = vstring_str(exp_output);
	ret = test_rfc2047_encode(&test);
    }

    vstring_free(input);
    vstring_free(exp_output);
    return (ret == 0 ? PASS : FAIL);
}

static int encodes_comment_text(const TEST_CASE *tp)
{
    const char specials[] = {
	/* RFC 2047 encoded-word formatting. */
	' ', '_', '=', '?',

	/* Not allowed in RFC 5322 ctext. */
	'(', ')', '\\',

	0,
    };

    return (encodes_text(tp->label, RFC2047_HEADER_CONTEXT_COMMENT, specials));
}

static int encodes_phrase_text(const TEST_CASE *tp)
{
    const char specials[] = {
	/* RFC 2047 encoded-word formatting. */
	' ', '_', '=', '?',

	/* RFC 5322 specials. */
	'"', '(', ')', ',', '.', ':', ';', '<', '>', '@', '[', '\\', ']',

	/* Not allowed in RFC 2047 section 5(3). */
	'#', '$', '%', '&', '\'', '^', '`', '{', '|', '}', '~',

	0,
    };

    return (encodes_text(tp->label, RFC2047_HEADER_CONTEXT_PHRASE, specials));
}

static const TEST_CASE test_cases[] = {

    /*
     * Smoke test. If this fails then all bets are off.
     */
    {"comment_needs_no_encoding",
	test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_COMMENT, "utf-8",
	"testme", 0, " ", "=?utf-8?Q?testme?="
    },

    /*
     * Charset validation.
     */
    {"validates_charset_not_empty", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_COMMENT, /* charset= */ "",
	"testme", 0, " ", NO_OUTPUT, "rfc2047_code: warning: rfc2047_encode: "
	"encoder called with empty charset name\n"
    },
    {"validates_charset", validates_charset},
    {"encodes_comment_text", encodes_comment_text},
    {"encodes_long_comment_text", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_COMMENT, "utf-8",
	"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
	0, "\n",
	"=?utf-8?Q?AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAA?=\n=?utf-8?Q?AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
	"AAAAAAAAAAAAAAAAAAAAAAAA?="
    },

    /*
     * Encoding validation.
     */
    {"validates_input_not_empty", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_COMMENT, "utf-8",
	 /* input= */ "", 0, " ", NO_OUTPUT, "rfc2047_code: warning: "
	"rfc2047_encode: encoder called with empty input\n"
    },
    {"validates_whitespace_separator", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_COMMENT, "utf-8",
	"whatever", 0, "foo", NO_OUTPUT, "rfc2047_code: warning: "
	"rfc2047_encode: encoder called with non-whitespace separator: "
	"'foo'\n"
    },
    {"validates_non_quoted_word", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"\"whatever", 0, " ", NO_OUTPUT, "rfc2047_code: warning: "
	"rfc2047_encode: encoder called with quoted word as input: "
	"'\"whatever'\n"
    },
    {"encodes_phrase_text", encodes_phrase_text},
    {"encodes_long_phrase_text", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"000102030405060708091011121314151617181920212223242526272829"
	"303132333435363738394041424344454647484950515253545556575859"
	"606162636465666768697071727374757677787980818283848586878889",
	0, "\n",
	"=?utf-8?Q?00010203040506070809101112131415161718192021222324"
	"2526272829303?=\n=?utf-8?Q?132333435363738394041424344454647"
	"484950515253545556575859606162?=\n=?utf-8?Q?6364656667686970"
	"71727374757677787980818283848586878889?="
    },
    {"encodes_mostly_ascii_as_qp", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"some non-ascii Δημοσ embedded in ascii", 0, "\n",
	"=?utf-8?Q?some_non-ascii_=CE=94=CE=B7=CE=BC=CE=BF=CF=83_embe"
	"dded_in_ascii?="
    },
    {"encodes_whole_qp_triplets_1x", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"x some small amount of non-ascii Δημοσ embedded in ascii", 0, "\n",
	"=?utf-8?Q?x_some_small_amount_of_non-ascii_=CE=94=CE=B7=CE=BC=CE=B"
	"F=CF=83?=\n=?utf-8?Q?_embedded_in_ascii?="
    },
    {"encodes_whole_qp_triplets_2x", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"xx some small amount of non-ascii Δημοσ embedded in ascii", 0, "\n",
	"=?utf-8?Q?xx_some_small_amount_of_non-ascii_=CE=94=CE=B7=CE=BC=CE=B"
	"F=CF?=\n=?utf-8?Q?=83_embedded_in_ascii?="
    },
    {"encodes_mostly_non_ascii_as_b64", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"mostly non-ascii Δημοσθένους", 0, "\n",
	"=?utf-8?B?bW9zdGx5IG5vbi1hc2NpaSDOlM63zrzOv8+DzrjhvbPOvc6/z4XPgg==?="
    },
    {"encodes_whole_b64_quads_4x", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"xxxx mostly non-ascii Δημοσθένους", 0, "\n",
	"=?utf-8?B?eHh4eCBtb3N0bHkgbm9uLWFzY2lpIM6UzrfOvM6/z4POuOG9s869zr/Phc"
	"+C?="
    },
    {"encodes_whole_b64_quads_5x", test_rfc2047_encode,
	RFC2047_HEADER_CONTEXT_PHRASE, "utf-8",
	"xxxxx mostly non-ascii Δημοσθένους", 0, "\n",
	"=?utf-8?B?eHh4eHggbW9zdGx5IG5vbi1hc2NpaSDOlM63zrzOv8+DzrjhvbPOvc6/z4"
	"XP?=\n=?utf-8?B?gg==?="
    },
    {0},
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
	int     test_failed;

	msg_info("RUN  %s", tp->label);
	test_failed = tp->action(tp);
	if (test_failed) {
	    msg_info("FAIL %s", tp->label);
	    fail++;
	} else {
	    msg_info("PASS %s", tp->label);
	    pass++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}

#endif
