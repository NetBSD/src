/*	$NetBSD: decl_direct_abstract.c,v 1.10 2023/08/02 21:11:35 rillig Exp $	*/
# 3 "decl_direct_abstract.c"

/*
 * Test parsing of direct-abstract-declarator (C99 6.7.6), which are a tricky
 * part of the C standard since they require lookahead and are so complicated
 * that GCC's parser dedicates 34 lines of comments to this topic.
 *
 * See msg_155.c.
 */

/* lint1-extra-flags: -X 351 */

struct incompatible {
	int member;
} x;

void
c99_6_7_6_examples(void)
{
	/* expect+1: ... 'int' ... */
	x = (int)x;
	/* expect+1: ... 'pointer to int' ... */
	x = (int *)x;
	/* expect+1: ... 'array[3] of pointer to int' ... */
	x = (int *[3])x;
	/* expect+1: ... 'pointer to array[3] of int' ... */
	x = (int (*)[3])x;
	/* expect+1: ... 'pointer to array[unknown_size] of int' ... */
	x = (int (*)[*])x;
	/* expect+1: ... 'function() returning pointer to int' ... */
	x = (int *())x;
	/* expect+1: ... 'pointer to function(void) returning int' ... */
	x = (int (*)(void))x;
	/* expect+1: ... 'array[unknown_size] of const pointer to function(unsigned int, ...) returning int' ... */
	x = (int (*const[])(unsigned int, ...))x;
}

void
function_returning_char(void)
{
	// GCC adds a pointer, then says 'char (*)(short int (*)(long int))'.
	// Clang says 'char (short (*)(long))'.
	/* cdecl says 'function (pointer to function (long) returning short) returning char' */
	/* expect+1: ... 'function(pointer to function(long) returning short) returning char' ... */
	x = (char(short (*)(long)))x;

	/* expect+1: warning: nested 'extern' declaration of 'f1' [352] */
	char f1(short (*)(long));

	/* expect+1: ... 'pointer to function(pointer to function(long) returning short) returning char' ... */
	x = f1;
}

void
function_returning_pointer(void)
{
	// GCC says 'error: cast specifies function type'.
	// Clang says 'char (short *(*)(long))'.
	/* expect+1: error: invalid cast from 'struct incompatible' to 'function(pointer to function(long) returning pointer to short) returning char' [147] */
	x = (char(short *(long)))x;

	/* expect+1: warning: nested 'extern' declaration of 'f2' [352] */
	char f2(short *(long));

	// GCC adds two pointers, saying 'char (*)(short int * (*)(long int))'.
	// Clang says 'char (short *(*)(long))' */
	/* cdecl says 'syntax error' */
	/* expect+1: ... 'pointer to function(pointer to function(long) returning pointer to short) returning char' ... */
	x = f2;
}


void int_array(int[]);
void int_array_3(int[3]);
/* supported since cgram.y 1.363 from 2021-09-14 */
void int_array_ast(int[*]);
/* expect+1: error: null dimension [17] */
void int_array_7_array(int[7][]);
void int_array_7_array_3(int[7][3]);
/* expect+1: error: null dimension [17] */
void int_array_7_array_ast(int[7][*]);

void int_array_array(int[][7]);
void int_array_3_array(int[3][7]);
/* supported since cgram.y 1.363 from 2021-09-14 */
void int_array_ast_array(int[*][7]);

/* expect+1: error: cannot take size/alignment of function type 'function() returning int' [144] */
unsigned long size_unspecified_args = sizeof(int());
/* expect+1: error: cannot take size/alignment of function type 'function(void) returning int' [144] */
unsigned long size_prototype_void = sizeof(int(void));
/* expect+1: error: cannot take size/alignment of function type 'function(double) returning int' [144] */
unsigned long size_prototype_unnamed = sizeof(int(double));
/* expect+1: error: cannot take size/alignment of function type 'function(double) returning int' [144] */
unsigned long size_prototype_named = sizeof(int(double dbl));

