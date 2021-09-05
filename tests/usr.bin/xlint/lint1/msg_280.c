/*	$NetBSD: msg_280.c,v 1.4 2021/09/05 18:39:58 rillig Exp $	*/
# 3 "msg_280.c"

// Test for message: must be outside function: /* %s */ [280]

/* VARARGS */
void
varargs_ok(const char *str, ...)
{
	(void)str;
}

void
/* XXX: Why is this comment considered 'outside' enough? */
varargs_bad_param(/* VARARGS */ const char *str, ...)
{
	(void)str;
}

void
/* expect+1: warning: must be outside function: */
varargs_bad_ellipsis(const char *str, /* VARARGS */ ...)
{
	(void)str;
}

void
/* XXX: Why is this comment considered 'outside' enough? */
varargs_bad_body(const char *str, ...)
{
	/* expect+1: warning: must be outside function */
	/* VARARGS */
	(void)str;
}

void
/* expect+1: warning: argument 'str' unused in function 'argsused_bad_body' [231] */
argsused_bad_body(const char *str)
{
	/* expect+1: warning: must be outside function */
	/* ARGSUSED */
}

void
printflike_bad_body(const char *fmt, ...)
{
	/* expect+1: warning: must be outside function */
	/* PRINTFLIKE */
	(void)fmt;
}

void
scanflike_bad_body(const char *fmt, ...)
{
	/* expect+1: warning: must be outside function */
	/* SCANFLIKE */
	(void)fmt;
}
