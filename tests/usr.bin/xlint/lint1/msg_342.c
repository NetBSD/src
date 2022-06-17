/*	$NetBSD: msg_342.c,v 1.4 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_342.c"

// Test for message: argument to '%s' must be cast to 'unsigned char', not to '%s' [342]

/*
 * Ensure that the functions from <ctype.h> are called with the correct
 * argument.
 */

/* NetBSD 9.99.81, <ctype.h> */
extern const unsigned short *_ctype_tab_;
extern const short *_tolower_tab_;
extern const short *_toupper_tab_;
int isalnum(int);
int isalpha(int);
int isblank(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int tolower(int);
int toupper(int);

int is_other(int);
int to_other(int);

void sink(int);

void
cover_is_ctype_function(char c)
{
	/* expect+1: warning: argument to 'isalnum' must be 'unsigned char' or EOF, not 'char' [341] */
	isalnum(c);
	/* expect+1: warning: argument to 'isalpha' must be 'unsigned char' or EOF, not 'char' [341] */
	isalpha(c);
	/* expect+1: warning: argument to 'isblank' must be 'unsigned char' or EOF, not 'char' [341] */
	isblank(c);
	/* expect+1: warning: argument to 'iscntrl' must be 'unsigned char' or EOF, not 'char' [341] */
	iscntrl(c);
	/* expect+1: warning: argument to 'isdigit' must be 'unsigned char' or EOF, not 'char' [341] */
	isdigit(c);
	/* expect+1: warning: argument to 'isgraph' must be 'unsigned char' or EOF, not 'char' [341] */
	isgraph(c);
	/* expect+1: warning: argument to 'islower' must be 'unsigned char' or EOF, not 'char' [341] */
	islower(c);
	/* expect+1: warning: argument to 'isprint' must be 'unsigned char' or EOF, not 'char' [341] */
	isprint(c);
	/* expect+1: warning: argument to 'ispunct' must be 'unsigned char' or EOF, not 'char' [341] */
	ispunct(c);
	/* expect+1: warning: argument to 'isspace' must be 'unsigned char' or EOF, not 'char' [341] */
	isspace(c);
	/* expect+1: warning: argument to 'isupper' must be 'unsigned char' or EOF, not 'char' [341] */
	isupper(c);
	/* expect+1: warning: argument to 'isxdigit' must be 'unsigned char' or EOF, not 'char' [341] */
	isxdigit(c);
	/* expect+1: warning: argument to 'tolower' must be 'unsigned char' or EOF, not 'char' [341] */
	tolower(c);
	/* expect+1: warning: argument to 'toupper' must be 'unsigned char' or EOF, not 'char' [341] */
	toupper(c);

	/* Functions with similar names are not checked. */
	is_other(c);
	to_other(c);
}

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
