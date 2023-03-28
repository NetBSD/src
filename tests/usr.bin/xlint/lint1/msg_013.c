/*	$NetBSD: msg_013.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_013.c"

// Test for message: incomplete enum type '%s' [13]

/* lint1-extra-flags: -X 351 */

enum tag;

/* XXX: why '<unnamed>'? */
/* expect+1: warning: incomplete enum type '<unnamed>' [13] */
void function(enum tag);

enum tag {
	CONSTANT
};
