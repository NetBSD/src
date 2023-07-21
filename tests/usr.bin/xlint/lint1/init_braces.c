/*	$NetBSD: init_braces.c,v 1.9 2023/07/21 06:02:07 rillig Exp $	*/
# 3 "init_braces.c"

/*
 * Test initialization with excess braces around expressions.
 *
 * See also:
 *	C99 6.7.8
 *	C11 6.7.9
 */

/* lint1-extra-flags: -X 351 */

void
init_int(void)
{
	/* gcc-expect+4: error: invalid initializer */
	/* clang-expect+3: error: array initializer must be an initializer list */
	/* expect+2: error: {}-enclosed or constant initializer of type 'array[unknown_size] of int' required [181] */
	/* expect+1: error: empty array declaration for 'num0' [190] */
	int num0[] = 0;
	int num1[] = { 1 };
	/* gcc-expect+2: warning: braces around scalar initializer */
	/* clang-expect+1: warning: braces around scalar initializer */
	int num2[] = {{ 1 }};
	/* gcc-expect+3: warning: braces around scalar initializer */
	/* gcc-expect+2: warning: braces around scalar initializer */
	/* clang-expect+1: warning: too many braces around scalar initializer */
	int num3[] = {{{ 1 }}};
	/* gcc-expect+5: warning: braces around scalar initializer */
	/* gcc-expect+4: warning: braces around scalar initializer */
	/* gcc-expect+3: warning: braces around scalar initializer */
	/* clang-expect+2: warning: too many braces around scalar initializer */
	/* clang-expect+1: warning: too many braces around scalar initializer */
	int num4[] = {{{{ 1 }}}};
}

void
init_string(void)
{
	char name0[] = "";
	char name1[] = { "" };
	/* gcc-expect+5: warning: braces around scalar initializer */
	/* gcc-expect+4: warning: initialization of 'char' from 'char *' makes integer from pointer without a cast */
	/* clang-expect+3: warning: incompatible pointer to integer conversion initializing 'char' with an expression of type 'char [1]' */
	/* clang-expect+2: warning: braces around scalar initializer */
	/* expect+1: warning: illegal combination of integer 'char' and pointer 'pointer to char' [183] */
	char name2[] = {{ "" }};
	/* gcc-expect+6: warning: braces around scalar initializer */
	/* gcc-expect+5: warning: braces around scalar initializer */
	/* gcc-expect+4: warning: initialization of 'char' from 'char *' makes integer from pointer without a cast */
	/* clang-expect+3: warning: too many braces around scalar initializer */
	/* clang-expect+2: warning: incompatible pointer to integer conversion initializing 'char' with an expression of type 'char [1]' */
	/* expect+1: warning: illegal combination of integer 'char' and pointer 'pointer to char' [183] */
	char name3[] = {{{ "" }}};
	/* gcc-expect+8: warning: braces around scalar initializer */
	/* gcc-expect+7: warning: braces around scalar initializer */
	/* gcc-expect+6: warning: braces around scalar initializer */
	/* gcc-expect+5: warning: initialization of 'char' from 'char *' makes integer from pointer without a cast */
	/* clang-expect+4: warning: too many braces around scalar initializer */
	/* clang-expect+3: warning: too many braces around scalar initializer */
	/* clang-expect+2: warning: incompatible pointer to integer conversion initializing 'char' with an expression of type 'char [1]' */
	/* expect+1: warning: illegal combination of integer 'char' and pointer 'pointer to char' [183] */
	char name4[] = {{{{ "" }}}};
}

/* C11 6.7.2.1p13 */
unsigned long
init_anonymous_struct_and_union(void)
{
	struct time {
		unsigned long ns;
	};

	struct times {
		struct time t0;
		struct time t1;
	};

	struct outer {
		union {
			struct {
				struct times times;
			};
		};
	};

	struct outer var = {	/* struct outer */
		{		/* anonymous union */
			{	/* anonymous struct */
				.times = {
					.t0 = { .ns = 0, },
					.t1 = { .ns = 0, },
				},
			},
		},
	};

	return var.times.t0.ns;
}

// Initializers may designate members from unnamed struct/union members.
// Example code adapted from jemalloc 5.1.0, jemalloc.c, init_lock.
unsigned char
init_unnamed_union(void)
{
	struct init_unnamed_union {
		union {
			struct {
				struct padded_union {
					unsigned char pad1[3];
					union {
						unsigned char u1;
						unsigned char u2;
					};
					unsigned char pad2[3];
				} padded_union;
			};
		};
	};

	struct init_unnamed_union var = {
		{
			{
				.padded_union = {
					.pad1 = { 0, 0, 0 },
					.u1 = 0,
					.pad2 = { 0, 0, 0 },
				},
			}
		},
	};
	return var.padded_union.u1;
}
