/*	$NetBSD: mac_expand.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	mac_expand 3
/* SUMMARY
/*	attribute expansion
/* SYNOPSIS
/*	#include <mac_expand.h>
/*
/*	int	mac_expand(result, pattern, flags, filter, lookup, context)
/*	VSTRING *result;
/*	const char *pattern;
/*	int	flags;
/*	const char *filter;
/*	const char *lookup(const char *key, int mode, void *context)
/*	void *context;
/* AUXILIARY FUNCTIONS
/*	typedef	MAC_EXP_OP_RES (*MAC_EXPAND_RELOP_FN) (
/*	const char *left,
/*	int	tok_val,
/*	const char *rite)
/*
/*	void	mac_expand_add_relop(
/*	int	*tok_list,
/*	const char *suffix,
/*	MAC_EXPAND_RELOP_FN relop_eval)
/*
/*	MAC_EXP_OP_RES mac_exp_op_res_bool[2];
/* DESCRIPTION
/*	This module implements parameter-less named attribute
/*	expansions, both conditional and unconditional. As of Postfix
/*	3.0 this code supports relational expression evaluation.
/*
/*	In this text, an attribute is considered "undefined" when its value
/*	is a null pointer.  Otherwise, the attribute is considered "defined"
/*	and is expected to have as value a null-terminated string.
/*
/*	In the text below, the legacy form $(...) is equivalent to
/*	${...}. The legacy form $(...) may eventually disappear
/*	from documentation. In the text below, the name in $name
/*	and ${name...} must contain only characters from the set
/*	[a-zA-Z0-9_].
/*
/*	The following substitutions are supported:
/* .IP "$name, ${name}"
/*	Unconditional attribute-based substitution. The result is the
/*	named attribute value (empty if the attribute is not defined)
/*	after optional further named attribute substitution.
/* .IP "${name?text}, ${name?{text}}"
/*	Conditional attribute-based substitution. If the named attribute
/*	value is non-empty, the result is the given text, after
/*	named attribute expansion and relational expression evaluation.
/*	Otherwise, the result is empty.  Whitespace before or after
/*	{text} is ignored.
/* .IP "${name:text}, ${name:{text}}"
/*	Conditional attribute-based substitution. If the attribute
/*	value is empty or undefined, the expansion is the given
/*	text, after named attribute expansion and relational expression
/*	evaluation.  Otherwise, the result is empty.  Whitespace
/*	before or after {text} is ignored.
/* .IP "${name?{text1}:{text2}}, ${name?{text1}:text2}"
/*	Conditional attribute-based substitution. If the named attribute
/*	value is non-empty, the result is text1.  Otherwise, the
/*	result is text2. In both cases the result is subject to
/*	named attribute expansion and relational expression evaluation.
/*	Whitespace before or after {text1} or {text2} is ignored.
/* .IP "${{text1} == ${text2} ? {text3} : {text4}}"
/*	Relational expression-based substitution.  First, the content
/*	of {text1} and ${text2} is subjected to named attribute and
/*	relational expression-based substitution.  Next, the relational
/*	expression is evaluated. If it evaluates to "true", the
/*	result is the content of {text3}, otherwise it is the content
/*	of {text4}, after named attribute and relational expression-based
/*	substitution. In addition to ==, this supports !=, <, <=,
/*	>=, and >. Comparisons are numerical when both operands are
/*	all digits, otherwise the comparisons are lexicographical.
/*
/*	Arguments:
/* .IP result
/*	Storage for the result of expansion. By default, the result
/*	is truncated upon entry.
/* .IP pattern
/*	The string to be expanded.
/* .IP flags
/*	Bit-wise OR of zero or more of the following:
/* .RS
/* .IP MAC_EXP_FLAG_RECURSE
/*	Expand attributes in lookup results. This should never be
/*	done with data whose origin is untrusted.
/* .IP MAC_EXP_FLAG_APPEND
/*	Append text to the result buffer without truncating it.
/* .IP MAC_EXP_FLAG_SCAN
/*	Scan the input for named attributes, including named
/*	attributes in all conditional result values.  Do not expand
/*	named attributes, and do not truncate or write to the result
/*	argument.
/* .IP MAC_EXP_FLAG_PRINTABLE
/*	Use the printable() function instead of \fIfilter\fR.
/* .PP
/*	The constant MAC_EXP_FLAG_NONE specifies a manifest null value.
/* .RE
/* .IP filter
/*	A null pointer, or a null-terminated array of characters that
/*	are allowed to appear in an expansion. Illegal characters are
/*	replaced by underscores.
/* .IP lookup
/*	The attribute lookup routine. Arguments are: the attribute name,
/*	MAC_EXP_MODE_TEST to test the existence of the named attribute
/*	or MAC_EXP_MODE_USE to use the value of the named attribute,
/*	and the caller context that was given to mac_expand(). A null
/*	result value means that the requested attribute was not defined.
/* .IP context
/*	Caller context that is passed on to the attribute lookup routine.
/* .PP
/*	mac_expand_add_relop() registers a function that implements
/*	support for custom relational operators. Custom operator names
/*	such as "==xxx" have two parts: a prefix that is identical to
/*	a built-in operator such as "==", and an application-specified
/*	suffix such as "xxx".
/*
/*	Arguments:
/* .IP tok_list
/*	A null-terminated list of MAC_EXP_OP_TOK_* values that support
/*	the custom operator suffix.
/* .IP suffix
/*	A null-terminated alphanumeric string that specifies the custom
/*	operator suffix.
/* .IP relop_eval
/*	A function that compares two strings according to the
/*	MAC_EXP_OP_TOK_* value specified with the tok_val argument,
/*	and that returns non-zero if the custom operator evaluates to
/*	true, zero otherwise.
/*
/*	mac_exp_op_res_bool provides an array that converts a boolean
/*	value (0 or 1) to the corresponding MAX_EXP_OP_RES_TRUE or
/*	MAX_EXP_OP_RES_FALSE value.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.  Warnings: syntax errors, unreasonable
/*	recursion depth.
/*
/*	The result value is the binary OR of zero or more of the following:
/* .IP MAC_PARSE_ERROR
/*	A syntax error was found in \fBpattern\fR, or some attribute had
/*	an unreasonable nesting depth.
/* .IP MAC_PARSE_UNDEF
/*	An attribute was expanded but its value was not defined.
/* SEE ALSO
/*	mac_parse(3) locate macro references in string.
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

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <vstring.h>
#include <mymalloc.h>
#include <stringops.h>
#include <name_code.h>
#include <sane_strtol.h>
#include <mac_parse.h>
#include <mac_expand.h>

 /*
  * Simplifies the return of common relational operator results.
  */
