/*	$NetBSD: msg_287.c,v 1.4 2025/03/10 22:35:02 rillig Exp $	*/
# 3 "msg_287.c"

// Test for message: function declaration is not a prototype [287]

/* lint1-extra-flags: -h -X 351 */

/* expect+1: warning: function declaration is not a prototype [287] */
void no_prototype_declaration();
void prototype_declaration(void);

/* expect+1: warning: function declaration is not a prototype [287] */
typedef void (no_prototype_typedef)();
typedef void (prototype_typedef)(void);

/* expect+1: warning: function declaration is not a prototype [287] */
int no_prototype_sizeof[sizeof(void (*)())];
int prototype_sizeof[sizeof(void (*)(void))];
