/* $NetBSD: token_string_prefix.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for strings of wide characters, which are prefixed with 'L'.
 */

#indent input
wchar_t wide_string[] = L"wide string";
#indent end

/*
 * Regardless of the line length, the 'L' must never be separated from the
 * string literal.
 */
#indent run-equals-input -di0
#indent run-equals-input -di0 -l25
#indent run-equals-input -di0 -l1