MAC_EXP_OP_RES mac_exp_op_res_bool[2] = {
    MAC_EXP_OP_RES_FALSE,
    MAC_EXP_OP_RES_TRUE
};

 /*
  * Little helper structure.
  */
typedef struct {
    VSTRING *result;			/* result buffer */
    int     flags;			/* features */
    const char *filter;			/* character filter */
    MAC_EXP_LOOKUP_FN lookup;		/* lookup routine */
    void   *context;			/* caller context */
    int     status;			/* findings */
    int     level;			/* nesting level */
} MAC_EXP_CONTEXT;

 /*
  * Support for relational expressions.
  * 
  * As of Postfix 2.2, ${attr-name?result} or ${attr-name:result} return the
  * result respectively when the parameter value is non-empty, or when the
  * parameter value is undefined or empty; support for the ternary ?:
  * operator was anticipated, but not implemented for 10 years.
  * 
  * To make ${relational-expr?result} and ${relational-expr:result} work as
  * expected without breaking the way that ? and : work, relational
  * expressions evaluate to a non-empty or empty value. It does not matter
  * what non-empty value we use for TRUE. However we must not use the
  * undefined (null pointer) value for FALSE - that would raise the
  * MAC_PARSE_UNDEF flag.
  * 
  * The value of a relational expression can be exposed with ${relational-expr},
  * i.e. a relational expression that is not followed by ? or : conditional
  * expansion.
  */
#define MAC_EXP_BVAL_TRUE	"true"
#define MAC_EXP_BVAL_FALSE	""

 /*
  * Relational operators. The MAC_EXP_OP_TOK_* are defined in the header
  * file.
  */
