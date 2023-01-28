/*	$NetBSD: gcc_bit_field_types.c,v 1.8 2023/01/28 08:36:17 rillig Exp $	*/
# 3 "gcc_bit_field_types.c"

struct incompatible {
	int dummy;
};
void reveal_type(struct incompatible);

/*
 * https://gcc.gnu.org/onlinedocs/gcc/Structures-unions-enumerations-and-bit-fields-implementation.html
 *
 * "Other integer types, such as long int, and enumerated types are permitted
 * even in strictly conforming mode."
 *
 * See msg_035.c.
 */

struct example {
	int int_flag: 1;
	unsigned int unsigned_int_flag: 1;
	long long_flag: 1;
	unsigned long unsigned_long_flag: 1;
	long long long_long_flag: 1;
	unsigned long long unsigned_long_long_flag: 1;
	/* expect+1: warning: illegal bit-field type 'double' [35] */
	double double_flag: 1;
};

struct large_bit_field {
	unsigned long long member: 48;
};

unsigned long long
promote_large_bit_field(struct large_bit_field lbf)
{
	/*
	 * Before tree.c 1.281 from 2021-05-04:
	 * lint: assertion "len == size_in_bits(INT)" failed
	 *     in promote at tree.c:1698
	 */
	return lbf.member & 0xf;
}

/*
 * C99 6.7.2.1p4 says: "A bit-field shall have a type that is a qualified or
 * unqualified version of _Bool, signed int, unsigned int, or some other
 * implementation-defined type."
 *
 * The wording of that constraint does not disambiguate whether it is talking
 * about the declared underlying type of the storage unit or the expression
 * type when evaluating a bit-field as an rvalue.
 */
void
type_of_bit_field(void)
{
	struct {
		unsigned bits:3;
	} s;

	/*
	 * Lint interprets the type of the bit-field is 'unsigned:3', which
	 * matches the non-bit-field type 'unsigned' in the _Generic
	 * expression.  (XXX: May or may not be intended.)
	 *
	 * The _Generic expression prevents the integer promotions from
	 * getting applied as part of the function argument conversions.
	 *
	 * GCC 11 says: error: '_Generic' selector of type 'unsigned char:3'
	 * is not compatible with any association
	 *
	 * Clang 15 says: error: controlling expression type 'unsigned int'
	 * not compatible with any generic association type
	 *
	 * TCC says: error: type 'unsigned int' does not match any association
	 *
	 * MSVC 19 says: error C7702: no compatible type for 'unsigned int'
	 * in _Generic association list
	 *
	 * ICC 2021.7.1 says: error: no association matches the selector type
	 * "unsigned int"
	 */
	/* expect+4: warning: passing 'pointer to unsigned int' to incompatible 'struct incompatible', arg #1 [155] */
	reveal_type(_Generic(s.bits,
	    int: (int *)0,
	    unsigned int: (unsigned int *)0
	));

	/*
	 * When lint promotes the bit-field as part of the function argument
	 * conversions, the type 'unsigned:3' gets promoted to 'int', as that
	 * is the smallest candidate type that can represent all possible
	 * values from 'unsigned:3', see promote_c90.  Maybe that's wrong,
	 * maybe not, the compilers disagree so lint can offer yet another
	 * alternative interpretation.
	 *
	 * GCC 12 says: expected 'struct incompatible' but argument is of
	 * type 'unsigned char:3'
	 *
	 * Clang 15 says: error: passing 'unsigned int' to parameter of
	 * incompatible type 'struct incompatible'
	 *
	 * MSVC 19 says: error C2440: 'function': cannot convert from
	 * 'unsigned int' to 'incompatible'
	 */
	/* expect+1: warning: passing 'unsigned int:3' to incompatible 'struct incompatible', arg #1 [155] */
	reveal_type(s.bits);
}
