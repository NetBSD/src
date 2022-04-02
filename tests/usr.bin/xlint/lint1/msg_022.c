/*	$NetBSD: msg_022.c,v 1.3 2022/04/02 20:12:46 rillig Exp $	*/
# 3 "msg_022.c"

// Test for message: incomplete or misplaced function definition [22]

/*
 * Before decl.c 1.264 and func.c 1.130 from 2022-04-02, lint ran into
 * assertion failures after trying to recover from the below syntax error.
 */
/* expect+1: error: syntax error 'f' [249] */
unsigned long asdf = sizeof(int f() {});

/* Give the parser a chance to recover. */
/* expect+1: warning: empty declaration [0] */
;

/*
 * Before decl.c 1.264 and func.c 1.130 from 2022-04-02, lint ran into
 * assertion failures after trying to recover from the below syntax error.
 */
/* expect+1: error: syntax error 'param1' [249] */
unsigned long sz = sizeof(int(param1, param2));

/* Give the parser a chance to recover. */
/* expect+1: warning: empty declaration [0] */
;

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
