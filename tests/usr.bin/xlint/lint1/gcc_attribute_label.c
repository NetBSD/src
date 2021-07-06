/*	$NetBSD: gcc_attribute_label.c,v 1.1 2021/07/06 17:33:07 rillig Exp $	*/
# 3 "gcc_attribute_label.c"

/*
 * Tests for the GCC __attribute__ for labels.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html
 */

void dead(void);

void
test(int i)
{
	if (i < 1000)
		goto hot;
	/* TODO: add quotes to message 232 */
	/*FIXME*//* expect+1: warning: label error unused in function test [232] */
error:
	__attribute__((__cold__));
	dead();

hot:
	/* expect+1: error: syntax error '__hot__' [249] */
	__attribute__((__hot__));
	/*FIXME*//* expect+1: error: 'i' undefined [99] */
	if (i < 0)
		/* TODO: add quotes to message 23 */
		/* expect+1: warning: undefined label error [23] */
		goto error;
}
