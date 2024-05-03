/*	$NetBSD: expr_sizeof.c,v 1.18 2024/05/03 19:16:13 rillig Exp $	*/
# 3 "expr_sizeof.c"

/*
 * C99 6.5.3.4 "The sizeof operator"
 * C11 6.5.3.4 "The sizeof operator"
 */

/* lint1-extra-flags: -X 351 */

/*
 * A sizeof expression can either take a type name or an expression.
 */

void sink(unsigned long);

struct {
	int member;
} s, *ps;

/*
 * In a sizeof expression taking a type name, the type name must be enclosed
 * in parentheses.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_int[-(int)sizeof(int)];

/*
 * In a sizeof expression taking an expression, the expression may or may not
 * be enclosed in parentheses, like any other expression.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_zero[-(int)sizeof(0)];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_zero[-(int)sizeof 0];

/*
 * Even though 's' is not a constant expression, 'sizeof s' is.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_global_var[-(int)sizeof s];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_var[-(int)sizeof(s)];

/*
 * Even though 'sizeof(s)' may look like a function call expression, the
 * parentheses around 's' are ordinary parentheses and do not influence the
 * precedence.
 *
 * Therefore, the '.' following the '(s)' takes precedence over the 'sizeof'.
 * Same for the '->' following the '(ps)'.  Same for the '[0]' following the
 * '(arr)'.
 */
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_struct_member[-(int)sizeof(s).member];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_paren_global_ptr_struct_member[-(int)sizeof(ps)->member];
int arr[] = { 1, 2, 3 };
/* expect+1: error: negative array dimension (-3) [20] */
typedef int arr_count[-(int)sizeof(arr) / (int)sizeof(arr)[0]];

/* FIXME: 'n' is actually used, for the variable length array. */
/* expect+2: warning: parameter 'n' unused in function 'variable_length_array' [231] */
void
variable_length_array(int n)
{
	int local_arr[n + 5];

	/*
	 * Since the array length is not constant, it cannot be used in a
	 * typedef.  Code like this is already rejected by the compiler.  For
	 * simplicity, lint assumes that the array has length 1.
	 */
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int sizeof_local_arr[-(int)sizeof(local_arr)];
}

void
bit_fields(void)
{
	struct {
		_Bool flag0:1;
		_Bool flag1:1;
		_Bool flag2:1;
	} flags;
	/* expect+1: error: negative array dimension (-1) [20] */
	typedef int sizeof_flags[-(int)sizeof(flags)];

	struct {
		struct {
			_Bool flag0:1;
			_Bool flag1:1;
			_Bool flag2:1;
		};
	} anonymous_flags;
	/* expect+1: error: negative array dimension (-1) [20] */
	typedef int sizeof_anonymous_flags[-(int)sizeof(anonymous_flags)];

	struct {
		unsigned int bits0:16;
		unsigned int bits1:16;
	} same_storage_unit;
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int sizeof_same_storage_unit[-(int)sizeof(same_storage_unit)];

	// Detect whether a bit-field can span multiple storage units.
	// If so, the size is 12, if not, the size is 16.
	struct {
		unsigned int bits0:24;
		unsigned int bits1:24;
		unsigned int bits2:24;
		unsigned int bits3:24;
	} cross_storage_unit;
	/* expect+1: error: negative array dimension (-16) [20] */
	typedef int sizeof_cross_storage_unit[-(int)sizeof(cross_storage_unit)];

	/*
	 * The bit-fields in a struct may be merged into the same storage
	 * units, even if their types differ. GCC 10, Clang 15 and lint all
	 * agree in packing the first group of bit-fields and the char into
	 * 4 bytes, even though their underlying types differ.  The second
	 * group of bit-fields gets its own storage unit.
	 */
	struct mixed {
		_Bool flag0:1;
		signed int signed0:1;
		unsigned int unsigned0:1;
		char ch[3];
		_Bool flag1:1;
		signed int signed1:1;
		unsigned int unsigned1:1;
	} mixed;
	/* expect+1: error: negative array dimension (-8) [20] */
	typedef int sizeof_mixed[-(int)sizeof(mixed)];
	/* expect+3: error: negative array dimension (-1) [20] */
	typedef int offsetof_mixed_ch[
	    -(int)__builtin_offsetof(struct mixed, ch)
	];
}

