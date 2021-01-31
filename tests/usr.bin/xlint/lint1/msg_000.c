/*	$NetBSD: msg_000.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_000.c"

// Test for message: empty declaration [0]

extern int extern_declared;

;				/* expect: 0 */

static int local_defined;	/* expect: 226 */
