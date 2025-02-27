/*	$NetBSD: msg_196.c,v 1.6 2025/02/27 06:48:29 rillig Exp $	*/
# 3 "msg_196.c"

// Test for message: case label is converted from '%s' to '%s' [196]

/* lint1-extra-flags: -X 351 */

// C23 6.8.5.3p5 says: [...] The constant expression in each case label is
// converted to the promoted type of the controlling expression. [...]

void
switch_int_unsigned(int x)
{
	switch (x) {
		/* expect+1: warning: case label is converted from 'unsigned int' to 'int' [196] */
	case (unsigned int)-1:
		/* expect+1: warning: case label is converted from 'unsigned int' to 'int' [196] */
	case -2U:
		/* expect+1: warning: case label is converted from 'unsigned long long' to 'int' [196] */
	case 0x1000200030004000ULL:
		return;
	}
}
