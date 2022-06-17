/*	$NetBSD: msg_341.c,v 1.2 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_341.c"

// Test for message: argument to '%s' must be 'unsigned char' or EOF, not '%s' [341]

/*
 * Ensure that the functions from <ctype.h> are called with the correct
 * argument.
 */

/* NetBSD 9.99.81, <ctype.h> */
extern const unsigned short *_ctype_tab_;
extern const short *_tolower_tab_;
extern const short *_toupper_tab_;
int isspace(int);

void sink(int);

void
function_call_char(char c)
{

	/* expect+1: warning: argument to 'isspace' must be 'unsigned char' or EOF, not 'char' [341] */
	(isspace)(c);

	/* This is the only allowed form. */
	isspace((unsigned char)c);

	/* The cast to 'int' is redundant, it doesn't hurt though. */
	isspace((int)(unsigned char)c);

	/* expect+1: warning: argument to 'isspace' must be cast to 'unsigned char', not to 'int' [342] */
	isspace((int)c);

	/* expect+1: warning: argument to 'isspace' must be cast to 'unsigned char', not to 'unsigned int' [342] */
	isspace((unsigned int)c);
}

/*
 * If the expression starts with type 'unsigned char', it can be cast to any
 * other type.  Chances are low enough that the cast is to 'char', which would
 * be the only bad type.
 */
void
function_call_unsigned_char(unsigned char c)
{

	(isspace)(c);
	isspace((unsigned char)c);
	isspace((int)c);
	isspace((unsigned int)c);
}

/* When used in a loop of fgetc, the type is already 'int'.  That's fine. */
void
function_call_int(int c)
{

	isspace(c);
}

void
macro_invocation_NetBSD(char c)
{

	/* expect+1: warning: argument to 'function from <ctype.h>' must be 'unsigned char' or EOF, not 'char' [341] */
	sink(((int)((_ctype_tab_ + 1)[(c)] & 0x0040)));

	/* This is the only allowed form. */
	sink(((int)((_ctype_tab_ + 1)[((unsigned char)c)] & 0x0040)));

	/* expect+1: warning: argument to 'function from <ctype.h>' must be cast to 'unsigned char', not to 'int' [342] */
	sink(((int)((_ctype_tab_ + 1)[((int)c)] & 0x0040)));

	/* expect+1: warning: argument to 'function from <ctype.h>' must be cast to 'unsigned char', not to 'unsigned int' [342] */
	sink(((int)((_ctype_tab_ + 1)[((unsigned int)c)] & 0x0040)));
}
