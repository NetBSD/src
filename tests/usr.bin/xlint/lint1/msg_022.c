/*	$NetBSD: msg_022.c,v 1.8 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_022.c"

// Test for message: incomplete or misplaced function definition [22]

/* lint1-extra-flags: -X 351 */

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

/* expect+3: warning: function definition for 'old_style' with identifier list is obsolete in C23 [384] */
/* expect+2: error: incomplete or misplaced function definition [22] */
/* expect+1: warning: old-style declaration; add 'int' [1] */
old_style(arg);
