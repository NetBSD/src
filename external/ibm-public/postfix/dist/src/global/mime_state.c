/*	$NetBSD: mime_state.c,v 1.1.1.1.2.2 2009/09/15 06:02:49 snj Exp $	*/

/*++
/* NAME
/*	mime_state 3
/* SUMMARY
/*	MIME parser state machine
/* SYNOPSIS
/*	#include <mime_state.h>
/*
/*	MIME_STATE *mime_state_alloc(flags, head_out, head_end,
/*					 body_out, body_end,
/*					 err_print, context)
/*	int	flags;
/*	void	(*head_out)(void *ptr, int header_class,
/*				const HEADER_OPTS *header_info,
/*				VSTRING *buf, off_t offset);
/*	void	(*head_end)(void *ptr);
/*	void	(*body_out)(void *ptr, int rec_type,
/*				const char *buf, ssize_t len,
/*				off_t offset);
/*	void	(*body_end)(void *ptr);
/*	void	(*err_print)(void *ptr, int err_flag, const char *text)
/*	void	*context;
/*
/*	int	mime_state_update(state, rec_type, buf, len)
/*	MIME_STATE *state;
/*	int	rec_type;
/*	const char *buf;
/*	ssize_t	len;
/*
/*	MIME_STATE *mime_state_free(state)
/*	MIME_STATE *state;
/*
/*	const char *mime_state_error(error_code)
/*	int	error_code;
/*
/*	typedef struct {
/* .in +4
/*		const int code;		/* internal error code */
/*		const char *dsn;	/* RFC 3463 */
/*		const char *text;	/* descriptive text */
/* .in -4
/*	} MIME_STATE_DETAIL;
/*
/*	const MIME_STATE_DETAIL *mime_state_detail(error_code)
/*	int	error_code;
/* DESCRIPTION
/*	This module implements a one-pass MIME processor with optional
/*	8-bit to quoted-printable conversion.
/*
/*	In order to fend off denial of service attacks, message headers
/*	are truncated at or above var_header_limit bytes, message boundary
/*	strings are truncated at var_boundary_len bytes, and the multipart
/*	nesting level is limited to var_mime_maxdepth levels.
/*
/*	mime_state_alloc() creates a MIME state machine. The machine
/*	is delivered in its initial state, expecting content type
/*	text/plain, 7-bit data.
/*
/*	mime_state_update() updates the MIME state machine according
/*	to the input record type and the record content.
/*	The result value is the bit-wise OR of zero or more of the following:
/* .IP MIME_ERR_TRUNC_HEADER
/*	A message header was longer than var_header_limit bytes.
/* .IP MIME_ERR_NESTING
/*	The MIME structure was nested more than var_mime_maxdepth levels.
/* .IP MIME_ERR_8BIT_IN_HEADER
/*	A message header contains 8-bit data. This is always illegal.
/* .IP MIME_ERR_8BIT_IN_7BIT_BODY
/*	A MIME header specifies (or defaults to) 7-bit content, but the
/*	corresponding message body or body parts contain 8-bit content.
/* .IP MIME_ERR_ENCODING_DOMAIN
/*	An entity of type "message" or "multipart" specifies the wrong
/*	content transfer encoding domain, or specifies a transformation
/*	(quoted-printable, base64) instead of a domain (7bit, 8bit,
/*	or binary).
/* .PP
/*	mime_state_free() releases storage for a MIME state machine,
/*	and conveniently returns a null pointer.
/*
/*	mime_state_error() returns a string representation for the
/*	specified error code. When multiple errors are specified it
/*	reports what it deems the most serious one.
/*
/*	mime_state_detail() returns a table entry with error
/*	information for the specified error code. When multiple
/*	errors are specified it reports what it deems the most
/*	serious one.
/*
/*	Arguments:
/* .IP body_out
/*	The output routine for body lines. It receives unmodified input
/*	records, or the result of 8-bit -> 7-bit conversion.
/* .IP body_end
/*	A null pointer, or a pointer to a routine that is called after
/*	the last input record is processed.
/* .IP buf
/*	Buffer with the content of a logical or physical message record.
/* .IP context
/*	Caller context that is passed on to the head_out and body_out
/*	routines.
/* .IP enc_type
/*	The content encoding: MIME_ENC_7BIT or MIME_ENC_8BIT.
/* .IP err_print
/*	Null pointer, or pointer to a function that is called with
/*	arguments: the application context, the error type, and the
/*	offending input. Only one instance per error type is reported.
/* .IP flags
/*	Special processing options. Specify the bit-wise OR of zero or
/*	more of the following:
/* .RS
/* .IP MIME_OPT_DISABLE_MIME
/*	Pay no attention to Content-* message headers, and switch to
/*	message body state at the end of the primary message headers.
/* .IP MIME_OPT_REPORT_TRUNC_HEADER
/*	Report errors that set the MIME_ERR_TRUNC_HEADER error flag
/*	(see above).
/* .IP MIME_OPT_REPORT_8BIT_IN_HEADER
/*	Report errors that set the MIME_ERR_8BIT_IN_HEADER error
/*	flag (see above). This rarely stops legitimate mail.
/* .IP MIME_OPT_REPORT_8BIT_IN_7BIT_BODY
/*	Report errors that set the MIME_ERR_8BIT_IN_7BIT_BODY error
/*	flag (see above). This currently breaks Majordomo mail that is
/*	forwarded for approval, because Majordomo does not propagate
/*	MIME type information from the enclosed message to the message
/*	headers of the request for approval.
/* .IP MIME_OPT_REPORT_ENCODING_DOMAIN
/*	Report errors that set the MIME_ERR_ENCODING_DOMAIN error
/*	flag (see above).
/* .IP MIME_OPT_REPORT_NESTING
/*	Report errors that set the MIME_ERR_NESTING error flag
/*	(see above).
/* .IP MIME_OPT_RECURSE_ALL_MESSAGE
/*	Recurse into message/anything types other than message/rfc822.
/*	This feature can detect "bad" information in headers of
/*	message/partial and message/external-body types. It must
/*	not be used with 8-bit -> 7-bit MIME transformations.
/* .IP MIME_OPT_DOWNGRADE
/*	Transform content that claims to be 8-bit into quoted-printable.
/*	Where appropriate, update Content-Transfer-Encoding: message
/*	headers.
/* .RE
/* .sp
/*	For convenience, MIME_OPT_NONE requests no special processing.
/* .IP header_class
/*	Specifies where a message header is located.
/* .RS
/* .IP MIME_HDR_PRIMARY
/*	In the primary message header section.
/* .IP MIME_HDR_MULTIPART
/*	In the header section after a multipart boundary string.
/* .IP MIME_HDR_NESTED
/*	At the start of a nested (e.g., message/rfc822) message.
/* .RE
/* .sp
/*	For convenience, the macros MIME_HDR_FIRST and MIME_HDR_LAST
/*	specify the range of MIME_HDR_MUMBLE macros.
/* .sp
/*	To find out if something is a MIME header at the beginning
/*	of an RFC 822 message or an attached message, look at the
/*	header_info argument.
/* .IP header_info
/*	Null pointer or information about the message header, see
/*	header_opts(3).
/* .IP head_out
/*	The output routine that is invoked for outputting a message header.
/*	A multi-line header is passed as one chunk of text with embedded
/*	newlines.
/*	It is the responsibility of the output routine to break the text
/*	at embedded newlines, and to break up long text between newlines
/*	into multiple output records.
/*	Note: an output routine is explicitly allowed to modify the text.
/* .IP head_end
/*	A null pointer, or a pointer to a routine that is called after
/*	the last message header in the first header block is processed.
/* .IP len
/*	Length of non-VSTRING input buffer.
/* .IP offset
/*	The offset in bytes from the start of the current block of message
/*	headers or body lines. Line boundaries are counted as one byte.
/* .IP rec_type
/*	The input record type as defined in rec_type(3h). State is
/*	updated for text records (REC_TYPE_NORM or REC_TYPE_CONT).
/*	Some input records are stored internally in order to reconstruct
/*	multi-line input.  Upon receipt of any non-text record type, all
/*	stored input is flushed and the state is set to "body".
/* .IP state
/*	MIME parser state created with mime_state_alloc().
/* BUGS
/*	NOTE: when the end of headers is reached, mime_state_update()
/*	may execute up to three call-backs before returning to the
/*	caller: head_out(), head_end(), and body_out() or body_end().
/*	As long as call-backs return no result, it is up to the
/*	call-back routines to check if a previous call-back experienced
/*	an error.
/*
/*	Different mail user agents treat malformed message boundary
/*	strings in different ways. The Postfix MIME processor cannot
/*	be bug-compatible with everything.
/*
/*	This module will not glue together multipart boundary strings that
/*	span multiple input records.
/*
/*	This module will not glue together RFC 2231 formatted (boundary)
/*	parameter values. RFC 2231 claims compatibility with existing
/*	MIME processors. Splitting boundary strings is not backwards
/*	compatible.
/*
/*	The "8-bit data inside 7-bit body" test is myopic. It is not aware
/*	of any enclosing (message or multipart) encoding information.
/*
/*	If the input ends in data other than a hard line break, this module
/*	will add a hard line break of its own. No line break is added to
/*	empty input.
/*
/*	This code recognizes the obsolete form "headername :" but will
/*	normalize it to the canonical form "headername:". Leaving the
/*	obsolete form alone would cause too much trouble with existing code
/*	that expects only the normalized form.
/* SEE ALSO
/*	msg(3) diagnostics interface
/*	header_opts(3) header information lookup
/*	RFC 822 (ARPA Internet Text Messages)
/*	RFC 2045 (MIME: Format of internet message bodies)
/*	RFC 2046 (MIME: Media types)
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This code was implemented from scratch after reading the RFC
/*	documents. This was a relatively straightforward effort with
/*	few if any surprises. Victor Duchovni of Morgan Stanley shared
/*	his experiences with ambiguities in real-life MIME implementations.
/*	Liviu Daia of the Romanian Academy shared his insights in some
/*	of the darker corners.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <rec_type.h>
#include <is_header.h>
#include <header_opts.h>
#include <mail_params.h>
#include <header_token.h>
#include <lex_822.h>
#include <mime_state.h>

/* Application-specific. */

 /*
  * Mime parser stack element for multipart content.
  */
