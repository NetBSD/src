/*	$NetBSD: gcc_attribute_var.c,v 1.13 2024/09/28 15:51:40 rillig Exp $	*/
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
 * A GCC extension allows statement of the form __attribute__((fallthrough)),
 * therefore, to avoid shift/reduce conflicts in the grammar, the attributes
 * cannot be part of the declaration specifiers between begin_type/end_type.
 */
void
ambiguity_for_attribute(void)
{
	__attribute__((unused)) _Bool var1;

	switch (1) {
	case 1:
		println();
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


void
anonymous_members(void)
{
	struct single_attribute_outer {
		struct single_attribute_inner {
			int member;
		} __attribute__(());
	} __attribute__(());

	struct multiple_attributes_outer {
		struct multiple_attributes_inner {
			int member;
		} __attribute__(()) __attribute__(());
	} __attribute__(()) __attribute__(());
}
