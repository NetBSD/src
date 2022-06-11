/*	$NetBSD: msg_260.c,v 1.5 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_260.c"

// Test for message: previous declaration of '%s' [260]

/* lint1-extra-flags: -r */

# 100 "header.h" 1
/* expect+1: previous declaration of 's' [260] */
struct s {
    int member;
};
# 14 "msg_260.c" 2

# 200 "header.h" 1
/* expect+2: error: struct tag 's' redeclared as union [46] */
/* expect+1: previous declaration of 's' [260] */
union s {
    int member;
};
/*
 * FIXME: the stack trace for the message 260 is wrong, as the previous
 * declaration is included from logical line msg_260.c:8, not from
 * msg_260.c:15.
 */
# 27 "msg_260.c" 2
/* expect+1: error: union tag 's' redeclared as union [46] */
union s {
    int member;
};
/*
 * FIXME: the stack trace for the 260 is missing.
 */
