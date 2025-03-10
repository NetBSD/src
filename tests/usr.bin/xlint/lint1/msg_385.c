/*	$NetBSD: msg_385.c,v 1.2 2025/03/10 22:08:36 rillig Exp $	*/
# 3 "msg_385.c"

// Test for message: do-while macro '%.*s' ends with semicolon [385]

/*
 * A function-like macro that consists of a do-while statement is intended to
 * expand to a single statement, but without the trailing semicolon, as the
 * semicolon is already provided by the calling site. When the macro expansion
 * ends with a semicolon, there are two semicolons, which can lead to syntax
 * errors.
 */

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: do-while macro 'wrong_stmt' ends with semicolon [385] */
#define		wrong_stmt()	do { } while (0);

#define		correct_stmt()	do { } while (0)

/* expect+5: warning: do-while macro 'wrong_stmt_with_comment' ends with semicolon [385] */
#define wrong_stmt_with_comment() do { } while (0); /*
a
b
c
*/

#define correct_stmt_with_comment() do { } while (0) /*
a
b
c
*/

/* The comment marker inside the string literal does not start a comment. */
#define stmt_with_string() do { print("/*"); } while (0)

void
call_wrong_stmt(int x)
{
	if (x > 0)
		do { } while (0);;
	/* expect+1: error: syntax error 'else' [249] */
	else
		do { } while (0);;
}

void
call_correct_stmt(int x)
{
	if (x < 0)
		do { } while (0);
	else
		do { } while (0);
}

// The macro expansion does start with "do", but not with the keyword "do",
// so don't warn in this case.
#define unrelated() do_something();
