/* $NetBSD: token_string_prefix.c,v 1.2 2021/10/22 19:46:41 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for strings of wide characters, which are prefixed with 'L'.
 *
 * See FreeBSD r309220.
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


#indent input
wchar_t wide_char[] = L'w';
#indent end

#indent run-equals-input -di0
