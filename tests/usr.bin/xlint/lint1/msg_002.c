/*	$NetBSD: msg_002.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_002.c"

// Test for message: empty declaration [2]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: empty declaration [2] */
int;

int local_variable;

/* expect+1: warning: empty declaration [2] */
const;

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