#define MAC_EXP_OP_STR_EQ	"=="
#define MAC_EXP_OP_STR_NE	"!="
#define MAC_EXP_OP_STR_LT	"<"
#define MAC_EXP_OP_STR_LE	"<="
#define MAC_EXP_OP_STR_GE	">="
#define MAC_EXP_OP_STR_GT	">"
#define MAC_EXP_OP_STR_ANY	"\"" MAC_EXP_OP_STR_EQ \
				"\" or \"" MAC_EXP_OP_STR_NE "\"" \
				"\" or \"" MAC_EXP_OP_STR_LT "\"" \
				"\" or \"" MAC_EXP_OP_STR_LE "\"" \
				"\" or \"" MAC_EXP_OP_STR_GE "\"" \
				"\" or \"" MAC_EXP_OP_STR_GT "\""

static const NAME_CODE mac_exp_op_table[] =
{
    MAC_EXP_OP_STR_EQ, MAC_EXP_OP_TOK_EQ,
    MAC_EXP_OP_STR_NE, MAC_EXP_OP_TOK_NE,
    MAC_EXP_OP_STR_LT, MAC_EXP_OP_TOK_LT,
    MAC_EXP_OP_STR_LE, MAC_EXP_OP_TOK_LE,
    MAC_EXP_OP_STR_GE, MAC_EXP_OP_TOK_GE,
    MAC_EXP_OP_STR_GT, MAC_EXP_OP_TOK_GT,
    0, MAC_EXP_OP_TOK_NONE,
};

 /*
  * The whitespace separator set.
  */
#define MAC_EXP_WHITESPACE	CHARS_SPACE

 /*
  * Support for operator extensions.
  */
static HTABLE *mac_exp_ext_table;
static VSTRING *mac_exp_ext_key;

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* atol_or_die - convert or die */

static long atol_or_die(const char *strval)
{
    long    result;
    char   *remainder;

    result = sane_strtol(strval, &remainder, 10);
    if (*strval == 0 /* can't happen */ || *remainder != 0 || errno == ERANGE)
	msg_fatal("mac_exp_eval: bad conversion: %s", strval);
    return (result);
}

/* mac_exp_eval - evaluate binary expression */

static MAC_EXP_OP_RES mac_exp_eval(const char *left, int tok_val,
				           const char *rite)
{
    static const char myname[] = "mac_exp_eval";
    long    delta;

    /*
     * Numerical or string comparison.
     */
    if (alldig(left) && alldig(rite)) {
	delta = atol_or_die(left) - atol_or_die(rite);
    } else {
	delta = strcmp(left, rite);
    }
    switch (tok_val) {
    case MAC_EXP_OP_TOK_EQ:
	return (mac_exp_op_res_bool[delta == 0]);
    case MAC_EXP_OP_TOK_NE:
	return (mac_exp_op_res_bool[delta != 0]);
    case MAC_EXP_OP_TOK_LT:
	return (mac_exp_op_res_bool[delta < 0]);
    case MAC_EXP_OP_TOK_LE:
	return (mac_exp_op_res_bool[delta <= 0]);
    case MAC_EXP_OP_TOK_GE:
	return (mac_exp_op_res_bool[delta >= 0]);
    case MAC_EXP_OP_TOK_GT:
	return (mac_exp_op_res_bool[delta > 0]);
    default:
	msg_panic("%s: unknown operator: %d",
		  myname, tok_val);
    }
}

/* mac_exp_parse_error - report parse error, set error flag, return status */

static int PRINTFLIKE(2, 3) mac_exp_parse_error(MAC_EXP_CONTEXT *mc,
						        const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_warn(fmt, ap);
    va_end(ap);
    return (mc->status |= MAC_PARSE_ERROR);
};

/* MAC_EXP_ERR_RETURN - report parse error, set error flag, return status */

#define MAC_EXP_ERR_RETURN(mc, fmt, ...) do { \
	return (mac_exp_parse_error(mc, fmt, __VA_ARGS__)); \
    } while (0)

 /*
  * Postfix 3.0 introduces support for {text} operands. Only with these do we
  * support the ternary ?: operator and relational operators.
  * 
  * We cannot support operators in random text, because that would break Postfix
  * 2.11 compatibility. For example, with the expression "${name?value}", the
  * value is random text that may contain ':', '?', '{' and '}' characters.
  * In particular, with Postfix 2.2 .. 2.11, "${name??foo:{b}ar}" evaluates
  * to "?foo:{b}ar" or empty. There are explicit tests in this directory and
  * the postconf directory to ensure that Postfix 2.11 compatibility is
  * maintained.
  * 
  * Ideally, future Postfix configurations enclose random text operands inside
  * {} braces. These allow whitespace around operands, which improves
  * readability.
  */

