/*	$NetBSD: msg_257.c,v 1.7 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_257.c"

// Test for message: extra characters in lint comment [257]

/* lint1-extra-flags: -X 351 */

void take(const void *);

/* expect+1: warning: extra characters in lint comment [257] */
/* FALLTHROUGH 3 */

/* expect+1: warning: extra characters in lint comment [257] */
/* FALLTHROUGH extra */

/* PRINTFLIKE 7 */
void
my_printf(const char *fmt)
/* expect+1: warning: argument number mismatch in comment ** PRINTFLIKE ** [283] */
{
	take(fmt);
}

/* expect+1: warning: extra characters in lint comment [257] */
/* SCANFLIKE extra */
void
my_scanf(const char *fmt)
{
	take(fmt);
}
