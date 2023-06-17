/* $NetBSD: lsym_typedef.c,v 1.9 2023/06/17 22:09:24 rillig Exp $ */

/*
 * Tests for the token lsym_typedef, which represents the keyword 'typedef'
 * for giving a type an additional name.
 */

/*
 * Since 2019-04-04 and before lexi.c 1.169 from 2022-02-12, indent placed all
 * enum constants except the first too far to the right, as if it were a
 * statement continuation, but only if the enum declaration followed a
 * 'typedef'.
 *
 * https://gnats.netbsd.org/55453
 */
//indent input
typedef enum {
    TC1,
    TC2
} T;

enum {
    EC1,
    EC2
} E;
//indent end

//indent run -ci4 -i4
typedef enum {
    TC1,
    TC2
} T;

enum {
    EC1,
    EC2
}		E;
//indent end

//indent run -ci2
typedef enum {
	TC1,
	TC2
} T;

enum {
	EC1,
	EC2
}		E;
//indent end


/*
 * Contrary to declarations, type definitions are not affected by the option
 * '-di'.
 */
//indent input
typedef int number;
//indent end

//indent run-equals-input


/*
 * Ensure that a typedef declaration does not introduce an unnecessary line
 * break after the '}'.
 */
//indent input
typedef struct {
	int member;
	bool bit:1;
} typedef_name;

struct {
	int member;
	bool bit:1;
} var_name;
//indent end

//indent run
typedef struct {
	int		member;
	bool		bit:1;
} typedef_name;

struct {
	int		member;
	bool		bit:1;
}		var_name;
//indent end

//indent run-equals-input -di0


/*
 * When 'typedef' or a tag is followed by a name, that name marks a type and a
 * following '*' marks a pointer type.
 */
//indent input
{
	// $ Syntactically invalid but shows that '*' is not multiplication.
	a = typedef name * y;
	a = (typedef x * y)z;
}
//indent end

//indent run
{
	// $ Everything before the '*' is treated as a declaration.
	a = typedef name *y;
	a = (typedef x *y)z;
}
//indent end
