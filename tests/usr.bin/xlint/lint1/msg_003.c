/*	$NetBSD: msg_003.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_003.c"

// Test for message: '%s' declared in argument declaration list [3]

/*ARGSUSED*/
void
example(declare_struct, declare_union, declare_enum)
    /* expect+1: warning: 'incomplete struct struct_in_argument' declared in argument declaration list [3] */
    struct struct_in_argument *declare_struct;
    /* expect+1: warning: 'incomplete union union_in_argument' declared in argument declaration list [3] */
    union union_in_argument *declare_union;
    /* expect+1: warning: 'enum enum_in_argument' declared in argument declaration list [3] */
    enum enum_in_argument *declare_enum;
{
}
