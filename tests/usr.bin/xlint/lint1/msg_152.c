/*	$NetBSD: msg_152.c,v 1.3 2021/03/16 23:39:41 rillig Exp $	*/
# 3 "msg_152.c"

// Test for message: argument cannot have unknown size, arg #%d [152]

struct incomplete;			/* expect: 233 */

void callee(struct incomplete);		/* expect: 31 */

void
caller(void)
{
	struct incomplete local_var;	/* expect: 31 */
	callee(local_var);		/* expect: 152 */
}
