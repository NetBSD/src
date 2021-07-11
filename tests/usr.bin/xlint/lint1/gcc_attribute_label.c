/*	$NetBSD: gcc_attribute_label.c,v 1.2 2021/07/11 19:24:42 rillig Exp $	*/
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
error:
	__attribute__((__cold__));
	dead();

hot:
	__attribute__((__hot__));
	if (i < 0)
		goto error;
}