typedef struct MIME_STACK {
    int     def_ctype;			/* default content type */
    int     def_stype;			/* default content subtype */
    char   *boundary;			/* boundary string */
    ssize_t bound_len;			/* boundary length */
    struct MIME_STACK *next;		/* linkage */
} MIME_STACK;

 /*
  * Mime parser state.
  */
#define MIME_MAX_TOKEN		3	/* tokens per attribute */

struct MIME_STATE {

    /*
     * Volatile members.
     */
    int     curr_state;			/* header/body state */
    int     curr_ctype;			/* last or default content type */
    int     curr_stype;			/* last or default content subtype */
    int     curr_encoding;		/* last or default content encoding */
    int     curr_domain;		/* last or default encoding unit */
    VSTRING *output_buffer;		/* headers, quoted-printable body */
    int     prev_rec_type;		/* previous input record type */
    int     nesting_level;		/* safety */
    MIME_STACK *stack;			/* for composite types */
    HEADER_TOKEN token[MIME_MAX_TOKEN];	/* header token array */
    VSTRING *token_buffer;		/* header parser scratch buffer */
    int     err_flags;			/* processing errors */
    off_t   head_offset;		/* offset in header block */
    off_t   body_offset;		/* offset in body block */

    /*
     * Static members.
     */
    int     static_flags;		/* static processing options */
    MIME_STATE_HEAD_OUT head_out;	/* header output routine */
    MIME_STATE_ANY_END head_end;	/* end of primary header routine */
    MIME_STATE_BODY_OUT body_out;	/* body output routine */
    MIME_STATE_ANY_END body_end;	/* end of body output routine */
    MIME_STATE_ERR_PRINT err_print;	/* error report */
    void   *app_context;		/* application context */
};

 /*
  * Content types and subtypes that we care about, either because we have to,
  * or because we want to filter out broken MIME messages.
  */
#define MIME_CTYPE_OTHER	0
#define MIME_CTYPE_TEXT		1
#define MIME_CTYPE_MESSAGE	2
#define MIME_CTYPE_MULTIPART	3