/* expect+2: error: cannot take size/alignment of function type 'function() returning int' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_unspecified_args_return_int[-1000 - (int)sizeof(int())];

/* expect+2: error: cannot take size/alignment of function type 'function() returning char' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_unspecified_args_return_char[-1000 - (int)sizeof(char())];

/* expect+2: error: cannot take size/alignment of function type 'function(void) returning int' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_void_return_int[-1000 - (int)sizeof(int(void))];

/* expect+2: error: cannot take size/alignment of function type 'function(void) returning double' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_void_return_double[-1000 - (int)sizeof(double(void))];

/* expect+2: error: cannot take size/alignment of function type 'function(double) returning int' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_unnamed_return_int[-1000 - (int)sizeof(int(double))];

/* expect+2: error: cannot take size/alignment of function type 'function(double) returning pointer to char' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_unnamed_return_pchar[-1000 - (int)sizeof(char *(double))];

/* expect+2: error: cannot take size/alignment of function type 'function(double) returning int' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_named_return_int[-1000 - (int)sizeof(int(double dbl))];

/* expect+2: error: cannot take size/alignment of function type 'function(double) returning pointer to pointer to pointer to char' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_named_return_pppchar[-1000 - (int)sizeof(char ***(double dbl))];


typedef struct {
	char a[1];
} a01;

typedef struct {
	char a[4];
} a04;

typedef struct {
	char a[8];
} a08;

typedef struct {
	char a[32];
} a32;

/* expect+2: error: cannot take size/alignment of function type 'function() returning struct typedef a01' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int unspecified_args_return_01[-1000 - (int)sizeof(a01())];
/* expect+2: error: cannot take size/alignment of function type 'function() returning struct typedef a04' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int unspecified_args_return_04[-1000 - (int)sizeof(a04())];
/* expect+2: error: cannot take size/alignment of function type 'function() returning struct typedef a08' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int unspecified_args_return_08[-1000 - (int)sizeof(a08())];
/* expect+2: error: cannot take size/alignment of function type 'function() returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int unspecified_args_return_32[-1000 - (int)sizeof(a32())];

/* expect+2: error: cannot take size/alignment of function type 'function(void) returning struct typedef a01' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_01[-1000 - (int)sizeof(a01(void))];
/* expect+2: error: cannot take size/alignment of function type 'function(void) returning struct typedef a04' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_04[-1000 - (int)sizeof(a04(void))];
/* expect+2: error: cannot take size/alignment of function type 'function(void) returning struct typedef a08' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_08[-1000 - (int)sizeof(a08(void))];
/* expect+2: error: cannot take size/alignment of function type 'function(void) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_32[-1000 - (int)sizeof(a32(void))];

/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a01) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_01_return_32[-1000 - (int)sizeof(a32(a01))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a04) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_04_return_32[-1000 - (int)sizeof(a32(a04))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a08) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_08_return_32[-1000 - (int)sizeof(a32(a08))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_32_return_32[-1000 - (int)sizeof(a32(a32))];

/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a01' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_32_return_01[-1000 - (int)sizeof(a01(a32))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a04' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_32_return_04[-1000 - (int)sizeof(a04(a32))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a08' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_32_return_08[-1000 - (int)sizeof(a08(a32))];

/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a01) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_01_return_32[-1000 - (int)sizeof(a32(a01 arg))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a04) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_04_return_32[-1000 - (int)sizeof(a32(a04 arg))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a08) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_08_return_32[-1000 - (int)sizeof(a32(a08 arg))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_32_return_32[-1000 - (int)sizeof(a32(a32 arg))];

/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a01' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_32_return_01[-1000 - (int)sizeof(a01(a32 arg))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a04' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_32_return_04[-1000 - (int)sizeof(a04(a32 arg))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a08' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_32_return_08[-1000 - (int)sizeof(a08(a32 arg))];
