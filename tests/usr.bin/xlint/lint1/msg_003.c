/*	$NetBSD: msg_003.c,v 1.9 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_003.c"

// Test for message: '%s' declared in parameter declaration list [3]

/* lint1-extra-flags: -X 351 */

/*ARGSUSED*/
void
/* expect+1: warning: function definition for 'example' with identifier list is obsolete in C23 [384] */
example(declare_struct, declare_union, declare_enum)
    /* expect+1: warning: 'incomplete struct struct_in_parameter' declared in parameter declaration list [3] */
    struct struct_in_parameter *declare_struct;
    /* expect+1: warning: 'incomplete union union_in_parameter' declared in parameter declaration list [3] */
    union union_in_parameter *declare_union;
    /* expect+1: warning: 'enum enum_in_parameter' declared in parameter declaration list [3] */
    enum enum_in_parameter *declare_enum;
{
}
