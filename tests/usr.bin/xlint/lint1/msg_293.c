/*	$NetBSD: msg_293.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_293.c"

// Test for message: argument %d must be 'char *' for PRINTFLIKE/SCANFLIKE [293]

/* expect+3: warning: argument 1 must be 'char *' for PRINTFLIKE/SCANFLIKE [293] */
/* expect+2: warning: argument 'msgid' unused in function 'my_printf' [231] */
/* PRINTFLIKE 1 */
void my_printf(int msgid, ...) {
}
