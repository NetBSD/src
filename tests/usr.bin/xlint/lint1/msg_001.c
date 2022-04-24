/*	$NetBSD: msg_001.c,v 1.6 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "msg_001.c"

// Test for message: old style declaration; add 'int' [1]

/* expect+1: warning: old style declaration; add 'int' [1] */
old_style = 1;

int new_style = 1;

/* expect+2: error: old style declaration; add 'int' [1] */
/* expect+1: warning: static variable static_old_style unused [226] */
static static_old_style = 1;

/* expect+1: warning: static variable static_new_style unused [226] */
static int static_new_style = 1;

/* expect+2: error: old style declaration; add 'int' [1] */
extern_implicit_int(void)
{
}

/* expect+4: error: old style declaration; add 'int' [1] */
/* expect+2: warning: static function static_implicit_int unused [236] */
static
static_implicit_int(void)
{
}
