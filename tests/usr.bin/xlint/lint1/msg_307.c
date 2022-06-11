/*	$NetBSD: msg_307.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_307.c"

// Test for message: static variable '%s' set but not used [307]

/* expect+1: warning: static variable 'set_but_not_used' set but not used [307] */
static int set_but_not_used;

static int only_incremented;

void function(void)
{
	set_but_not_used = 3;
	only_incremented++;
}
