/*	$NetBSD: msg_287.c,v 1.5 2026/03/15 08:16:53 rillig Exp $	*/
# 3 "msg_287.c"

// Test for message: function declaration for '%s' is not a prototype [287]

/* lint1-extra-flags: -h -X 351 */

/* expect+1: warning: function declaration for 'no_prototype_declaration' is not a prototype [287] */
void no_prototype_declaration();
void prototype_declaration(void);

/* expect+1: warning: function declaration for 'no_prototype_typedef' is not a prototype [287] */
typedef void (no_prototype_typedef)();
typedef void (prototype_typedef)(void);

/* expect+1: warning: function declaration for '<unnamed>' is not a prototype [287] */
int no_prototype_sizeof[sizeof(void (*)())];
int prototype_sizeof[sizeof(void (*)(void))];
