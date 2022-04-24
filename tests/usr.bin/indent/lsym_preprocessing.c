/* $NetBSD: lsym_preprocessing.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_preprocessing, which represents a '#' that starts
 * a preprocessing line.
 *
 * The whole preprocessing line is processed separately from the main source
 * code, without much tokenizing or parsing.
 */

// TODO: test '#' in the middle of a non-preprocessing line
// TODO: test stringify '#'
// TODO: test token paste '##'

//indent input
// TODO: add input
//indent end

//indent run-equals-input


/*
 * Whitespace in the following preprocessing directives is preserved.
 */
//indent input
#define space ' '		/* the 'define' is followed by a space */
#define	tab '\t'		/* the 'define' is followed by a tab */
#if   0				/* 3 spaces */
#elif		0		/* 2 tabs */
#elif	0	>	1	/* tabs between the tokens */
#endif
//indent end

//indent run-equals-input

// TODO: #define unfinished_string "...
// TODO: #define unfinished_char '...
// TODO: # 123 "file.h"
// TODO: backslash-newline
// TODO: block comment
// TODO: line comment