/* MAC_EXP_FIND_LEFT_CURLY - skip over whitespace to '{', advance read ptr */

#define MAC_EXP_FIND_LEFT_CURLY(len, cp) \
	((cp[len = strspn(cp, MAC_EXP_WHITESPACE)] == '{') ? \
	 (cp += len) : 0)

/* mac_exp_extract_curly_payload - balance {}, skip whitespace, return payload */

static char *mac_exp_extract_curly_payload(MAC_EXP_CONTEXT *mc, char **bp)
{
    char   *payload;
    char   *cp;
    int     level;
    int     ch;

    /*
     * Extract the payload and balance the {}. The caller is expected to skip
     * leading whitespace before the {. See MAC_EXP_FIND_LEFT_CURLY().
     */
    for (level = 1, cp = *bp, payload = ++cp; /* see below */ ; cp++) {
	if ((ch = *cp) == 0) {
	    mac_exp_parse_error(mc, "unbalanced {} in attribute expression: "
				"\"%s\"",
				*bp);
	    return (0);
	} else if (ch == '{') {
	    level++;
	} else if (ch == '}') {
	    if (--level <= 0)
		break;
	}
    }
    *cp++ = 0;

    /*
     * Skip trailing whitespace after }.
     */
    *bp = cp + strspn(cp, MAC_EXP_WHITESPACE);
    return (payload);
}

/* mac_exp_parse_relational - parse relational expression, advance read ptr */

static int mac_exp_parse_relational(MAC_EXP_CONTEXT *mc, const char **lookup,
				            char **bp)
{
    char   *cp = *bp;
    VSTRING *left_op_buf;
    VSTRING *rite_op_buf;
    const char *left_op_strval;
    const char *rite_op_strval;
    char   *op_pos;
    char   *op_strval;
    size_t  op_len;
    int     op_tokval;
    int     op_result;
    size_t  tmp_len;
    char   *type_pos;
    size_t  type_len;
    MAC_EXPAND_RELOP_FN relop_eval;

    /*
     * Left operand. The caller is expected to skip leading whitespace before
     * the {. See MAC_EXP_FIND_LEFT_CURLY().
     */
    if ((left_op_strval = mac_exp_extract_curly_payload(mc, &cp)) == 0)
	return (mc->status);

    /*
     * Operator. Todo: regexp operator.
     */
    op_pos = cp;
    op_len = strspn(cp, "<>!=?+-*/~&|%");	/* for better diagnostics. */
    op_strval = mystrndup(cp, op_len);
    op_tokval = name_code(mac_exp_op_table, NAME_CODE_FLAG_NONE, op_strval);
    myfree(op_strval);
    if (op_tokval == MAC_EXP_OP_TOK_NONE)
	MAC_EXP_ERR_RETURN(mc, "%s expected at: \"...%s}>>>%.20s\"",
			   MAC_EXP_OP_STR_ANY, left_op_strval, cp);
    cp += op_len;

    /*
     * Custom operator suffix.
     */
    if (mac_exp_ext_table && ISALNUM(*cp)) {
	type_pos = cp;
	for (type_len = 1; ISALNUM(cp[type_len]); type_len++)
	     /* void */ ;
	cp += type_len;
	vstring_sprintf(mac_exp_ext_key, "%.*s",
			(int) (op_len + type_len), op_pos);
	if ((relop_eval = (MAC_EXPAND_RELOP_FN) htable_find(mac_exp_ext_table,
						STR(mac_exp_ext_key))) == 0)
	    MAC_EXP_ERR_RETURN(mc, "bad operator suffix at: \"...%.*s>>>%.*s\"",
			    (int) op_len, op_pos, (int) type_len, type_pos);
    } else {
	relop_eval = mac_exp_eval;
    }

    /*
     * Right operand. Todo: syntax may depend on operator.
     */
    if (MAC_EXP_FIND_LEFT_CURLY(tmp_len, cp) == 0)
	MAC_EXP_ERR_RETURN(mc, "\"{expression}\" expected at: "
			   "\"...{%s} %.*s>>>%.20s\"",
			   left_op_strval, (int) op_len, op_pos, cp);
    if ((rite_op_strval = mac_exp_extract_curly_payload(mc, &cp)) == 0)
	return (mc->status);

    /*
     * Evaluate the relational expression. Todo: regexp support.
     */
    mc->status |=
	mac_expand(left_op_buf = vstring_alloc(100), left_op_strval,
		   mc->flags, mc->filter, mc->lookup, mc->context);
    mc->status |=
	mac_expand(rite_op_buf = vstring_alloc(100), rite_op_strval,
		   mc->flags, mc->filter, mc->lookup, mc->context);
    if ((mc->flags & MAC_EXP_FLAG_SCAN) == 0
	&& (op_result = relop_eval(vstring_str(left_op_buf), op_tokval,
			 vstring_str(rite_op_buf))) == MAC_EXP_OP_RES_ERROR)
	mc->status |= MAC_PARSE_ERROR;
    vstring_free(left_op_buf);
    vstring_free(rite_op_buf);
    if (mc->status & MAC_PARSE_ERROR)
	return (mc->status);

    /*
     * Here, we fake up a non-empty or empty parameter value lookup result,
     * for compatibility with the historical code that looks named parameter
     * values.
     */
    if (mc->flags & MAC_EXP_FLAG_SCAN) {
	*lookup = 0;
    } else {
	switch (op_result) {
	case MAC_EXP_OP_RES_TRUE:
	    *lookup = MAC_EXP_BVAL_TRUE;
	    break;
	case MAC_EXP_OP_RES_FALSE:
	    *lookup = MAC_EXP_BVAL_FALSE;
	    break;
	default:
	    msg_panic("mac_expand: unexpected operator result: %d", op_result);
	}
    }
    *bp = cp;
    return (0);
}

