/*++
/* NAME
/*	tok822_parse 3
/* SUMMARY
/*	RFC 822 address parser
/* SYNOPSIS
/*	#include <tok822.h>
/*
/*	TOK822 *tok822_scan(str, tailp)
/*	const char *str;
/*	TOK822	**tailp;
/*
/*	TOK822	*tok822_parse(str)
/*	const char *str;
/*
/*	TOK822	*tok822_scan_addr(str)
/*	const char *str;
/*
/*	VSTRING	*tok822_externalize(buffer, tree, flags)
/*	VSTRING	*buffer;
/*	TOK822	*tree;
/*	int	flags;
/*
/*	VSTRING	*tok822_internalize(buffer, tree, flags)
/*	VSTRING	*buffer;
/*	TOK822	*tree;
/*	int	flags;
/* DESCRIPTION
/*	This module converts address lists between string form and parse
/*	tree formats. The string form can appear in two different ways:
/*	external (or quoted) form, as used in message headers, and internal
/*	(unquoted) form, as used internally by the mail software.
/*	Although RFC 822 expects 7-bit data, these routines pay no
/*	special attention to 8-bit characters.
/*
/*	tok822_scan() converts the external-form string in \fIstr\fR
/*	to a linear token list. The \fItailp\fR argument is a null pointer
/*	or receives the pointer value of the last result list element.
/*
/*	tok822_parse() converts the external-form address list in
/*	\fIstr\fR to the corresponding token tree. The parser is permissive
/*	and will not throw away information that it does not understand.
/*	The parser adds missing commas between addresses.
/*
/*	tok822_scan_addr() converts the external-form string in
/*	\fIstr\fR to an address token tree. This is just string to
/*	token list conversion; no parsing is done. This routine is
/*	suitable for data should contain just one address and no
/*	other information.
/*
/*	tok822_externalize() converts a token list to external form.
/*	Where appropriate, characters and strings are quoted and white
/*	space is inserted. The \fIflags\fR argument is the binary OR of
/*	zero or more of the following:
/* .IP TOK822_STR_WIPE
/*	Initially, truncate the result to zero length.
/* .IP TOK822_STR_TERM
/*	Append a null terminator to the result when done.
/* .IP TOK822_STR_LINE
/*	Append a line break after each comma token, instead of appending
/*	whitespace.  It is up to the caller to concatenate short lines to
/*	produce longer ones.
/* .PP
/*	The macro TOK_822_NONE expresses that none of the above features
/*	should be activated.
/*
/*	The macro TOK822_STR_DEFL combines the TOK822_STR_WIPE and
/*	TOK822_STR_TERM flags. This is useful for most token to string
/*	conversions.
/*
/*	The macro TOK822_STR_HEAD combines the TOK822_STR_TERM and
/*	TOK822_STR_LINE flags. This is useful for the special case of
/*	token to mail header conversion.
/*
/*	tok822_internalize() converts a token list to string form,
/*	without quoting. White space is inserted where appropriate.
/*	The \fIflags\fR argument is as with tok822_externalize().
/* STANDARDS
/* .ad
/* .fi
/*	RFC 822 (ARPA Internet Text Messages). In addition to this standard
/*	this module implements additional operators such as % and !. These
/*	are needed because the real world is not all RFC 822. Also, the ':'
/*	operator is allowed to appear inside addresses, to accommodate DECnet.
/*	In addition, 8-bit data is not given special treatment.
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

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <vstring.h>
#include <msg.h>

/* Global library. */

#include "tok822.h"

 /*
  * I suppose this is my favorite macro. Used heavily for tokenizing.
  */
#define COLLECT(t,s,c,cond) { \
	while ((c = *(unsigned char *) s) != 0) { \
	    if (c == '\\') { \
		if ((c = *(unsigned char *)++s) == 0) \
		    break; \
	    } else if (!(cond)) { \
		break; \
	    } \
	    VSTRING_ADDCH(t->vstr, ISSPACE(c) ? ' ' : c); \
	    s++; \
	} \
	VSTRING_TERMINATE(t->vstr); \
    }

#define COLLECT_SKIP_LAST(t,s,c,cond) { COLLECT(t,s,c,cond); if (*s) s++; }

 /*
  * Not quite as complex. The parser depends heavily on it.
  */