#define MIME_STYPE_OTHER	0
#define MIME_STYPE_PLAIN	1
#define MIME_STYPE_RFC822	2
#define MIME_STYPE_PARTIAL	3
#define MIME_STYPE_EXTERN_BODY	4

 /*
  * MIME parser states. We steal from the public interface.
  */
#define MIME_STATE_PRIMARY	MIME_HDR_PRIMARY	/* primary headers */
#define MIME_STATE_MULTIPART	MIME_HDR_MULTIPART	/* after --boundary */
#define MIME_STATE_NESTED	MIME_HDR_NESTED	/* message/rfc822 */
#define MIME_STATE_BODY		(MIME_HDR_NESTED + 1)

#define SET_MIME_STATE(ptr, state, ctype, stype, encoding, domain) do { \
	(ptr)->curr_state = (state); \
	(ptr)->curr_ctype = (ctype); \
	(ptr)->curr_stype = (stype); \
	(ptr)->curr_encoding = (encoding); \
	(ptr)->curr_domain = (domain); \
	if ((state) == MIME_STATE_BODY) \
	    (ptr)->body_offset = 0; \
	else \
	    (ptr)->head_offset = 0; \
    } while (0)

#define SET_CURR_STATE(ptr, state) do { \
	(ptr)->curr_state = (state); \
	if ((state) == MIME_STATE_BODY) \
	    (ptr)->body_offset = 0; \
	else \
	    (ptr)->head_offset = 0; \
    } while (0)

 /*
  * MIME encodings and domains. We intentionally use the same codes for
  * encodings and domains, so that we can easily find out whether a content
  * transfer encoding header specifies a domain or whether it specifies
  * domain+encoding, which is illegal for multipart/any and message/any.
  */
typedef struct MIME_ENCODING {
    const char *name;			/* external representation */
    int     encoding;			/* internal representation */
    int     domain;			/* subset of encoding */
} MIME_ENCODING;

#define MIME_ENC_QP		1	/* encoding + domain */
#define MIME_ENC_BASE64		2	/* encoding + domain */
 /* These are defined in mime_state.h as part of the external interface. */
#ifndef MIME_ENC_7BIT
#define MIME_ENC_7BIT		7	/* domain only */
#define MIME_ENC_8BIT		8	/* domain only */
#define MIME_ENC_BINARY		9	/* domain only */
#endif

static const MIME_ENCODING mime_encoding_map[] = {	/* RFC 2045 */
    "7bit", MIME_ENC_7BIT, MIME_ENC_7BIT,	/* domain */
    "8bit", MIME_ENC_8BIT, MIME_ENC_8BIT,	/* domain */
    "binary", MIME_ENC_BINARY, MIME_ENC_BINARY,	/* domain */
    "base64", MIME_ENC_BASE64, MIME_ENC_7BIT,	/* encoding */
    "quoted-printable", MIME_ENC_QP, MIME_ENC_7BIT,	/* encoding */
    0,
};

 /*
  * Silly Little Macros.
  */
#define STR(x)		vstring_str(x)
#define LEN(x)		VSTRING_LEN(x)
#define END(x)		vstring_end(x)
#define CU_CHAR_PTR(x)	((const unsigned char *) (x))

#define REPORT_ERROR_LEN(state, err_type, text, len) do { \
	if ((state->err_flags & err_type) == 0) { \
	    if (state->err_print != 0) \
		state->err_print(state->app_context, err_type, text, len); \
	    state->err_flags |= err_type; \
	} \
    } while (0)

#define REPORT_ERROR(state, err_type, text) do { \
	const char *_text = text; \
	ssize_t _len = strlen(text); \
	REPORT_ERROR_LEN(state, err_type, _text, _len); \
    } while (0)

#define REPORT_ERROR_BUF(state, err_type, buf) \
    REPORT_ERROR_LEN(state, err_type, STR(buf), LEN(buf))


 /*
  * Outputs and state changes are interleaved, so we must maintain separate
  * offsets for header and body segments.
  */
#define HEAD_OUT(ptr, info, len) do { \
	if ((ptr)->head_out) { \
	    (ptr)->head_out((ptr)->app_context, (ptr)->curr_state, \
			    (info), (ptr)->output_buffer, (ptr)->head_offset); \
	    (ptr)->head_offset += (len) + 1; \
	} \
    } while(0)

#define BODY_OUT(ptr, rec_type, text, len) do { \
	if ((ptr)->body_out) { \
	    (ptr)->body_out((ptr)->app_context, (rec_type), \
			    (text), (len), (ptr)->body_offset); \
	    (ptr)->body_offset += (len) + 1; \
	} \
    } while(0)

/* mime_state_push - push boundary onto stack */

static void mime_state_push(MIME_STATE *state, int def_ctype, int def_stype,
			            const char *boundary)
{
    MIME_STACK *stack;

    /*
     * RFC 2046 mandates that a boundary string be up to 70 characters long.
     * Some MTAs, including Postfix, include the fully-qualified MTA name
     * which can be longer, so we are willing to handle boundary strings that
     * exceed the RFC specification. We allow for message headers of up to
     * var_header_limit characters. In order to avoid denial of service, we
     * have to impose a configurable limit on the amount of text that we are
     * willing to store as a boundary string. Despite this truncation way we
     * will still correctly detect all intermediate boundaries and all the
     * message headers that follow those boundaries.
     */
    state->nesting_level += 1;
    stack = (MIME_STACK *) mymalloc(sizeof(*stack));
    stack->def_ctype = def_ctype;
    stack->def_stype = def_stype;
    if ((stack->bound_len = strlen(boundary)) > var_mime_bound_len)
	stack->bound_len = var_mime_bound_len;
    stack->boundary = mystrndup(boundary, stack->bound_len);
    stack->next = state->stack;
    state->stack = stack;
    if (msg_verbose)
	msg_info("PUSH boundary %s", stack->boundary);
}

/* mime_state_pop - pop boundary from stack */

static void mime_state_pop(MIME_STATE *state)
{
    MIME_STACK *stack;

    if ((stack = state->stack) == 0)
	msg_panic("mime_state_pop: there is no stack");
    if (msg_verbose)
	msg_info("POP boundary %s", stack->boundary);
    state->nesting_level -= 1;
    state->stack = stack->next;
    myfree(stack->boundary);
    myfree((char *) stack);
}