/*
 * Ensure that anonymous structs and unions are handled correctly.  They were
 * added in C11, and lint did not properly support them until 2023.
 */
void
anonymous_struct_and_union(void)
{
	struct {
		union {
			unsigned char uc16[16];
			unsigned char uc32[32];
		};
	} su_16_32;
	/* expect+1: error: negative array dimension (-32) [20] */
	typedef int sizeof_su_16_32[-(int)sizeof(su_16_32)];

	union {
		struct {
			unsigned char uc16[16];
			unsigned char uc32[32];
		};
	} us_16_32;
	/* expect+1: error: negative array dimension (-48) [20] */
	typedef int sizeof_us_16_32[-(int)sizeof(us_16_32)];
}


void
sizeof_errors(void)
{
	/* expect+1: error: cannot take size/alignment of void [146] */
	typedef int sizeof_void[-(int)sizeof(void)];

	/*
	 * A 'void array' gets replaced with an 'int array' before
	 * type_size_in_bits gets to see it, thus the 256 * 4 = 1024.
	 */
	/* expect+2: error: illegal use of 'void' [18] */
	/* expect+1: error: negative array dimension (-1024) [20] */
	typedef int sizeof_void_array[-(int)sizeof(void[256])];

	/* expect+1: warning: enum 'incomplete_enum' never defined [235] */
	enum incomplete_enum;
	/* expect+2: warning: cannot take size/alignment of incomplete type [143] */
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int sizeof_incomplete_enum[-(int)sizeof(enum incomplete_enum)];
}


/*
 * Due to the 'double' member, the alignment of this struct is 8, so the size
 * has to be 24 (or at least divisible by 8), otherwise the 'double' member
 * would not get the correct alignment in an array of this struct.
 */
struct s24 {
	char c0;
	double d8;
	char c16;
};
/* expect+1: error: negative array dimension (-24) [20] */
typedef int sizeof_s24[-(int)sizeof(struct s24)];

void
sizeof_array_parameter(short arr[12345])
{
	// The size of an array parameter is the size of the decayed pointer.
	// Subtracting 'sizeof(void *)' makes the test platform-independent.
	typedef int sizeof_arr[-(int)(sizeof arr - sizeof(void *))];

	// The 2 comes from 'sizeof(short)', as the type 'array[size] of elem'
	// decays into the type 'pointer to elem', not 'pointer to array[size]
	// of elem'.
	/* expect+1: error: negative array dimension (-2) [20] */
	typedef int sizeof_arr_elem[-(int)(sizeof *arr)];
}


void
sequence_of_structs(void)
{
	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef unsigned long long uint64_t;

	union fp_addr {
		uint64_t fa_64;
		struct {
			uint32_t fa_off;
			uint16_t fa_seg;
			uint16_t fa_opcode;
		} fa_32;
	} __packed _Alignas(4);

	struct fpacc87 {
		uint64_t f87_mantissa;
		uint16_t f87_exp_sign;
	} __packed _Alignas(2);

	// FIXME: This otherwise unused struct declaration influences the
	// offsets checked below. Without this struct, sizeof(struct save87)
	// is calculated correctly as 108 below.
	struct fpaccfx {
		struct fpacc87 r _Alignas(16);
	};

	struct save87 {
		uint16_t s87_cw _Alignas(4);
		uint16_t s87_sw _Alignas(4);
		uint16_t s87_tw _Alignas(4);
		union fp_addr s87_ip;
		union fp_addr s87_dp;
		struct fpacc87 s87_ac[8];
	};

	/* expect+1: error: negative array dimension (-20) [20] */
	typedef int o1[-(int)((unsigned long)(&(((struct save87 *)0)->s87_dp)))];
	// FIXME: must be 28.
	/* expect+1: error: negative array dimension (-32) [20] */
	typedef int o2[-(int)((unsigned long)(&(((struct save87 *)0)->s87_ac)))];
	// FIXME: must be 108.
	/* expect+1: error: negative array dimension (-112) [20] */
	typedef int reveal[-(int)sizeof(struct save87)];
}
