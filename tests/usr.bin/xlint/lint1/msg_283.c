/*	$NetBSD: msg_283.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_283.c"

// Test for message: argument number mismatch with directive: /* %s */ [283]

/* Do not warn about unused parameters. */
/* lint1-extra-flags: -X 231 */

/* PRINTFLIKE */
void
printflike_comment(const char *fmt)
{
}

/* PRINTFLIKE 0 */
void
printflike_0_comment(const char *fmt)
{
}

/* PRINTFLIKE 2 */
void
printflike_2_comment(int a, const char *fmt)
{
}

/* PRINTFLIKE 3 */
void
printflike_3_comment(int a, const char *fmt)
/* expect+1: warning: argument number mismatch with directive: ** PRINTFLIKE ** [283] */
{
}