/* mime_state_alloc - create MIME state machine */

MIME_STATE *mime_state_alloc(int flags,
			             MIME_STATE_HEAD_OUT head_out,
			             MIME_STATE_ANY_END head_end,
			             MIME_STATE_BODY_OUT body_out,
			             MIME_STATE_ANY_END body_end,
			             MIME_STATE_ERR_PRINT err_print,
			             void *context)
{
    MIME_STATE *state;

    state = (MIME_STATE *) mymalloc(sizeof(*state));

    /* Volatile members. */
    state->err_flags = 0;
    state->body_offset = 0;			/* XXX */
    SET_MIME_STATE(state, MIME_STATE_PRIMARY,
		   MIME_CTYPE_TEXT, MIME_STYPE_PLAIN,
		   MIME_ENC_7BIT, MIME_ENC_7BIT);
    state->output_buffer = vstring_alloc(100);
    state->prev_rec_type = 0;
    state->stack = 0;
    state->token_buffer = vstring_alloc(1);

    /* Static members. */
    state->static_flags = flags;
    state->head_out = head_out;
    state->head_end = head_end;
    state->body_out = body_out;
    state->body_end = body_end;
    state->err_print = err_print;
    state->app_context = context;
    return (state);
}

/* mime_state_free - destroy MIME state machine */

MIME_STATE *mime_state_free(MIME_STATE *state)
{
    vstring_free(state->output_buffer);
    while (state->stack)
	mime_state_pop(state);
    if (state->token_buffer)
	vstring_free(state->token_buffer);
    myfree((char *) state);
    return (0);
}

/* mime_state_content_type - process content-type header */

static void mime_state_content_type(MIME_STATE *state,
				            const HEADER_OPTS *header_info)
{
    const char *cp;
    ssize_t tok_count;
    int     def_ctype;
    int     def_stype;

#define TOKEN_MATCH(tok, text) \
    ((tok).type == HEADER_TOK_TOKEN && strcasecmp((tok).u.value, (text)) == 0)

#define RFC2045_TSPECIALS	"()<>@,;:\\\"/[]?="

#define PARSE_CONTENT_TYPE_HEADER(state, ptr) \
    header_token(state->token, MIME_MAX_TOKEN, \
	state->token_buffer, ptr, RFC2045_TSPECIALS, ';')

    cp = STR(state->output_buffer) + strlen(header_info->name) + 1;
    if ((tok_count = PARSE_CONTENT_TYPE_HEADER(state, &cp)) > 0) {

	/*
	 * text/whatever. Right now we don't really care if it is plain or
	 * not, but we may want to recognize subtypes later, and then this
	 * code can serve as an example.
	 */
	if (TOKEN_MATCH(state->token[0], "text")) {
	    state->curr_ctype = MIME_CTYPE_TEXT;
	    if (tok_count >= 3
		&& state->token[1].type == '/'
		&& TOKEN_MATCH(state->token[2], "plain"))
		state->curr_stype = MIME_STYPE_PLAIN;
	    else
		state->curr_stype = MIME_STYPE_OTHER;
	    return;
	}

	/*
	 * message/whatever body parts start with another block of message
	 * headers that we may want to look at. The partial and external-body
	 * subtypes cannot be subjected to 8-bit -> 7-bit conversion, so we
	 * must properly recognize them.
	 */
	if (TOKEN_MATCH(state->token[0], "message")) {
	    state->curr_ctype = MIME_CTYPE_MESSAGE;
	    state->curr_stype = MIME_STYPE_OTHER;
	    if (tok_count >= 3
		&& state->token[1].type == '/') {
		if (TOKEN_MATCH(state->token[2], "rfc822"))
		    state->curr_stype = MIME_STYPE_RFC822;
		else if (TOKEN_MATCH(state->token[2], "partial"))
		    state->curr_stype = MIME_STYPE_PARTIAL;
		else if (TOKEN_MATCH(state->token[2], "external-body"))
		    state->curr_stype = MIME_STYPE_EXTERN_BODY;
	    }
	    return;
	}

	/*
	 * multipart/digest has default content type message/rfc822,
	 * multipart/whatever has default content type text/plain.
	 */
	if (TOKEN_MATCH(state->token[0], "multipart")) {
	    state->curr_ctype = MIME_CTYPE_MULTIPART;
	    if (tok_count >= 3
		&& state->token[1].type == '/'
		&& TOKEN_MATCH(state->token[2], "digest")) {
		def_ctype = MIME_CTYPE_MESSAGE;
		def_stype = MIME_STYPE_RFC822;
	    } else {
		def_ctype = MIME_CTYPE_TEXT;
		def_stype = MIME_STYPE_PLAIN;
	    }

	    /*
	     * Yes, this is supposed to capture multiple boundary strings,
	     * which are illegal and which could be used to hide content in
	     * an implementation dependent manner. The code below allows us
	     * to find embedded message headers as long as the sender uses
	     * only one of these same-level boundary strings.
	     * 
	     * Yes, this is supposed to ignore the boundary value type.
	     */
	    while ((tok_count = PARSE_CONTENT_TYPE_HEADER(state, &cp)) >= 0) {
		if (tok_count >= 3
		    && TOKEN_MATCH(state->token[0], "boundary")
		    && state->token[1].type == '=') {
		    if (state->nesting_level > var_mime_maxdepth) {
			if (state->static_flags & MIME_OPT_REPORT_NESTING)
			    REPORT_ERROR_BUF(state, MIME_ERR_NESTING,
					     state->output_buffer);
		    } else {
			mime_state_push(state, def_ctype, def_stype,
					state->token[2].u.value);
		    }
		}
	    }
	}
	return;
    }

    /*
     * other/whatever.
     */
    else {
	state->curr_ctype = MIME_CTYPE_OTHER;
	return;
    }
}

/* mime_state_content_encoding - process content-transfer-encoding header */

