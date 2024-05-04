/*	$NetBSD: msg_005.c,v 1.6 2024/05/04 06:52:17 rillig Exp $	*/
# 3 "msg_005.c"

// Test for message: modifying typedef with '%s'; only qualifiers allowed [5]

typedef char char_alias;
typedef signed char schar_alias;
typedef unsigned char uchar_alias;
typedef short short_alias;
typedef unsigned short ushort_alias;
typedef int int_alias;
typedef unsigned int uint_alias;
typedef long long_alias;
typedef unsigned long ulong_alias;
typedef long long llong_alias;
typedef unsigned long long ullong_alias;
typedef float float_alias;
typedef double double_alias;
typedef long double ldouble_alias;
typedef float _Complex fcomplex_alias;
typedef double _Complex dcomplex_alias;
typedef long double _Complex lcomplex_alias;

/* expect+1: warning: modifying typedef with 'signed'; only qualifiers allowed [5] */
typedef char_alias signed err_s_char;
/* expect+1: warning: modifying typedef with 'unsigned'; only qualifiers allowed [5] */
typedef char_alias unsigned err_u_char;
/* expect+1: error: illegal type combination [4] */
typedef schar_alias signed err_s_schar;
/* expect+1: error: illegal type combination [4] */
typedef schar_alias unsigned err_u_schar;
/* expect+1: error: illegal type combination [4] */
typedef uchar_alias signed err_s_uchar;
/* expect+1: error: illegal type combination [4] */
typedef uchar_alias unsigned err_u_uchar;
/* expect+1: warning: modifying typedef with 'signed'; only qualifiers allowed [5] */
typedef short_alias signed err_s_short;
/* expect+1: warning: modifying typedef with 'unsigned'; only qualifiers allowed [5] */
typedef short_alias unsigned err_u_short;
/* expect+1: error: illegal type combination [4] */
typedef ushort_alias signed err_s_ushort;
/* expect+1: error: illegal type combination [4] */
typedef ushort_alias unsigned err_u_ushort;
/* expect+1: warning: modifying typedef with 'signed'; only qualifiers allowed [5] */
typedef int_alias signed err_s_int;
/* expect+1: warning: modifying typedef with 'unsigned'; only qualifiers allowed [5] */
typedef int_alias unsigned err_u_int;
/* expect+1: error: illegal type combination [4] */
typedef uint_alias signed err_s_uint;
/* expect+1: error: illegal type combination [4] */
typedef uint_alias unsigned err_u_uint;
/* expect+1: warning: modifying typedef with 'signed'; only qualifiers allowed [5] */
typedef long_alias signed err_s_long;
/* expect+1: warning: modifying typedef with 'unsigned'; only qualifiers allowed [5] */
typedef long_alias unsigned err_u_long;
/* expect+1: error: illegal type combination [4] */
typedef ulong_alias signed err_s_ulong;
/* expect+1: error: illegal type combination [4] */
typedef ulong_alias unsigned err_u_ulong;
/* expect+1: warning: modifying typedef with 'signed'; only qualifiers allowed [5] */
typedef llong_alias signed err_s_llong;
/* expect+1: warning: modifying typedef with 'unsigned'; only qualifiers allowed [5] */
typedef llong_alias unsigned err_u_llong;
/* expect+1: error: illegal type combination [4] */
typedef ullong_alias signed err_s_ullong;
/* expect+1: error: illegal type combination [4] */
typedef ullong_alias unsigned err_u_ullong;
/* expect+1: error: illegal type combination [4] */
typedef float_alias signed err_s_float;
/* expect+1: error: illegal type combination [4] */
typedef float_alias unsigned err_u_float;
/* expect+1: error: illegal type combination [4] */
typedef double_alias signed err_s_double;
/* expect+1: error: illegal type combination [4] */
typedef double_alias unsigned err_u_double;
/* expect+1: error: illegal type combination [4] */
typedef ldouble_alias signed err_s_ldouble;
/* expect+1: error: illegal type combination [4] */
typedef ldouble_alias unsigned err_u_ldouble;
/* expect+1: error: illegal type combination [4] */
typedef fcomplex_alias signed err_s_fcomplex;
/* expect+1: error: illegal type combination [4] */
typedef fcomplex_alias unsigned err_u_fcomplex;
/* expect+1: error: illegal type combination [4] */
typedef dcomplex_alias signed err_s_dcomplex;
/* expect+1: error: illegal type combination [4] */
typedef dcomplex_alias unsigned err_u_dcomplex;
/* expect+1: error: illegal type combination [4] */
typedef lcomplex_alias signed err_s_lcomplex;
/* expect+1: error: illegal type combination [4] */
typedef lcomplex_alias unsigned err_u_lcomplex;

/* expect+1: warning: modifying typedef with 'short'; only qualifiers allowed [5] */
typedef int_alias short err_short_int;
/* expect+1: error: illegal type combination [4] */
typedef long_alias short err_short_long;
/* expect+1: error: illegal type combination [4] */
typedef float_alias short err_short_float;

/* expect+1: error: illegal type combination [4] */
typedef char_alias long err_l_char;
/* expect+1: error: illegal type combination [4] */
typedef schar_alias long err_l_schar;
/* expect+1: error: illegal type combination [4] */
typedef uchar_alias long err_l_uchar;
/* expect+1: error: illegal type combination [4] */
typedef short_alias long err_l_short;
/* expect+1: error: illegal type combination [4] */
typedef ushort_alias long err_l_ushort;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef int_alias long err_l_int;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef uint_alias long err_l_uint;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef long_alias long err_l_long;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef ulong_alias long err_l_ulong;
/* expect+1: error: illegal type combination [4] */
typedef llong_alias long err_l_llong;
/* expect+1: error: illegal type combination [4] */
typedef ullong_alias long err_l_ullong;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef float_alias long err_l_float;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef double_alias long err_l_double;
/* expect+1: error: illegal type combination [4] */
typedef ldouble_alias long err_l_ldouble;
/* expect+1: error: illegal type combination [4] */
typedef fcomplex_alias long err_l_fcomplex;
/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef dcomplex_alias long err_l_dcomplex;
/* expect+1: error: illegal type combination [4] */
typedef lcomplex_alias long err_l_lcomplex;


/*
 * If the type qualifier comes before the type name, which would be the
 * natural order, the type name is interpreted as a new name, not as the one
 * referring to the typedef.  This makes the above type modifications even
 * more unlikely to be accidentally seen in practice.
 */
/* expect+1: error: syntax error 'prefix_long_number' [249] */
typedef long int_alias prefix_long_number;

/* Type qualifiers are OK. */
typedef int_alias const const_int;
