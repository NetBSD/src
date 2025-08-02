/*	$NetBSD: msg_286.c,v 1.3.4.1 2025/08/02 05:58:18 perseant Exp $	*/
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