/* mac_expand_add_relop - register operator extensions */

void    mac_expand_add_relop(int *tok_list, const char *suffix,
			             MAC_EXPAND_RELOP_FN relop_eval)
{
    const char myname[] = "mac_expand_add_relop";
    const char *tok_name;
    int    *tp;

    /*
     * Sanity checks.
     */
    if (!allalnum(suffix))
	msg_panic("%s: bad operator suffix: %s", myname, suffix);

    /*
     * One-time initialization.
     */
    if (mac_exp_ext_table == 0) {
	mac_exp_ext_table = htable_create(10);
	mac_exp_ext_key = vstring_alloc(10);
    }
    for (tp = tok_list; *tp; tp++) {
	if ((tok_name = str_name_code(mac_exp_op_table, *tp)) == 0)
	    msg_panic("%s: unknown token code: %d", myname, *tp);
	vstring_sprintf(mac_exp_ext_key, "%s%s", tok_name, suffix);
	if (htable_locate(mac_exp_ext_table, STR(mac_exp_ext_key)) != 0)
	    msg_panic("%s: duplicate key: %s", myname, STR(mac_exp_ext_key));
	(void) htable_enter(mac_exp_ext_table,
			    STR(mac_exp_ext_key), (void *) relop_eval);
    }
}

/* mac_expand_callback - callback for mac_parse */