static void mime_state_content_encoding(MIME_STATE *state,
				             const HEADER_OPTS *header_info)
{
    const char *cp;
    const MIME_ENCODING *cmp;

#define PARSE_CONTENT_ENCODING_HEADER(state, ptr) \
    header_token(state->token, 1, state->token_buffer, ptr, (char *) 0, 0)

    /*
     * Do content-transfer-encoding header. Never set the encoding domain to
     * something other than 7bit, 8bit or binary, even if we don't recognize
     * the input.
     */
    cp = STR(state->output_buffer) + strlen(header_info->name) + 1;
    if (PARSE_CONTENT_ENCODING_HEADER(state, &cp) > 0
	&& state->token[0].type == HEADER_TOK_TOKEN) {
	for (cmp = mime_encoding_map; cmp->name != 0; cmp++) {
	    if (strcasecmp(state->token[0].u.value, cmp->name) == 0) {
		state->curr_encoding = cmp->encoding;
		state->curr_domain = cmp->domain;
		break;
	    }
	}
    }
}

/* mime_state_enc_name - encoding to printable form */

static const char *mime_state_enc_name(int encoding)
{
    const MIME_ENCODING *cmp;

    for (cmp = mime_encoding_map; cmp->name != 0; cmp++)
	if (encoding == cmp->encoding)
	    return (cmp->name);
    return ("unknown");
}

/* mime_state_downgrade - convert 8-bit data to quoted-printable */

static void mime_state_downgrade(MIME_STATE *state, int rec_type,
				         const char *text, ssize_t len)
{
    static char hexchars[] = "0123456789ABCDEF";
    const unsigned char *cp;
    int     ch;

#define QP_ENCODE(buffer, ch) { \
	VSTRING_ADDCH(buffer, '='); \
	VSTRING_ADDCH(buffer, hexchars[(ch >> 4) & 0xff]); \
	VSTRING_ADDCH(buffer, hexchars[ch & 0xf]); \
    }

    /*
     * Insert a soft line break when the output reaches a critical length
     * before we reach a hard line break.
     */
    for (cp = CU_CHAR_PTR(text); cp < CU_CHAR_PTR(text + len); cp++) {
	/* Critical length before hard line break. */
	if (LEN(state->output_buffer) > 72) {
	    VSTRING_ADDCH(state->output_buffer, '=');
	    VSTRING_TERMINATE(state->output_buffer);
	    BODY_OUT(state, REC_TYPE_NORM,
		     STR(state->output_buffer),
		     LEN(state->output_buffer));
	    VSTRING_RESET(state->output_buffer);
	}
	/* Append the next character. */
	ch = *cp;
	if ((ch < 32 && ch != '\t') || ch == '=' || ch > 126) {
	    QP_ENCODE(state->output_buffer, ch);
	} else {
	    VSTRING_ADDCH(state->output_buffer, ch);
	}
    }

    /*
     * Flush output after a hard line break (i.e. the end of a REC_TYPE_NORM
     * record). Fix trailing whitespace as per the RFC: in the worst case,
     * the output length will grow from 73 characters to 75 characters.
     */
    if (rec_type == REC_TYPE_NORM) {
	if (LEN(state->output_buffer) > 0
	    && ((ch = END(state->output_buffer)[-1]) == ' ' || ch == '\t')) {
	    vstring_truncate(state->output_buffer,
			     LEN(state->output_buffer) - 1);
	    QP_ENCODE(state->output_buffer, ch);
	}
	VSTRING_TERMINATE(state->output_buffer);
	BODY_OUT(state, REC_TYPE_NORM,
		 STR(state->output_buffer),
		 LEN(state->output_buffer));
	VSTRING_RESET(state->output_buffer);
    }
}

/* mime_state_update - update MIME state machine */