#define SKIP(tp, cond) { \
	while (tp->type && (cond)) \
	    tp = tp->prev; \
    }

#define MOVE_COMMENT_AND_CONTINUE(tp, right) { \
	TOK822 *prev = tok822_unlink(tp); \
	right = tok822_prepend(right, tp); \
	tp = prev; \
	continue; \
    }

#define SKIP_MOVE_COMMENT(tp, cond, right) { \
	while (tp->type && (cond)) { \
	    if (tp->type == TOK822_COMMENT) \
		MOVE_COMMENT_AND_CONTINUE(tp, right); \
	    tp = tp->prev; \
	} \
    }

 /*
  * Single-character operators. We include the % and ! operators because not
  * all the world is RFC822. XXX Make this operator list configurable when we
  * have a real rewriting language.
  */
static char tok822_opchar[] = "|\"(),.:;<>@[]%!";

static void tok822_quote_atom(TOK822 *);
static const char *tok822_comment(TOK822 *, const char *);
static TOK822 *tok822_group(int, TOK822 *, TOK822 *, int);
static void tok822_copy_quoted(VSTRING *, char *, char *);
static int tok822_append_space(TOK822 *);

#define DO_WORD		(1<<0)		/* finding a word is ok here */
#define DO_GROUP	(1<<1)		/* doing an address group */

#define ADD_COMMA	','		/* resynchronize */
#define NO_MISSING_COMMA 0

/* tok822_internalize - token tree to string, internal form */

VSTRING *tok822_internalize(VSTRING *vp, TOK822 *tree, int flags)
{
    TOK822 *tp;

    if (flags & TOK822_STR_WIPE)
	VSTRING_RESET(vp);

    for (tp = tree; tp; tp = tp->next) {
	switch (tp->type) {
	case ',':
	    VSTRING_ADDCH(vp, tp->type);
	    if (flags & TOK822_STR_LINE) {
		VSTRING_ADDCH(vp, '\n');
		continue;
	    }
	    break;
	case TOK822_ADDR:
	    tok822_internalize(vp, tp->head, TOK822_STR_NONE);
	    break;
	case TOK822_COMMENT:
	    VSTRING_ADDCH(vp, '(');
	    tok822_internalize(vp, tp->head, TOK822_STR_NONE);
	    VSTRING_ADDCH(vp, ')');
	    break;
	case TOK822_ATOM:
	case TOK822_COMMENT_TEXT:
	case TOK822_QSTRING:
	    vstring_strcat(vp, vstring_str(tp->vstr));
	    break;
	case TOK822_DOMLIT:
	    VSTRING_ADDCH(vp, '[');
	    vstring_strcat(vp, vstring_str(tp->vstr));
	    VSTRING_ADDCH(vp, ']');
	    break;
	case TOK822_STARTGRP:
	    VSTRING_ADDCH(vp, ':');
	    break;
	default:
	    if (tp->type >= TOK822_MINTOK)
		msg_panic("tok822_internalize: unknown operator %d", tp->type);
	    VSTRING_ADDCH(vp, tp->type);
	}
	if (tok822_append_space(tp))
	    VSTRING_ADDCH(vp, ' ');
    }
    if (flags & TOK822_STR_TERM)
	VSTRING_TERMINATE(vp);
    return (vp);
}

/* tok822_externalize - token tree to string, external form */

