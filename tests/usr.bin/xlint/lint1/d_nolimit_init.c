/*	$NetBSD: d_nolimit_init.c,v 1.3 2021/03/27 13:59:18 rillig Exp $	*/
# 3 "d_nolimit_init.c"

/*
 * no limit initializers, or as C99 calls it, initialization of an array of
 * unknown size
 */
char weekday_names[][4] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
