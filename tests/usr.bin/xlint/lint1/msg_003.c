/*	$NetBSD: msg_003.c,v 1.2 2021/01/31 09:21:24 rillig Exp $	*/
# 3 "msg_003.c"

// Test for message: %s declared in argument declaration list [3]

/*ARGSUSED*/
void
example(declare_struct, declare_union, declare_enum)
    struct struct_in_argument *declare_struct;
    union union_in_argument *declare_union;
    enum enum_in_argument *declare_enum;
{
}
