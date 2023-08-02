/*	$NetBSD: msg_085.c,v 1.8 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_085.c"

// Test for message: dubious tag declaration '%s %s' [85]

/*
 * Declarations of structs/unions/enums in parameter lists are allowed
 * but useless.
 */

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: dubious tag declaration 'struct in_parameter' [85] */
void declare_struct(struct in_parameter *);
/* expect+1: warning: dubious tag declaration 'union in_parameter' [85] */
void declare_union(union in_parameter *);
/* expect+1: warning: dubious tag declaration 'enum in_parameter' [85] */
void declare_enum(enum in_parameter *);

/* expect+1: warning: struct 'ok' never defined [233] */
struct ok;
extern int ok(struct ok *);
