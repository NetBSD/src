/*	$NetBSD: lex_comment.c,v 1.3 2024/10/04 11:38:03 rillig Exp $	*/
# 3 "lex_comment.c"

/*
 * Tests for comments, including lint-style comments that
 * suppress a single diagnostic.
 */

/* lint1-extra-flags: -X 351 -aa */

signed char s8;
signed long long s64;

// A "LINTED" comment suppresses a single warning until the end of the next
// statement.
void
lint_comment(void)
{
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64;

	/* LINTED 132 */
	s8 = s64;

	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64;

	/* LINTED 132 "comment" */
	s8 = s64;

	/* LINTED 132 */
	{
	}
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64;

	/* LINTED 132 */
	{
		s8 = s64;
	}
	/* expect+1: warning: conversion from 'long long' to 'signed char' may lose accuracy [132] */
	s8 = s64;

	if (s8 == 0)
		;
	/* LINTED 132 */
	s8 = s64;

	if (s8 == 0) {
	}
	/* LINTED 132 */
	s8 = s64;

	if (s8 == 0)
		;
	else
		;
	/* LINTED 132 */
	s8 = s64;

	if (s8 == 0) {
	} else {
	}
	/* LINTED 132 */
	s8 = s64;

	if (s8 == 0) {
	} else if (s8 == 1)
		;
	/* LINTED 132 */
	s8 = s64;

	if (s8 == 0) {
	} else if (s8 == 1) {
	}
	/* LINTED 132 */
	s8 = s64;
}


/*
 * Before lex.c 1.41 from 2021-06-19, lint ran into an endless loop when it
 * saw an unclosed comment at the end of the translation unit.  In practice
 * this was not relevant since the translation unit always comes from the C
 * preprocessor, which always emits a well-formed token sequence.
 */

/* expect+2: error: unterminated comment [256] */
/* unclosed comment
