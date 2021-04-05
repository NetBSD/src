/*	$NetBSD: msg_019.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_019.c"

// Test for message: void type for '%s' [19]

void global_variable;		/* expect: 19 */

static void unit_variable;	/* expect: 19 *//* expect: 226 */

void
function(void parameter)	/* expect: 61 *//* expect: 231 */
{
	void local_variable;	/* expect: 19 */
}
