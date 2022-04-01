/*	$NetBSD: decl_direct_abstract.c,v 1.6 2022/04/01 23:16:32 rillig Exp $	*/
# 3 "decl_direct_abstract.c"

/*
 * Test parsing of direct-abstract-declarator (C99 6.7.6), which are a tricky
 * part of the C standard since they require lookahead and are so complicated
 * that GCC's parser dedicates 34 lines of comments to this topic.
 *
 * See msg_155.c.
 */

/*
 * The following tests do not use int, to avoid confusion with the implicit
 * return type.
 */

char func0001(short (*)(long));

/* GCC says 'char (*)(short int (*)(long int))' */
/* Clang says 'char (short (*)(long))' */
/* cdecl says 'function (pointer to function (long) returning short) returning char' */
/* expect+1: 'pointer to function(pointer to function(long) returning short) returning char' */
double type_of_func0001 = func0001;

char func0002(short *(long));

/* GCC says 'char (*)(short int * (*)(long int))' */
/* Clang says 'char (short *(*)(long))' */
/* cdecl says 'syntax error' */
/* FIXME: lint is wrong, it discards the 'short *' */
/* expect+1: 'pointer to function(long) returning char' */
double type_of_func0002 = func0002;

void c99_6_7_6_example_a(int);
void c99_6_7_6_example_b(int *);
void c99_6_7_6_example_c(int *[3]);
void c99_6_7_6_example_d(int (*)[3]);
void c99_6_7_6_example_e(int (*)[*]);
void c99_6_7_6_example_f(int *());
void c99_6_7_6_example_g(int (*)(void));
void c99_6_7_6_example_h(int (*const[])(unsigned int, ...));

struct incompatible {
	int member;
} x;

/* expect+1: 'pointer to function(int) returning void' */
double type_of_c99_6_7_6_example_a = c99_6_7_6_example_a;
/* expect+1: 'pointer to function(pointer to int) returning void' */
double type_of_c99_6_7_6_example_b = c99_6_7_6_example_b;
/* expect+1: 'pointer to function(pointer to pointer to int) returning void' */
double type_of_c99_6_7_6_example_c = c99_6_7_6_example_c;
/* expect+1: 'pointer to function(pointer to array[3] of int) returning void' */
double type_of_c99_6_7_6_example_d = c99_6_7_6_example_d;
/* expect+1: 'pointer to function(pointer to array[unknown_size] of int) returning void' */
double type_of_c99_6_7_6_example_e = c99_6_7_6_example_e;
/* Wrong type before decl.c 1.256 from 2022-04-01. */
/* expect+1: 'pointer to function(pointer to function() returning pointer to int) returning void' */
double type_of_c99_6_7_6_example_f = c99_6_7_6_example_f;
/* expect+1: 'pointer to function(pointer to function(void) returning int) returning void' */
double type_of_c99_6_7_6_example_g = c99_6_7_6_example_g;
/* expect+1: 'pointer to function(pointer to const pointer to function(unsigned int, ...) returning int) returning void' */
double type_of_c99_6_7_6_example_h = c99_6_7_6_example_h;

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
/* FIXME: Must be 'of function', not 'of void'. */
/* expect+1: error: cannot take size/alignment of void [146] */
unsigned long size_prototype_void = sizeof(int(void));
/* TODO: error: cannot take size/alignment of function type 'function(double) returning int' [144] */
unsigned long size_prototype_unnamed = sizeof(int(double));
/* TODO: error: cannot take size/alignment of function type 'function(double) returning int' [144] */
unsigned long size_prototype_named = sizeof(int(double dbl));

/* expect+2: error: cannot take size/alignment of function type 'function() returning int' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_unspecified_args_return_int[-1000 - (int)sizeof(int())];

/* expect+2: error: cannot take size/alignment of function type 'function() returning char' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_unspecified_args_return_char[-1000 - (int)sizeof(char())];

/* FIXME: 'of void' must be 'of function'. */
/* expect+2: error: cannot take size/alignment of void [146] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_void_return_int[-1000 - (int)sizeof(int(void))];

/* FIXME: 'of void' must be 'of function'. */
/* expect+2: error: cannot take size/alignment of void [146] */
/* expect+1: error: negative array dimension (-1000) [20] */
int size_prototype_void_return_double[-1000 - (int)sizeof(double(void))];

/* expect+1: error: negative array dimension (-1008) [20] */
int size_prototype_unnamed_return_int[-1000 - (int)sizeof(int(double))];

/* expect+1: error: negative array dimension (-1008) [20] */
int size_prototype_unnamed_return_pchar[-1000 - (int)sizeof(char *(double))];

/* expect+1: error: negative array dimension (-1008) [20] */
int size_prototype_named_return_int[-1000 - (int)sizeof(int(double dbl))];

/* expect+1: error: negative array dimension (-1008) [20] */
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

/* expect+2: error: cannot take size/alignment of void [146] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_01[-1000 - (int)sizeof(a01(void))];
/* expect+2: error: cannot take size/alignment of void [146] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_04[-1000 - (int)sizeof(a04(void))];
/* expect+2: error: cannot take size/alignment of void [146] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_08[-1000 - (int)sizeof(a08(void))];
/* expect+2: error: cannot take size/alignment of void [146] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_void_return_32[-1000 - (int)sizeof(a32(void))];

/* expect+1: error: negative array dimension (-1001) [20] */
int prototype_unnamed_01_return_32[-1000 - (int)sizeof(a32(a01))];
/* expect+1: error: negative array dimension (-1004) [20] */
int prototype_unnamed_04_return_32[-1000 - (int)sizeof(a32(a04))];
/* expect+1: error: negative array dimension (-1008) [20] */
int prototype_unnamed_08_return_32[-1000 - (int)sizeof(a32(a08))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_unnamed_32_return_32[-1000 - (int)sizeof(a32(a32))];

/* expect+1: error: negative array dimension (-1032) [20] */
int prototype_unnamed_32_return_01[-1000 - (int)sizeof(a01(a32))];
/* expect+1: error: negative array dimension (-1032) [20] */
int prototype_unnamed_32_return_04[-1000 - (int)sizeof(a04(a32))];
/* expect+1: error: negative array dimension (-1032) [20] */
int prototype_unnamed_32_return_08[-1000 - (int)sizeof(a08(a32))];

/* expect+1: error: negative array dimension (-1001) [20] */
int prototype_named_01_return_32[-1000 - (int)sizeof(a32(a01 arg))];
/* expect+1: error: negative array dimension (-1004) [20] */
int prototype_named_04_return_32[-1000 - (int)sizeof(a32(a04 arg))];
/* expect+1: error: negative array dimension (-1008) [20] */
int prototype_named_08_return_32[-1000 - (int)sizeof(a32(a08 arg))];
/* expect+2: error: cannot take size/alignment of function type 'function(struct typedef a32) returning struct typedef a32' [144] */
/* expect+1: error: negative array dimension (-1000) [20] */
int prototype_named_32_return_32[-1000 - (int)sizeof(a32(a32 arg))];

/* expect+1: error: negative array dimension (-1032) [20] */
int prototype_named_32_return_01[-1000 - (int)sizeof(a01(a32 arg))];
/* expect+1: error: negative array dimension (-1032) [20] */
int prototype_named_32_return_04[-1000 - (int)sizeof(a04(a32 arg))];
/* expect+1: error: negative array dimension (-1032) [20] */
int prototype_named_32_return_08[-1000 - (int)sizeof(a08(a32 arg))];