int     mime_state_update(MIME_STATE *state, int rec_type,
			          const char *text, ssize_t len)
{
    int     input_is_text = (rec_type == REC_TYPE_NORM
			     || rec_type == REC_TYPE_CONT);
    MIME_STACK *sp;
    const HEADER_OPTS *header_info;
    const unsigned char *cp;

#define SAVE_PREV_REC_TYPE_AND_RETURN_ERR_FLAGS(state, rec_type) do { \
	state->prev_rec_type = rec_type; \
	return (state->err_flags); \
    } while (0)

    /*
     * Be sure to flush any partial output line that might still be buffered
     * up before taking any other "end of input" actions.
     */
    if (!input_is_text && state->prev_rec_type == REC_TYPE_CONT)
	mime_state_update(state, REC_TYPE_NORM, "", 0);

    /*
     * This message state machine is kept simple for the sake of robustness.
     * Standards evolve over time, and we want to be able to correctly
     * process messages that are not yet defined. This state machine knows
     * about headers and bodies, understands that multipart/whatever has
     * multiple body parts with a header and body, and that message/whatever
     * has message headers at the start of a body part.
     */
    switch (state->curr_state) {

	/*
	 * First, deal with header information that we have accumulated from
	 * previous input records. Discard text that does not fit in a header
	 * buffer. Our limit is quite generous; Sendmail will refuse mail
	 * with only 32kbyte in all the message headers combined.
	 */
    case MIME_STATE_PRIMARY:
    case MIME_STATE_MULTIPART:
    case MIME_STATE_NESTED:
	if (LEN(state->output_buffer) > 0) {
	    if (input_is_text) {
		if (state->prev_rec_type == REC_TYPE_CONT) {
		    if (LEN(state->output_buffer) < var_header_limit) {
			vstring_strncat(state->output_buffer, text, len);
		    } else {
			if (state->static_flags & MIME_OPT_REPORT_TRUNC_HEADER)
			    REPORT_ERROR_BUF(state, MIME_ERR_TRUNC_HEADER,
					     state->output_buffer);
		    }
		    SAVE_PREV_REC_TYPE_AND_RETURN_ERR_FLAGS(state, rec_type);
		}
		if (IS_SPACE_TAB(*text)) {
		    if (LEN(state->output_buffer) < var_header_limit) {
			vstring_strcat(state->output_buffer, "\n");
			vstring_strncat(state->output_buffer, text, len);
		    } else {
			if (state->static_flags & MIME_OPT_REPORT_TRUNC_HEADER)
			    REPORT_ERROR_BUF(state, MIME_ERR_TRUNC_HEADER,
					     state->output_buffer);
		    }
		    SAVE_PREV_REC_TYPE_AND_RETURN_ERR_FLAGS(state, rec_type);
		}
	    }

	    /*
	     * The input is (the beginning of) another message header, or is
	     * not a message header, or is not even a text record. With no
	     * more input to append to this saved header, do output
	     * processing and reset the saved header buffer. Hold on to the
	     * content transfer encoding header if we have to do a 8->7
	     * transformation, because the proper information depends on the
	     * content type header: message and multipart require a domain,
	     * leaf entities have either a transformation or a domain.
	     */
	    if (LEN(state->output_buffer) > 0) {
		header_info = header_opts_find(STR(state->output_buffer));
		if (!(state->static_flags & MIME_OPT_DISABLE_MIME)
		    && header_info != 0) {
		    if (header_info->type == HDR_CONTENT_TYPE)
			mime_state_content_type(state, header_info);
		    if (header_info->type == HDR_CONTENT_TRANSFER_ENCODING)
			mime_state_content_encoding(state, header_info);
		}
		if ((state->static_flags & MIME_OPT_REPORT_8BIT_IN_HEADER) != 0
		    && (state->err_flags & MIME_ERR_8BIT_IN_HEADER) == 0) {
		    for (cp = CU_CHAR_PTR(STR(state->output_buffer));
			 cp < CU_CHAR_PTR(END(state->output_buffer)); cp++)
			if (*cp & 0200) {
			    REPORT_ERROR_BUF(state, MIME_ERR_8BIT_IN_HEADER,
					     state->output_buffer);
			    break;
			}
		}
		/* Output routine is explicitly allowed to change the data. */
		if (header_info == 0
		    || header_info->type != HDR_CONTENT_TRANSFER_ENCODING
		    || (state->static_flags & MIME_OPT_DOWNGRADE) == 0
		    || state->curr_domain == MIME_ENC_7BIT)
		    HEAD_OUT(state, header_info, len);
		state->prev_rec_type = 0;
		VSTRING_RESET(state->output_buffer);
	    }
	}

	/*
	 * With past header information moved out of the way, proceed with a
	 * clean slate.
	 */
	if (input_is_text) {
	    ssize_t header_len;

	    /*
	     * See if this input is (the beginning of) a message header.
	     * 
	     * Normalize obsolete "name space colon" syntax to "name colon".
	     * Things would be too confusing otherwise.
	     * 
	     * Don't assume that the input is null terminated.
	     */
	    if ((header_len = is_header_buf(text, len)) > 0) {
		vstring_strncpy(state->output_buffer, text, header_len);
		for (text += header_len, len -= header_len;
		     len > 0 && IS_SPACE_TAB(*text);
		     text++, len--)
		     /* void */ ;
		vstring_strncat(state->output_buffer, text, len);
		SAVE_PREV_REC_TYPE_AND_RETURN_ERR_FLAGS(state, rec_type);
	    }
	}

	/*
	 * This input terminates a block of message headers. When converting
	 * 8-bit to 7-bit mail, this is the right place to emit the correct
	 * content-transfer-encoding header. With message or multipart we
	 * specify 7bit, with leaf entities we specify quoted-printable.
	 * 
	 * We're not going to convert non-text data into base 64. If they send
	 * arbitrary binary data as 8-bit text, then the data is already
	 * broken beyond recovery, because the Postfix SMTP server sanitizes
	 * record boundaries, treating broken record boundaries as CRLF.
	 * 
	 * Clear the output buffer, we will need it for storage of the
	 * conversion result.
	 */
	if ((state->static_flags & MIME_OPT_DOWNGRADE)
	    && state->curr_domain != MIME_ENC_7BIT) {
	    if (state->curr_ctype == MIME_CTYPE_MESSAGE
		|| state->curr_ctype == MIME_CTYPE_MULTIPART)
		cp = CU_CHAR_PTR("7bit");
	    else
		cp = CU_CHAR_PTR("quoted-printable");
	    vstring_sprintf(state->output_buffer,
			    "Content-Transfer-Encoding: %s", cp);
	    HEAD_OUT(state, (HEADER_OPTS *) 0, len);
	    VSTRING_RESET(state->output_buffer);
	}

	/*
	 * This input terminates a block of message headers. Call the
	 * optional header end routine at the end of the first header block.
	 */
	if (state->curr_state == MIME_STATE_PRIMARY && state->head_end)
	    state->head_end(state->app_context);

	/*
	 * This is the right place to check if the sender specified an
	 * appropriate identity encoding (7bit, 8bit, binary) for multipart
	 * and for message.
	 */
	if (state->static_flags & MIME_OPT_REPORT_ENCODING_DOMAIN) {
	    if (state->curr_ctype == MIME_CTYPE_MESSAGE) {
		if (state->curr_stype == MIME_STYPE_PARTIAL
		    || state->curr_stype == MIME_STYPE_EXTERN_BODY) {
		    if (state->curr_domain != MIME_ENC_7BIT)
			REPORT_ERROR(state, MIME_ERR_ENCODING_DOMAIN,
				 mime_state_enc_name(state->curr_encoding));
		} else {
		    if (state->curr_encoding != state->curr_domain)
			REPORT_ERROR(state, MIME_ERR_ENCODING_DOMAIN,
				 mime_state_enc_name(state->curr_encoding));
		}
	    } else if (state->curr_ctype == MIME_CTYPE_MULTIPART) {
		if (state->curr_encoding != state->curr_domain)
		    REPORT_ERROR(state, MIME_ERR_ENCODING_DOMAIN,
				 mime_state_enc_name(state->curr_encoding));
	    }
	}

	/*
	 * Find out if the next body starts with its own message headers. In
	 * agressive mode, examine headers of partial and external-body
	 * messages. Otherwise, treat such headers as part of the "body". Set
	 * the proper encoding information for the multipart prolog.
	 * 
	 * XXX We parse headers inside message/* content even when the encoding
	 * is invalid (encoding != domain). With base64 we won't recognize
	 * any headers, and with quoted-printable we won't recognize MIME
	 * boundary strings, but the MIME processor will still resynchronize
	 * when it runs into the higher-level boundary string at the end of
	 * the message/* content. Although we will treat some headers as body
	 * text, we will still do a better job than if we were treating the
	 * entire message/* content as body text.
	 * 
	 * XXX This changes state to MIME_STATE_NESTED and then outputs a body
	 * line, so that the body offset is not properly reset.
	 * 
	 * Don't assume that the input is null terminated.
	 */
	if (input_is_text) {
	    if (len == 0) {
		state->body_offset = 0;		/* XXX */
		if (state->curr_ctype == MIME_CTYPE_MESSAGE) {
		    if (state->curr_stype == MIME_STYPE_RFC822
		    || (state->static_flags & MIME_OPT_RECURSE_ALL_MESSAGE))
			SET_MIME_STATE(state, MIME_STATE_NESTED,
				       MIME_CTYPE_TEXT, MIME_STYPE_PLAIN,
				       MIME_ENC_7BIT, MIME_ENC_7BIT);
		    else
			SET_CURR_STATE(state, MIME_STATE_BODY);
		} else if (state->curr_ctype == MIME_CTYPE_MULTIPART) {
		    SET_MIME_STATE(state, MIME_STATE_BODY,
				   MIME_CTYPE_OTHER, MIME_STYPE_OTHER,
				   MIME_ENC_7BIT, MIME_ENC_7BIT);
		} else {
		    SET_CURR_STATE(state, MIME_STATE_BODY);
		}
	    }

	    /*
	     * Invalid input. Force output of one blank line and jump to the
	     * body state, leaving all other state alone.
	     * 
	     * We don't break legitimate mail by inserting a blank line
	     * separator between primary headers and a non-empty body. Many
	     * MTA's don't even record the presence or absence of this
	     * separator, nor does the Milter protocol pass it on to Milter
	     * applications.
	     * 
	     * XXX We don't insert a blank line separator into attachments, to
	     * avoid breaking digital signatures. Postfix shall not do a
	     * worse mail delivery job than MTAs that can't even parse MIME.
	     * We switch to body state anyway, to avoid treating body text as
	     * header text, and mis-interpreting or truncating it. The code
	     * below for initial From_ lines is for educational purposes.
	     * 
	     * Sites concerned about MIME evasion can use a MIME normalizer.
	     * Postfix has a different mission.
	     */
	    else {
		if (msg_verbose)
		    msg_info("garbage in %s header",
		    state->curr_state == MIME_STATE_MULTIPART ? "multipart" :
		       state->curr_state == MIME_STATE_PRIMARY ? "primary" :
			 state->curr_state == MIME_STATE_NESTED ? "nested" :
			     "other");
		switch (state->curr_state) {
		case MIME_STATE_PRIMARY:
		    BODY_OUT(state, REC_TYPE_NORM, "", 0);
		    SET_CURR_STATE(state, MIME_STATE_BODY);
		    break;
#if 0
		case MIME_STATE_NESTED:
		    if (state->body_offset <= 1
			&& rec_type == REC_TYPE_NORM
			&& len > 7
			&& (strncmp(text + (*text == '>'), "From ", 5) == 0
			    || strncmp(text, "=46rom ", 7) == 0))
			break;
		    /* FALLTHROUGH */
#endif
		default:
		    SET_CURR_STATE(state, MIME_STATE_BODY);
		    break;
		}
	    }
	}

	/*
	 * This input is not text. Go to body state, unconditionally.
	 */
	else {
	    SET_CURR_STATE(state, MIME_STATE_BODY);
	}
	/* FALLTHROUGH */

	/*
	 * Body text. Look for message boundaries, and recover from missing
	 * boundary strings. Missing boundaries can happen in agressive mode
	 * with text/rfc822-headers or with message/partial. Ignore non-space
	 * cruft after --boundary or --boundary--, because some MUAs do, and
	 * because only perverse software would take advantage of this to
	 * escape detection. We have to ignore trailing cruft anyway, because
	 * our saved copy of the boundary string may have been truncated for
	 * safety reasons.
	 * 
	 * Optionally look for 8-bit data in content that was announced as, or
	 * that defaults to, 7-bit. Unfortunately, we cannot turn this on by
	 * default. Majordomo sends requests for approval that do not
	 * propagate the MIME information from the enclosed message to the
	 * message headers of the approval request.
	 * 
	 * Set the proper state information after processing a message boundary
	 * string.
	 * 
	 * Don't look for boundary strings at the start of a continued record.
	 * 
	 * Don't assume that the input is null terminated.
	 */
    case MIME_STATE_BODY:
	if (input_is_text) {
	    if ((state->static_flags & MIME_OPT_REPORT_8BIT_IN_7BIT_BODY) != 0
		&& state->curr_encoding == MIME_ENC_7BIT
		&& (state->err_flags & MIME_ERR_8BIT_IN_7BIT_BODY) == 0) {
		for (cp = CU_CHAR_PTR(text); cp < CU_CHAR_PTR(text + len); cp++)
		    if (*cp & 0200) {
			REPORT_ERROR_LEN(state, MIME_ERR_8BIT_IN_7BIT_BODY,
					 text, len);
			break;
		    }
	    }
	    if (state->stack && state->prev_rec_type != REC_TYPE_CONT
		&& len > 2 && text[0] == '-' && text[1] == '-') {
		for (sp = state->stack; sp != 0; sp = sp->next) {
		    if (len >= 2 + sp->bound_len &&
		      strncmp(text + 2, sp->boundary, sp->bound_len) == 0) {
			while (sp != state->stack)
			    mime_state_pop(state);
			if (len >= 4 + sp->bound_len &&
			  strncmp(text + 2 + sp->bound_len, "--", 2) == 0) {
			    mime_state_pop(state);
			    SET_MIME_STATE(state, MIME_STATE_BODY,
					 MIME_CTYPE_OTHER, MIME_STYPE_OTHER,
					   MIME_ENC_7BIT, MIME_ENC_7BIT);
			} else {
			    SET_MIME_STATE(state, MIME_STATE_MULTIPART,
					   sp->def_ctype, sp->def_stype,
					   MIME_ENC_7BIT, MIME_ENC_7BIT);
			}
			break;
		    }
		}
	    }
	    /* Put last for consistency with header output routine. */
	    if ((state->static_flags & MIME_OPT_DOWNGRADE)
		&& state->curr_domain != MIME_ENC_7BIT)
		mime_state_downgrade(state, rec_type, text, len);
	    else
		BODY_OUT(state, rec_type, text, len);
	}

	/*
	 * The input is not a text record. Inform the application that this
	 * is the last opportunity to send any pending output.
	 */
	else {
	    if (state->body_end)
		state->body_end(state->app_context);
	}
	SAVE_PREV_REC_TYPE_AND_RETURN_ERR_FLAGS(state, rec_type);

	/*
	 * Oops. This can't happen.
	 */
    default:
	msg_panic("mime_state_update: unknown state: %d", state->curr_state);
    }
}

 /*
  * Mime error to (DSN, text) mapping. Order matters; more serious errors
  * must precede less serious errors, because the error-to-text conversion
  * can report only one error.
  */
