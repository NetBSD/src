/*	$NetBSD: gcc_attribute_var.c,v 1.1 2021/07/06 17:33:07 rillig Exp $	*/
# 3 "gcc_attribute_var.c"

/*
 * Tests for the GCC __attribute__ for variables.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html
 */

void
write_to_page(unsigned index, char ch)
{
	static char page[4096]
	    __attribute__((__aligned__(4096)));

	page[index] = ch;
}

void
placement(
    __attribute__((__deprecated__)) int before,
    int __attribute__((__deprecated__)) between,
    int after __attribute__((__deprecated__))
);

/* just to trigger _some_ error, to keep the .exp file */
/* expect+1: error: syntax error 'syntax_error' [249] */
__attribute__((syntax_error));
