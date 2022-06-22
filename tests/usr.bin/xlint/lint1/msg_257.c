/*	$NetBSD: msg_257.c,v 1.5 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_257.c"

// Test for message: extra characters in lint comment [257]

void take(const void *);

/* expect+1: warning: extra characters in lint comment [257] */
/* FALLTHROUGH 3 */

/* expect+1: warning: extra characters in lint comment [257] */
/* FALLTHROUGH extra */

/* PRINTFLIKE 7 */
void
my_printf(const char *fmt)
/* expect+1: warning: argument number mismatch with directive ** PRINTFLIKE ** [283] */
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
