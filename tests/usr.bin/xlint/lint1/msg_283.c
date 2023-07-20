/*	$NetBSD: msg_283.c,v 1.7 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_283.c"

// Test for message: argument number mismatch in comment /* %s */ [283]

/* Do not warn about unused parameters. */
/* lint1-extra-flags: -X 231,351 */

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
/* expect+1: warning: argument number mismatch in comment ** PRINTFLIKE ** [283] */
{
}