static const MIME_STATE_DETAIL mime_err_detail[] = {
    MIME_ERR_NESTING, "5.6.0", "MIME nesting exceeds safety limit",
    MIME_ERR_TRUNC_HEADER, "5.6.0", "message header length exceeds safety limit",
    MIME_ERR_8BIT_IN_HEADER, "5.6.0", "improper use of 8-bit data in message header",
    MIME_ERR_8BIT_IN_7BIT_BODY, "5.6.0", "improper use of 8-bit data in message body",
    MIME_ERR_ENCODING_DOMAIN, "5.6.0", "invalid message/* or multipart/* encoding domain",
    0,
};

/* mime_state_error - error code to string */

const char *mime_state_error(int error_code)
{
    const MIME_STATE_DETAIL *mp;

    if (error_code == 0)
	msg_panic("mime_state_error: there is no error");
    for (mp = mime_err_detail; mp->code; mp++)
	if (mp->code & error_code)
	    return (mp->text);
    msg_panic("mime_state_error: unknown error code %d", error_code);
}

/* mime_state_detail - error code to table entry with assorted data */

const MIME_STATE_DETAIL *mime_state_detail(int error_code)
{
    const MIME_STATE_DETAIL *mp;

    if (error_code == 0)
	msg_panic("mime_state_detail: there is no error");
    for (mp = mime_err_detail; mp->code; mp++)
	if (mp->code & error_code)
	    return (mp);
    msg_panic("mime_state_detail: unknown error code %d", error_code);
}

