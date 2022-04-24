/*	$NetBSD: msg_072.c,v 1.6 2022/04/24 19:21:01 rillig Exp $	*/
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

	/* expect+1: error: syntax error 'missing base type for typedef' [249] */
	typedef not_a_type;

	/* expect+1: error: old style declaration; add 'int' [1] */
	static missing_type;
}
