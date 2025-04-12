/*	$NetBSD: msg_004.c,v 1.10 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_004.c"

// Test for message: invalid type combination [4]
//
// See also:
//	msg_005.c

/* lint1-extra-flags: -X 351 */

// Lint does not detect "two or more data types", but GCC does.
signed double signed_double;

int ok_int;
double ok_double;
float _Complex ok_float_complex;

/* expect+2: error: invalid type for _Complex [308] */
/* expect+1: error: invalid type combination [4] */
int _Complex invalid_int_complex;

char enum {
	CHAR
};
/* expect-1: error: invalid type combination [4] */

long struct {
	int member;
};
/* expect-1: error: invalid type combination [4] */

struct str {
};
/* expect+1: error: invalid type combination [4] */
struct str int struct_str_int;

/* expect+1: error: invalid type combination [4] */
unsigned signed int unsigned_signed_int;

/* expect+1: error: invalid type combination [4] */
unsigned unsigned int unsigned_unsigned_int;

/* expect+1: error: invalid type combination [4] */
long long long int long_long_long_int;

/* expect+1: error: invalid type combination [4] */
short double short_double;

double short double_short;

/* expect+1: error: invalid type combination [4] */
char double short char_double_short;
