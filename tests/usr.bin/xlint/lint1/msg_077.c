/*	$NetBSD: msg_077.c,v 1.8 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_077.c"

/* Test for message: bad octal digit '%c' [77] */

/* lint1-flags: -tw -X 351 */

/* expect+1: warning: bad octal digit '8' [77] */
char single_digit = '\8';

/*
 * Before lex.c 1.47 from 2021-06-29, lint intended to detect a "bad octal
 * digit" following good octal digits, but the corresponding code had an
 * unsatisfiable guard clause.
 *
 * The C Reference Manual 1978, 2.4.3 "Character constants" does not mention
 * non-octal digits, therefore this code must have been due to a particular
 * C compiler's interpretation.  It's even wrong according to the Reference
 * Manual to interpret '\088' as anything else than a malformed character
 * literal.
 *
 * That code has been removed since nobody runs lint in traditional C mode
 * anyway.
 * https://mail-index.netbsd.org/tech-toolchain/2021/03/16/msg003933.html
 */
/* expect+1: warning: multi-character character constant [294] */
char several_digits = '\08';
