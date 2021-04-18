/*	$NetBSD: msg_260.c,v 1.3 2021/04/18 07:31:47 rillig Exp $	*/
# 3 "msg_260.c"

// Test for message: previous declaration of %s [260]

/* lint1-extra-flags: -r */

# 100 "header.h" 1
struct s {		/* expect: 260 */
    int member;
};
# 13 "msg_260.c" 2

# 200 "header.h" 1
union s {		/* expect: tag redeclared *//* expect: 260 */
    int member;
};
/*
 * FIXME: the stack trace for the 260 is wrong, as the 260 is included from
 * line 8, not from line 14.
 */
# 19 "msg_160.c" 2

union s {		/* expect: tag redeclared */
    int member;
};
/*
 * FIXME: the stack trace for the 260 is missing.
 */
