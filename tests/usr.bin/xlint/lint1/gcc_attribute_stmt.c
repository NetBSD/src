/*	$NetBSD: gcc_attribute_stmt.c,v 1.3 2022/04/28 21:38:38 rillig Exp $	*/
# 3 "gcc_attribute_stmt.c"

/*
 * Tests for the GCC __attribute__ for statements.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Attributes.html
 */

void println(const char *);

void
attribute_fallthrough(int i)
{
	switch (i) {
	case 5:
		/*
		 * The attribute 'fallthrough' is only valid after a
		 * preceding statement. This is already caught by GCC, so
		 * lint does not need to care.
		 */
		__attribute__((__fallthrough__));
	case 3:
		println("odd");
		__attribute__((__fallthrough__));
	case 2:
		/*
		 * Only the null statement can have the attribute
		 * 'fallthrough'. This is already caught by GCC, so
		 * lint does not need to care.
		 */
		/* expect+2: error: syntax error '__attribute__' [249] */
		println("prime")
		    __attribute__((__fallthrough__));
	}
}
