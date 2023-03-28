/*	$NetBSD: msg_085.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_085.c"

// Test for message: dubious tag declaration '%s %s' [85]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: dubious tag declaration 'struct in_argument' [85] */
void declare_struct(struct in_argument *);
/* expect+1: warning: dubious tag declaration 'union in_argument' [85] */
void declare_union(union in_argument *);
/* expect+1: warning: dubious tag declaration 'enum in_argument' [85] */
void declare_enum(enum in_argument *);

/* expect+1: warning: struct 'ok' never defined [233] */
struct ok;
extern int ok(struct ok *);
