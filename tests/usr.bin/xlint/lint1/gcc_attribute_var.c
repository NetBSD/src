/*	$NetBSD: gcc_attribute_var.c,v 1.11 2023/07/15 21:47:35 rillig Exp $	*/
# 3 "gcc_attribute_var.c"

/*
 * Tests for the GCC __attribute__ for variables.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html
 */

/* lint1-extra-flags: -X 351 */

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

void println(void);

/*
 * Since cgram.y 1.294 from 2021-07-10, lint did not accept declarations that
 * started with __attribute__, due to a newly and accidentally introduced
 * shift/reduce conflict in the grammar.
 *
 * A GCC extension allows statement of the form __attribute__((fallthrough)),
 * thus starting with __attribute__.  This is the 'shift' in the conflict.
 * The 'reduce' in the conflict was begin_type.
 *
 * Before cgram 1.294, the gcc_attribute was placed outside the pair of
 * begin_type/end_type, exactly to resolve this conflict.
 *
 * Conceptually, it made sense to put the __attribute__((unused)) between
 * begin_type and end_type, to make it part of the declaration-specifiers.
 * This change introduced the hidden conflict though.
 *
 * Interestingly, the number of shift/reduce conflicts did not change in
 * cgram 1.294, the conflicts were just resolved differently than before.
 *
 * To prevent this from happening again, make sure that declarations as well
 * as statements can start with gcc_attribute.
 */
void
ambiguity_for_attribute(void)
{
	/* expect+1: warning: 'var1' unused in function 'ambiguity_for_attribute' [192] */
	__attribute__((unused)) _Bool var1;

	switch (1) {
	case 1:
		println();
		/* expect+1: warning: 'var2' unused in function 'ambiguity_for_attribute' [192] */
		__attribute__((unused)) _Bool var2;
		__attribute__((fallthrough));
		case 2:
			println();
	}
}

void
attribute_after_array_brackets(
    const char *argv[] __attribute__((__unused__))
)
{
}

struct attribute_in_member_declaration {
	int __attribute__(())
	    x __attribute__(()),
	    y __attribute__(());

	unsigned int __attribute__(())
	    bit1:1 __attribute__(()),
	    bit2:2 __attribute__(()),
	    bit3:3 __attribute__(());
};
