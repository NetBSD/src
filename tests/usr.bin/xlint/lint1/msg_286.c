/*	$NetBSD: msg_286.c,v 1.4 2025/03/10 22:35:02 rillig Exp $	*/
# 3 "msg_286.c"

// Test for message: function definition is not a prototype [286]

/* lint1-extra-flags: -h -X 351 */

/* expect+1: warning: function declaration is not a prototype [287] */
void no_prototype_declaration();
void prototype_declaration(void);

void
no_prototype_definition()
/* expect+1: warning: function definition is not a prototype [286] */
{
}

void
prototype_definition(void)
{
}

int
main()
/* expect+1: warning: function definition is not a prototype [286] */
{
}