#ifdef TEST

#include <stdlib.h>
#include <stringops.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <rec_streamlf.h>

 /*
  * Stress test the REC_TYPE_CONT/NORM handling, but don't break header
  * labels.
  */
/*#define REC_LEN	40*/

#define REC_LEN	1024

static void head_out(void *context, int class, const HEADER_OPTS *unused_info,
		             VSTRING *buf, off_t offset)
{
    VSTREAM *stream = (VSTREAM *) context;

    vstream_fprintf(stream, "%s %ld\t|%s\n",
		    class == MIME_HDR_PRIMARY ? "MAIN" :
		    class == MIME_HDR_MULTIPART ? "MULT" :
		    class == MIME_HDR_NESTED ? "NEST" :
		    "ERROR", (long) offset, STR(buf));
}

static void head_end(void *context)
{
    VSTREAM *stream = (VSTREAM *) context;

    vstream_fprintf(stream, "HEADER END\n");
}

static void body_out(void *context, int rec_type, const char *buf, ssize_t len,
		             off_t offset)
{
    VSTREAM *stream = (VSTREAM *) context;

    vstream_fprintf(stream, "BODY %c %ld\t|", rec_type, (long) offset);
    vstream_fwrite(stream, buf, len);
    if (rec_type == REC_TYPE_NORM)
	VSTREAM_PUTC('\n', stream);
}

static void body_end(void *context)
{
    VSTREAM *stream = (VSTREAM *) context;

    vstream_fprintf(stream, "BODY END\n");
}

static void err_print(void *unused_context, int err_flag,
		              const char *text, ssize_t len)
{
    msg_warn("%s: %.*s", mime_state_error(err_flag),
	     len < 100 ? (int) len : 100, text);
}

int     var_header_limit = 2000;
int     var_mime_maxdepth = 20;
int     var_mime_bound_len = 2000;

int     main(int unused_argc, char **argv)
{
    int     rec_type;
    int     last = 0;
    VSTRING *buf;
    MIME_STATE *state;
    int     err;

    /*
     * Initialize.
     */
#define MIME_OPTIONS \
	    (MIME_OPT_REPORT_8BIT_IN_7BIT_BODY \
	    | MIME_OPT_REPORT_8BIT_IN_HEADER \
	    | MIME_OPT_REPORT_ENCODING_DOMAIN \
	    | MIME_OPT_REPORT_TRUNC_HEADER \
	    | MIME_OPT_REPORT_NESTING \
	    | MIME_OPT_DOWNGRADE)

    msg_vstream_init(basename(argv[0]), VSTREAM_OUT);
    msg_verbose = 1;
    buf = vstring_alloc(10);
    state = mime_state_alloc(MIME_OPTIONS,
			     head_out, head_end,
			     body_out, body_end,
			     err_print,
			     (void *) VSTREAM_OUT);

    /*
     * Main loop.
     */
    do {
	rec_type = rec_streamlf_get(VSTREAM_IN, buf, REC_LEN);
	VSTRING_TERMINATE(buf);
	err = mime_state_update(state, last = rec_type, STR(buf), LEN(buf));
	vstream_fflush(VSTREAM_OUT);
    } while (rec_type > 0);

    /*
     * Error reporting.
     */
    if (err & MIME_ERR_TRUNC_HEADER)
	msg_warn("message header length exceeds safety limit");
    if (err & MIME_ERR_NESTING)
	msg_warn("MIME nesting exceeds safety limit");
    if (err & MIME_ERR_8BIT_IN_HEADER)
	msg_warn("improper use of 8-bit data in message header");
    if (err & MIME_ERR_8BIT_IN_7BIT_BODY)
	msg_warn("improper use of 8-bit data in message body");
    if (err & MIME_ERR_ENCODING_DOMAIN)
	msg_warn("improper message/* or multipart/* encoding domain");

    /*
     * Cleanup.
     */
    mime_state_free(state);
    vstring_free(buf);
    exit(0);
}

#endif
