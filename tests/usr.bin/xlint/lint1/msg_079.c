/*	$NetBSD: msg_079.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_079.c"

// Test for message: dubious escape \%c [79]

/* lint1-extra-flags: -X 351 */

int my_printf(const char *, ...);

void
print_color(_Bool red, _Bool green, _Bool blue)
{
	/* expect+1: warning: dubious escape \e [79] */
	my_printf("\e[%dm",
	    30 + (red ? 1 : 0) + (green ? 2 : 0) + (blue ? 4 : 0));
}
