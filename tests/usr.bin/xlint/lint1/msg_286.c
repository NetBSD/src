/*	$NetBSD: msg_286.c,v 1.5 2026/03/15 08:16:53 rillig Exp $	*/
# 3 "msg_286.c"

// Test for message: function definition for '%s' is not a prototype [286]

/* lint1-extra-flags: -h -X 351 */

/* expect+1: warning: function declaration for 'no_prototype_declaration' is not a prototype [287] */
void no_prototype_declaration();
void prototype_declaration(void);

void
no_prototype_definition()
/* expect+1: warning: function definition for 'no_prototype_definition' is not a prototype [286] */
{
}

void
prototype_definition(void)
{
}

int
main()
/* expect+1: warning: function definition for 'main' is not a prototype [286] */
{
}
