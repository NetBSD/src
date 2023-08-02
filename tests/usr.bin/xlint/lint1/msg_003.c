/*	$NetBSD: msg_003.c,v 1.7 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_003.c"

// Test for message: '%s' declared in parameter declaration list [3]

/* lint1-extra-flags: -X 351 */

/*ARGSUSED*/
void
example(declare_struct, declare_union, declare_enum)
    /* expect+1: warning: 'incomplete struct struct_in_parameter' declared in parameter declaration list [3] */
    struct struct_in_parameter *declare_struct;
    /* expect+1: warning: 'incomplete union union_in_parameter' declared in parameter declaration list [3] */
    union union_in_parameter *declare_union;
    /* expect+1: warning: 'enum enum_in_parameter' declared in parameter declaration list [3] */
    enum enum_in_parameter *declare_enum;
{
}