VSTRING *tok822_externalize(VSTRING *vp, TOK822 *tree, int flags)
{
    TOK822 *tp;

    if (flags & TOK822_STR_WIPE)
	VSTRING_RESET(vp);

    for (tp = tree; tp; tp = tp->next) {
	switch (tp->type) {
	case ',':
	    VSTRING_ADDCH(vp, tp->type);
	    if (flags & TOK822_STR_LINE) {
		VSTRING_ADDCH(vp, '\n');
		continue;
	    }
	    break;
	case TOK822_ADDR:
	    tok822_externalize(vp, tp->head, TOK822_STR_NONE);
	    break;
	case TOK822_ATOM:
	    vstring_strcat(vp, vstring_str(tp->vstr));
	    break;
	case TOK822_COMMENT:
	    VSTRING_ADDCH(vp, '(');
	    tok822_externalize(vp, tp->head, TOK822_STR_NONE);
	    VSTRING_ADDCH(vp, ')');
	    break;
	case TOK822_COMMENT_TEXT:
	    tok822_copy_quoted(vp, vstring_str(tp->vstr), "()\\\r\n");
	    break;
	case TOK822_QSTRING:
	    VSTRING_ADDCH(vp, '"');
	    tok822_copy_quoted(vp, vstring_str(tp->vstr), "\"\\\r\n");
	    VSTRING_ADDCH(vp, '"');
	    break;
	case TOK822_DOMLIT:
	    VSTRING_ADDCH(vp, '[');
	    tok822_copy_quoted(vp, vstring_str(tp->vstr), "\\\r\n");
	    VSTRING_ADDCH(vp, ']');
	    break;
	case TOK822_STARTGRP:
	    VSTRING_ADDCH(vp, ':');
	    break;
	default:
	    if (tp->type >= TOK822_MINTOK)
		msg_panic("tok822_externalize: unknown operator %d", tp->type);
	    VSTRING_ADDCH(vp, tp->type);
	}
	if (tok822_append_space(tp))
	    VSTRING_ADDCH(vp, ' ');
    }
    if (flags & TOK822_STR_TERM)
	VSTRING_TERMINATE(vp);
    return (vp);
}

/* tok822_copy_quoted - copy a string while quoting */

static void tok822_copy_quoted(VSTRING *vp, char *str, char *quote_set)
{
    int     ch;

    while ((ch = *(unsigned char *) str++) != 0) {
	if (strchr(quote_set, ch))
	    VSTRING_ADDCH(vp, '\\');
	VSTRING_ADDCH(vp, ch);
    }
}

/* tok822_append_space - see if space is needed after this token */

static int tok822_append_space(TOK822 *tp)
{
    TOK822 *next;

    if (tp == 0 || (next = tp->next) == 0 || tp->owner != 0)
	return (0);
    if (tp->type == ',' || tp->type == TOK822_STARTGRP || next->type == '<')
	return (1);

#define NON_OPERATOR(x) \
    (x->type == TOK822_ATOM || x->type == TOK822_QSTRING \
     || x->type == TOK822_COMMENT || x->type == TOK822_DOMLIT \
     || x->type == TOK822_ADDR)

    return (NON_OPERATOR(tp) && NON_OPERATOR(next));
}

/* tok822_scan - tokenize string */

TOK822 *tok822_scan(const char *str, TOK822 **tailp)
{
    TOK822 *head = 0;
    TOK822 *tail = 0;
    TOK822 *tp;
    int     ch;

    while ((ch = *(unsigned char *) str++) != 0) {
	if (ISSPACE(ch))
	    continue;
	if (ch == '(') {
	    tp = tok822_alloc(TOK822_COMMENT, (char *) 0);
	    str = tok822_comment(tp, str);
	} else if (ch == '[') {
	    tp = tok822_alloc(TOK822_DOMLIT, (char *) 0);
	    COLLECT_SKIP_LAST(tp, str, ch, ch != ']');
	} else if (ch == '"') {
	    tp = tok822_alloc(TOK822_QSTRING, (char *) 0);
	    COLLECT_SKIP_LAST(tp, str, ch, ch != '"');
	} else if (ch != '\\' && strchr(tok822_opchar, ch)) {
	    tp = tok822_alloc(ch, (char *) 0);
	} else {
	    tp = tok822_alloc(TOK822_ATOM, (char *) 0);
	    str -= 1;				/* \ may be first */
	    COLLECT(tp, str, ch, !ISSPACE(ch) && !strchr(tok822_opchar, ch));
	    tok822_quote_atom(tp);
	}
	if (head == 0) {
	    head = tail = tp;
	    while (tail->next)
		tail = tail->next;
	} else {
	    tail = tok822_append(tail, tp);
	}
    }
    if (tailp)
	*tailp = tail;
    return (head);
}

/* tok822_parse - translate external string to token tree */

