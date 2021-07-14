/*	$NetBSD: msg_072.c,v 1.5 2021/07/14 20:39:13 rillig Exp $	*/
# 3 "msg_072.c"

// Test for message: typedef declares no type name [72]

/* expect+1: warning: typedef declares no type name [72] */
typedef int;

typedef int number;

/* expect+1: warning: typedef declares no type name [72] */
const typedef;

void
cover_cgram_declaration(void)
{

	/* expect+1: warning: typedef declares no type name [72] */
	typedef const;

	/* expect+1: warning: empty declaration [2] */
	const;

	/* expect+1: warning: typedef declares no type name [72] */
	typedef int;

	/* expect+1: warning: empty declaration [2] */
	int;
}
