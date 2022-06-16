/*	$NetBSD: msg_244.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_244.c"

// Test for message: illegal structure pointer combination [244]

struct a {
	int member;
};

struct b {
	int member;
};

int
diff(struct a *a, struct b *b)
{
	/* expect+1: error: illegal pointer subtraction [116] */
	return a - b;
}

_Bool
lt(struct a *a, struct b *b)
{
	/* expect+1: warning: incompatible structure pointers: 'pointer to struct a' '<' 'pointer to struct b' [245] */
	return a < b;
}

struct a *
ret(struct b *b)
{
	/* expect+1: warning: illegal structure pointer combination [244] */
	return b;
}
