/*	$NetBSD: msg_085.c,v 1.3 2021/01/31 09:21:24 rillig Exp $	*/
# 3 "msg_085.c"

// Test for message: dubious tag declaration: %s %s [85]

void declare_struct(struct in_argument *);	/* expect: 85 */
void declare_union(union in_argument *);	/* expect: 85 */
void declare_enum(enum in_argument *);		/* expect: 85 */

struct ok;					/* expect: 233 */
extern int ok(struct ok *);