TOK822 *tok822_parse(const char *str)
{
    TOK822 *head;
    TOK822 *tail;
    TOK822 *right;
    TOK822 *first_token;
    TOK822 *last_token;
    TOK822 *tp;
    int     state;

    /*
     * First, tokenize the string, from left to right. We are not allowed to
     * throw away any information that we do not understand. With a flat
     * token list that contains all tokens, we can always convert back to
     * string form.
     */
    if ((first_token = tok822_scan(str, &last_token)) == 0)
	return (0);

    /*
     * For convenience, sandwich the token list between two sentinel tokens.
     */
#define GLUE(left,rite) { left->next = rite; rite->prev = left; }

    head = tok822_alloc(0, (char *) 0);
    GLUE(head, first_token);
    tail = tok822_alloc(0, (char *) 0);
    GLUE(last_token, tail);

    /*
     * Next step is to transform the token list into a parse tree. This is
     * done most conveniently from right to left. If there is something that
     * we do not understand, just leave it alone, don't throw it away. The
     * address information that we're looking for sits in-between the current
     * node (tp) and the one called right. Add missing commas on the fly.
     */
    state = DO_WORD;
    right = tail;
    tp = tail->prev;
    while (tp->type) {
	if (tp->type == TOK822_COMMENT) {	/* move comment to the side */
	    MOVE_COMMENT_AND_CONTINUE(tp, right);
	} else if (tp->type == ';') {		/* rh side of named group */
	    right = tok822_group(TOK822_ADDR, tp, right, ADD_COMMA);
	    state = DO_GROUP | DO_WORD;
	} else if (tp->type == ':' && (state & DO_GROUP) != 0) {
	    tp->type = TOK822_STARTGRP;
	    (void) tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);
	    SKIP(tp, tp->type != ',');
	    right = tp;
	    continue;
	} else if (tp->type == '>') {		/* rh side of <route> */
	    right = tok822_group(TOK822_ADDR, tp, right, ADD_COMMA);
	    SKIP_MOVE_COMMENT(tp, tp->type != '<', right);
	    (void) tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);
	    SKIP(tp, tp->type > 0xff || strchr(">;,:", tp->type) == 0);
	    right = tp;
	    state |= DO_WORD;
	    continue;
	} else if (tp->type == TOK822_ATOM || tp->type == TOK822_QSTRING
		   || tp->type == TOK822_DOMLIT) {
	    if ((state & DO_WORD) == 0)
		right = tok822_group(TOK822_ADDR, tp, right, ADD_COMMA)->next;
	    state &= ~DO_WORD;
	} else if (tp->type == ',') {
	    right = tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);
	    state |= DO_WORD;
	} else {
	    state |= DO_WORD;
	}
	tp = tp->prev;
    }
    (void) tok822_group(TOK822_ADDR, tp, right, NO_MISSING_COMMA);

    /*
     * Discard the sentinel tokens on the left and right extremes. Properly
     * terminate the resulting list.
     */
    tp = (head->next != tail ? head->next : 0);
    tok822_cut_before(head->next);
    tok822_free(head);
    tok822_cut_before(tail);
    tok822_free(tail);
    return (tp);
}

/* tok822_quote_atom - see if an atom needs quoting when externalized */

static void tok822_quote_atom(TOK822 *tp)
{
    char   *cp;
    int     ch;

    /*
     * RFC 822 expects 7-bit data. Rather than quoting every 8-bit character
     * (and still passing it on as 8-bit data) we leave 8-bit data alone.
     */
    for (cp = vstring_str(tp->vstr); (ch = *(unsigned char *) cp) != 0; cp++) {
	if ( /* !ISASCII(ch) || */ ISSPACE(ch)
	    || ISCNTRL(ch) || strchr(tok822_opchar, ch)) {
	    tp->type = TOK822_QSTRING;
	    break;
	}
    }
}

/* tok822_comment - tokenize comment */

static const char *tok822_comment(TOK822 *tp, const char *str)
{
    TOK822 *tc = 0;
    int     ch;

#define COMMENT_TEXT_TOKEN(t) ((t) && (t)->type == TOK822_COMMENT_TEXT)

#define APPEND_NEW_TOKEN(tp, type, strval) \
	tok822_sub_append(tp, tok822_alloc(type, strval))

    while ((ch = *(unsigned char *) str) != 0) {
	str++;
	if (ch == '(') {			/* comments can nest! */
	    if (COMMENT_TEXT_TOKEN(tc))
		VSTRING_TERMINATE(tc->vstr);
	    tc = APPEND_NEW_TOKEN(tp, TOK822_COMMENT, (char *) 0);
	    str = tok822_comment(tc, str);
	} else if (ch == ')') {
	    break;
	} else {
	    if (ch == '\\') {
		if ((ch = *(unsigned char *) str) == 0)
		    break;
		str++;
	    }
	    if (!COMMENT_TEXT_TOKEN(tc))
		tc = APPEND_NEW_TOKEN(tp, TOK822_COMMENT_TEXT, (char *) 0);
	    VSTRING_ADDCH(tc->vstr, ch);
	}
    }
    if (COMMENT_TEXT_TOKEN(tc))
	VSTRING_TERMINATE(tc->vstr);
    return (str);
}

