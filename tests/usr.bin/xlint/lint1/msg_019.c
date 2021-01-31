/*	$NetBSD: msg_019.c,v 1.3 2021/01/31 11:23:01 rillig Exp $	*/
# 3 "msg_019.c"

// Test for message: void type for '%s' [19]

void global_variable;		/* expect: 19 */

static void unit_variable;	/* expect: 19, 226 */

void
function(void parameter)	/* expect: 61, 231 */
{
	void local_variable;	/* expect: 19 */
}
