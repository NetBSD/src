/* $NetBSD: lsym_typedef.c,v 1.7 2023/06/16 11:58:33 rillig Exp $ */

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
//indent end

//indent run -di0
typedef struct {
	int member;
// $ FIXME: No space after the ':' here.
	bool bit: 1;
}
// $ FIXME: No linebreak here.
typedef_name;
//indent end