/* tok822_group - cluster a group of tokens */

static TOK822 *tok822_group(int group_type, TOK822 *left, TOK822 *right, int sync_type)
{
    TOK822 *group;
    TOK822 *sync;
    TOK822 *first;

    /*
     * Cluster the tokens between left and right under their own parse tree
     * node. Optionally insert a resync token.
     */
    if (left != right && (first = left->next) != right) {
	tok822_cut_before(right);
	tok822_cut_before(first);
	group = tok822_alloc(group_type, (char *) 0);
	tok822_sub_append(group, first);
	tok822_append(left, group);
	tok822_append(group, right);
	if (sync_type) {
	    sync = tok822_alloc(sync_type, (char *) 0);
	    tok822_append(left, sync);
	}
    }
    return (left);
}

/* tok822_scan_addr - convert external address string to address token */

TOK822 *tok822_scan_addr(const char *addr)
{
    TOK822 *tree = tok822_alloc(TOK822_ADDR, (char *) 0);

    tree->head = tok822_scan(addr, &tree->tail);
    return (tree);
}

#ifdef TEST

#include <unistd.h>
#include <vstream.h>
#include <vstring_vstream.h>

/* tok822_print - display token */

static void tok822_print(TOK822 *list, int indent)
{
    TOK822 *tp;

    for (tp = list; tp; tp = tp->next) {
	if (tp->type < TOK822_MINTOK) {
	    vstream_printf("%*s %s \"%c\"\n", indent, "", "OP", tp->type);
	} else if (tp->type == TOK822_ADDR) {
	    vstream_printf("%*s %s\n", indent, "", "address");
	    tok822_print(tp->head, indent + 2);
	} else if (tp->type == TOK822_COMMENT) {
	    vstream_printf("%*s %s\n", indent, "", "comment");
	    tok822_print(tp->head, indent + 2);
	} else if (tp->type == TOK822_STARTGRP) {
	    vstream_printf("%*s %s\n", indent, "", "group \":\"");
	} else {
	    vstream_printf("%*s %s \"%s\"\n", indent, "",
			   tp->type == TOK822_COMMENT_TEXT ? "text" :
			   tp->type == TOK822_ATOM ? "atom" :
			   tp->type == TOK822_QSTRING ? "quoted string" :
			   tp->type == TOK822_DOMLIT ? "domain literal" :
			   tp->type == TOK822_ADDR ? "address" :
			   "unknown\n", vstring_str(tp->vstr));
	}
    }
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *vp = vstring_alloc(100);
    TOK822 *list;
    VSTRING *buf = vstring_alloc(100);

    while (vstring_fgets_nonl(buf, VSTREAM_IN)) {
	if (!isatty(vstream_fileno(VSTREAM_IN)))
	    vstream_printf(">>>%s<<<\n\n", vstring_str(buf));
	list = tok822_parse(vstring_str(buf));
	vstream_printf("Parse tree:\n");
	tok822_print(list, 0);
	vstream_printf("\n");

	vstream_printf("Internalized:\n%s\n\n",
		vstring_str(tok822_internalize(vp, list, TOK822_STR_DEFL)));
	vstream_printf("Externalized, no newlines inserted:\n%s\n\n",
		vstring_str(tok822_externalize(vp, list, TOK822_STR_DEFL)));
	vstream_printf("Externalized, newlines inserted:\n%s\n\n",
		       vstring_str(tok822_externalize(vp, list,
				       TOK822_STR_DEFL | TOK822_STR_LINE)));
	vstream_fflush(VSTREAM_OUT);
	tok822_free_tree(list);
    }
    vstring_free(vp);
    vstring_free(buf);
    return (0);
}

#endif