static int mac_expand_callback(int type, VSTRING *buf, void *ptr)
{
    static const char myname[] = "mac_expand_callback";
    MAC_EXP_CONTEXT *mc = (MAC_EXP_CONTEXT *) ptr;
    int     lookup_mode;
    const char *lookup;
    char   *cp;
    int     ch;
    ssize_t res_len;
    ssize_t tmp_len;
    const char *res_iftrue;
    const char *res_iffalse;

    /*
     * Sanity check.
     */
    if (mc->level++ > 100)
	mac_exp_parse_error(mc, "unreasonable macro call nesting: \"%s\"",
			    vstring_str(buf));
    if (mc->status & MAC_PARSE_ERROR)
	return (mc->status);

    /*
     * Named parameter or relational expression. In case of a syntax error,
     * return without doing damage, and issue a warning instead.
     */
    if (type == MAC_PARSE_EXPR) {

	cp = vstring_str(buf);

	/*
	 * Relational expression. If recursion is disabled, perform only one
	 * level of $name expansion.
	 */
	if (MAC_EXP_FIND_LEFT_CURLY(tmp_len, cp)) {
	    if (mac_exp_parse_relational(mc, &lookup, &cp) != 0)
		return (mc->status);

	    /*
	     * Look for the ? or : operator.
	     */
	    if ((ch = *cp) != 0) {
		if (ch != '?' && ch != ':')
		    MAC_EXP_ERR_RETURN(mc, "\"?\" or \":\" expected at: "
				       "\"...}>>>%.20s\"", cp);
		cp++;
	    }
	}

	/*
	 * Named parameter.
	 */
	else {
	    char   *start;

	    /*
	     * Look for the ? or : operator. In case of a syntax error,
	     * return without doing damage, and issue a warning instead.
	     */
	    start = (cp += strspn(cp, MAC_EXP_WHITESPACE));
	    for ( /* void */ ; /* void */ ; cp++) {
		if ((ch = cp[tmp_len = strspn(cp, MAC_EXP_WHITESPACE)]) == 0) {
		    *cp = 0;
		    lookup_mode = MAC_EXP_MODE_USE;
		    break;
		}
		if (ch == '?' || ch == ':') {
		    *cp++ = 0;
		    cp += tmp_len;
		    lookup_mode = MAC_EXP_MODE_TEST;
		    break;
		}
		ch = *cp;
		if (!ISALNUM(ch) && ch != '_') {
		    MAC_EXP_ERR_RETURN(mc, "attribute name syntax error at: "
				       "\"...%.*s>>>%.20s\"",
				       (int) (cp - vstring_str(buf)),
				       vstring_str(buf), cp);
		}
	    }

	    /*
	     * Look up the named parameter. Todo: allow the lookup function
	     * to specify if the result is safe for $name expansion.
	     */
	    lookup = mc->lookup(start, lookup_mode, mc->context);
	}

	/*
	 * Return the requested result. After parsing the result operand
	 * following ?, we fall through to parse the result operand following
	 * :. This is necessary with the ternary ?: operator: first, with
	 * MAC_EXP_FLAG_SCAN to parse both result operands with mac_parse(),
	 * and second, to find garbage after any result operand. Without
	 * MAC_EXP_FLAG_SCAN the content of only one of the ?: result
	 * operands will be parsed with mac_parse(); syntax errors in the
	 * other operand will be missed.
	 */
	switch (ch) {
	case '?':
	    if (MAC_EXP_FIND_LEFT_CURLY(tmp_len, cp)) {
		if ((res_iftrue = mac_exp_extract_curly_payload(mc, &cp)) == 0)
		    return (mc->status);
	    } else {
		res_iftrue = cp;
		cp = "";			/* no left-over text */
	    }
	    if ((lookup != 0 && *lookup != 0) || (mc->flags & MAC_EXP_FLAG_SCAN))
		mc->status |= mac_parse(res_iftrue, mac_expand_callback,
					(void *) mc);
	    if (*cp == 0)			/* end of input, OK */
		break;
	    if (*cp != ':')			/* garbage */
		MAC_EXP_ERR_RETURN(mc, "\":\" expected at: "
				   "\"...%s}>>>%.20s\"", res_iftrue, cp);
	    cp += 1;
	    /* FALLTHROUGH: do not remove, see comment above. */
	case ':':
	    if (MAC_EXP_FIND_LEFT_CURLY(tmp_len, cp)) {
		if ((res_iffalse = mac_exp_extract_curly_payload(mc, &cp)) == 0)
		    return (mc->status);
	    } else {
		res_iffalse = cp;
		cp = "";			/* no left-over text */
	    }
	    if (lookup == 0 || *lookup == 0 || (mc->flags & MAC_EXP_FLAG_SCAN))
		mc->status |= mac_parse(res_iffalse, mac_expand_callback,
					(void *) mc);
	    if (*cp != 0)			/* garbage */
		MAC_EXP_ERR_RETURN(mc, "unexpected input at: "
				   "\"...%s}>>>%.20s\"", res_iffalse, cp);
	    break;
	case 0:
	    if (lookup == 0) {
		mc->status |= MAC_PARSE_UNDEF;
	    } else if (*lookup == 0 || (mc->flags & MAC_EXP_FLAG_SCAN)) {
		 /* void */ ;
	    } else if (mc->flags & MAC_EXP_FLAG_RECURSE) {
		vstring_strcpy(buf, lookup);
		mc->status |= mac_parse(vstring_str(buf), mac_expand_callback,
					(void *) mc);
	    } else {
		res_len = VSTRING_LEN(mc->result);
		vstring_strcat(mc->result, lookup);
		if (mc->flags & MAC_EXP_FLAG_PRINTABLE) {
		    printable(vstring_str(mc->result) + res_len, '_');
		} else if (mc->filter) {
		    cp = vstring_str(mc->result) + res_len;
		    while (*(cp += strspn(cp, mc->filter)))
			*cp++ = '_';
		}
	    }
	    break;
	default:
	    msg_panic("%s: unknown operator code %d", myname, ch);
	}
    }

    /*
     * Literal text.
     */
    else if ((mc->flags & MAC_EXP_FLAG_SCAN) == 0) {
	vstring_strcat(mc->result, vstring_str(buf));
    }
    mc->level--;

    return (mc->status);
}

