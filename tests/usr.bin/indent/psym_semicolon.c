/* $NetBSD: psym_semicolon.c,v 1.4 2023/05/12 22:36:15 rillig Exp $ */

/*
 * Tests for the parser symbol psym_0 (formerly named psym_semicolon), which
 * pushes a new statement on the stack.
 *
 * This token is never stored on the stack itself.
 */

//indent input
void func(void) { stmt(); }
//indent end

//indent run
void
func(void)
{
	stmt();
}
//indent end

//indent run -npsl
void func(void)
{
	stmt();
}
//indent end

//indent run -nfbs
void
func(void) {
	stmt();
}
//indent end
