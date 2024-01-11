/*	$NetBSD: msg_141.c,v 1.10 2024/01/11 20:25:04 rillig Exp $	*/
# 3 "msg_141.c"

// Test for message: operator '%s' produces integer overflow [141]

/* lint1-extra-flags: -h -X 351 */

/*
 * Before tree.c 1.347 from 2021-08-23, lint wrongly warned about integer
 * overflow in '-'.
 */
int signed_int_max = (1U << 31) - 1;

/*
 * Before tree.c 1.347 from 2021-08-23, lint wrongly warned about integer
 * overflow in '-'.
 */
unsigned int unsigned_int_max = (1U << 31) - 1;

/* expect+1: warning: operator '+' produces integer overflow [141] */
int int_overflow = (1 << 30) + (1 << 30);

/* expect+2: warning: operator '+' produces integer overflow [141] */
/* expect+1: warning: initialization of unsigned with negative constant [221] */
unsigned int intermediate_overflow = (1 << 30) + (1 << 30);

unsigned int no_overflow = (1U << 30) + (1 << 30);

/* expect+1: warning: operator '-' produces integer overflow [141] */
unsigned int unsigned_int_min = 0U - (1U << 31);

/* expect+1: warning: operator '-' produces integer overflow [141] */
unsigned int unsigned_int_min_unary = -(1U << 31);

enum {
	INT_MAX = 2147483647,
	INT_MIN = -INT_MAX - 1,
};

unsigned long long overflow_unsigned[] = {

	// unary '+'

	+0U,
	+~0U,
	+~0ULL,

	// unary '-'

	-0U,
	-~0U,
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	-(1ULL << 63),
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	-(1U << 31),

	// '~'

	~0U,
	~~0U,
	~0ULL,
	~~0ULL,

	// '*'

	0x10001U * 0xffffU,
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	0x10000U * 0x10000U,
	0x10000ULL * 0x10000U,
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	0x100000000ULL * 0x100000000ULL,

	// '/'

	0xFFFFFFFFU / ~0U,
	/* expect+1: error: division by 0 [139] */
	0xFFFFFFFFU / 0U,

	// '%'

	0xFFFFFFFFU % ~0U,
	/* expect+1: error: modulus by 0 [140] */
	0xFFFFFFFFU % 0U,

	// '+'

	0U + 0U,
	~0U + 0U,
	0U + ~0U,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	~0U + 1U,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	1U + ~0U,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	~0U + ~0U,

	// '-'

	0U - 0U,
	0U - 1U,
	0U - ~0U,

	// '<<'

	256U << 23,
	/* expect+1: warning: operator '<<' produces integer overflow [141] */
	256U << 24,

	// '>>'

	10U >> 1,
	-10U >> 1,

	// bitwise operators

	1U & -1U,
	1U ^ -1U,
	1U | -1U,
};

long long overflow_signed[] = {

	// unary '+'

	+(unsigned char)255,
	+0x7fffffffffffffffLL,

	// unary '-'

	-(unsigned char)255,
	-0x7fffffffffffffffLL,

	// '~'

	~0,
	~1,
	~1,
	~-1,

	// '*'

	/* expect+1: warning: operator '*' produces integer overflow [141] */
	0x10000 * 0x8000,
	/* expect+1: warning: operator '*' produces integer overflow [141] */
	0x10000 * 0x10000,
	0x10000LL * 0x10000,
	/* TODO: expect+1: warning: operator '*' produces integer overflow [141] */
	// TODO: don't abort: 0x100000000 * 0x100000000

	// '/'

	-1 / INT_MIN,
	-1 / INT_MAX,
	/* expect+1: warning: operator '/' produces integer overflow [141] */
	INT_MIN / -1,
	INT_MAX / -1,

	// '%'

	-1 % INT_MIN,
	-1 % INT_MAX,
	INT_MIN % -1,
	INT_MAX % -1,

	// '+'

	INT_MIN + 0,
	0 + INT_MAX,
	INT_MAX + 0,
	0 + INT_MIN,
	INT_MIN + INT_MAX,
	INT_MAX + INT_MIN,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	INT_MIN + -1,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	-1 + INT_MIN,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	INT_MIN + INT_MIN,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	INT_MAX + 1,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	1 + INT_MAX,
	/* expect+1: warning: operator '+' produces integer overflow [141] */
	INT_MAX + INT_MAX,

	// '-'

	INT_MIN - 0,
	INT_MIN - -1,
	INT_MAX - 0,
	INT_MAX - 1,
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	INT_MIN - 1,
	/* expect+1: warning: operator '-' produces integer overflow [141] */
	INT_MAX - -1,

	// '<<'

	256 << 23,		// TODO: overflow
	/* expect+1: warning: operator '<<' produces integer overflow [141] */
	256 << 24,

	// '>>'

	10 >> 1,
	-10 >> 1,

	// bitwise operators

	1 & -1,
	1 ^ -1,
	1 | -1,
};
