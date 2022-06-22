/*	$NetBSD: msg_280.c,v 1.6 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_280.c"

// Test for message: comment /* %s */ must be outside function [280]

/* VARARGS */
void
varargs_ok(const char *str, ...)
{
	(void)str;
}

/*
 * In the following example, the comment looks misplaced, but lint does not
 * warn about it.
 *
 * This is due to the implementation of the parser and the C grammar.  When
 * the parser sees the token T_LPAREN, it has to decide whether the following
 * tokens will form a parameter type list or an identifier list.  For that,
 * it needs to look at the next token to see whether it is a T_NAME (in which
 * case the T_LPAREN belongs to an id_list_lparen) or something else (in
 * which case the T_LPAREN belongs to an abstract_decl_lparen).  This token
 * lookahead happens just before either of these grammar rules is reduced.
 * During that reduction, the current declaration context switches from
 * 'extern' to 'prototype argument', which makes this exact position the very
 * last possible.  Everything later would already be in the wrong context.
 *
 * As of cgram.y 1.360 from 2021-09-04, the implementation of these grammar
 * rules is exactly the same, which makes it tempting to join them into a
 * single rule.
 */
void
varargs_bad_param(/* VARARGS */ const char *str, ...)
{
	(void)str;
}

void
/* expect+1: warning: comment ** VARARGS ** must be outside function [280] */
varargs_bad_ellipsis(const char *str, /* VARARGS */ ...)
{
	(void)str;
}

void
varargs_bad_body(const char *str, ...)
{
	/* expect+1: warning: comment ** VARARGS ** must be outside function [280] */
	/* VARARGS */
	(void)str;
}

void
/* expect+1: warning: argument 'str' unused in function 'argsused_bad_body' [231] */
argsused_bad_body(const char *str)
{
	/* expect+1: warning: comment ** ARGSUSED ** must be outside function [280] */
	/* ARGSUSED */
}

void
printflike_bad_body(const char *fmt, ...)
{
	/* expect+1: warning: comment ** PRINTFLIKE ** must be outside function [280] */
	/* PRINTFLIKE */
	(void)fmt;
}

void
scanflike_bad_body(const char *fmt, ...)
{
	/* expect+1: warning: comment ** SCANFLIKE ** must be outside function [280] */
	/* SCANFLIKE */
	(void)fmt;
}
