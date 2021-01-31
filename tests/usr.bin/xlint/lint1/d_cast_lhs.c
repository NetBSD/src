/*	$NetBSD: d_cast_lhs.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_cast_lhs.c"

/* pointer casts are valid lhs lvalues */
struct sockaddr { };
void
foo() {
    unsigned long p = 6;
    ((struct sockaddr *)p) = 0;
}
