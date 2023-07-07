/*	$NetBSD: msg_293.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_293.c"

// Test for message: argument %d must be 'char *' for PRINTFLIKE/SCANFLIKE [293]

/* lint1-extra-flags: -X 351 */

/* expect+3: warning: argument 1 must be 'char *' for PRINTFLIKE/SCANFLIKE [293] */
/* expect+2: warning: argument 'msgid' unused in function 'my_printf' [231] */
/* PRINTFLIKE 1 */
void my_printf(int msgid, ...) {
}