/* mac_expand - expand $name instances */

int     mac_expand(VSTRING *result, const char *pattern, int flags,
		           const char *filter,
		           MAC_EXP_LOOKUP_FN lookup, void *context)
{
    MAC_EXP_CONTEXT mc;
    int     status;

    /*
     * Bundle up the request and do the substitutions.
     */
    mc.result = result;
    mc.flags = flags;
    mc.filter = filter;
    mc.lookup = lookup;
    mc.context = context;
    mc.status = 0;
    mc.level = 0;
    if ((flags & (MAC_EXP_FLAG_APPEND | MAC_EXP_FLAG_SCAN)) == 0)
	VSTRING_RESET(result);
    status = mac_parse(pattern, mac_expand_callback, (void *) &mc);
    if ((flags & MAC_EXP_FLAG_SCAN) == 0)
	VSTRING_TERMINATE(result);

    return (status);
}

#ifdef TEST

 /*
  * This code certainly deserves a stand-alone test program.
  */
#include <stringops.h>
#include <htable.h>
#include <vstream.h>
#include <vstring_vstream.h>

static const char *lookup(const char *name, int unused_mode, void *context)
{
    HTABLE *table = (HTABLE *) context;

    return (htable_find(table, name));
}

static MAC_EXP_OP_RES length_relop_eval(const char *left, int relop,
					        const char *rite)
{
    const char myname[] = "length_relop_eval";
    ssize_t delta = strlen(left) - strlen(rite);

    switch (relop) {
    case MAC_EXP_OP_TOK_EQ:
	return (mac_exp_op_res_bool[delta == 0]);
    case MAC_EXP_OP_TOK_NE:
	return (mac_exp_op_res_bool[delta != 0]);
    case MAC_EXP_OP_TOK_LT:
	return (mac_exp_op_res_bool[delta < 0]);
    case MAC_EXP_OP_TOK_LE:
	return (mac_exp_op_res_bool[delta <= 0]);
    case MAC_EXP_OP_TOK_GE:
	return (mac_exp_op_res_bool[delta >= 0]);
    case MAC_EXP_OP_TOK_GT:
	return (mac_exp_op_res_bool[delta > 0]);
    default:
	msg_panic("%s: unknown operator: %d",
		  myname, relop);
    }
}

int     main(int unused_argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    VSTRING *result = vstring_alloc(100);
    char   *cp;
    char   *name;
    char   *value;
    HTABLE *table;
    int     stat;
    int     length_relops[] = {
	MAC_EXP_OP_TOK_EQ, MAC_EXP_OP_TOK_NE,
	MAC_EXP_OP_TOK_GT, MAC_EXP_OP_TOK_GE,
	MAC_EXP_OP_TOK_LT, MAC_EXP_OP_TOK_LE,
	0,
    };

    /*
     * Add relops that compare string lengths instead of content.
     */
    mac_expand_add_relop(length_relops, "length", length_relop_eval);

    /*
     * Loop over the inputs.
     */
    while (!vstream_feof(VSTREAM_IN)) {

	table = htable_create(0);

	/*
	 * Read a block of definitions, terminated with an empty line.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    name = mystrtok(&cp, CHARS_SPACE "=");
	    value = mystrtok(&cp, CHARS_SPACE "=");
	    htable_enter(table, name, value ? mystrdup(value) : 0);
	}

	/*
	 * Read a block of patterns, terminated with an empty line or EOF.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    VSTRING_RESET(result);
	    stat = mac_expand(result, vstring_str(buf), MAC_EXP_FLAG_NONE,
			      (char *) 0, lookup, (void *) table);
	    vstream_printf("stat=%d result=%s\n", stat, vstring_str(result));
	    vstream_fflush(VSTREAM_OUT);
	}
	htable_free(table, myfree);
	vstream_printf("\n");
    }

    /*
     * Clean up.
     */
    vstring_free(buf);
    vstring_free(result);
    exit(0);
}

#endif
