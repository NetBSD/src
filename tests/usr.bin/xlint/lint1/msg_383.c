/*	$NetBSD: msg_383.c,v 1.4 2025/05/04 09:40:03 rillig Exp $	*/
# 3 "msg_383.c"

// Test for message: passing '%s' as argument %d to '%s' discards '%s' [383]

/* lint1-extra-flags: -X 351 */

void sink_char(char *, const char *, volatile char *, const volatile char *);
void sink_int(int *, const int *, volatile int *, const volatile int *);

void (*indirect_char)(char *, const char *, volatile char *, const volatile char *);

struct {
	void (*member_char)(char *, const char *, volatile char *, const volatile char *);
} doubly_indirect;

void
caller(const volatile char *cvcp, const volatile int *cvip, int (*fn)(void))
{
	/* expect+3: warning: passing 'pointer to const volatile char' as argument 1 to 'sink_char' discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile char' as argument 2 to 'sink_char' discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile char' as argument 3 to 'sink_char' discards 'const' [383] */
	sink_char(cvcp, cvcp, cvcp, cvcp);
	/* expect+3: warning: passing 'pointer to const volatile int' as argument 1 to 'sink_int' discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile int' as argument 2 to 'sink_int' discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile int' as argument 3 to 'sink_int' discards 'const' [383] */
	sink_int(cvip, cvip, cvip, cvip);
	/* expect+4: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to char' for argument 1 [153] */
	/* expect+3: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to const char' for argument 2 [153] */
	/* expect+2: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to volatile char' for argument 3 [153] */
	/* expect+1: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to const volatile char' for argument 4 [153] */
	sink_char(fn, fn, fn, fn);

	/* expect+3: warning: passing 'pointer to const volatile char' as argument 1 to 'indirect_char' discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile char' as argument 2 to 'indirect_char' discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile char' as argument 3 to 'indirect_char' discards 'const' [383] */
	indirect_char(cvcp, cvcp, cvcp, cvcp);

	/* expect+3: warning: passing 'pointer to const volatile char' as argument 1 to 'function(pointer to char, pointer to const char, pointer to volatile char, pointer to const volatile char) returning void' discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile char' as argument 2 to 'function(pointer to char, pointer to const char, pointer to volatile char, pointer to const volatile char) returning void' discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile char' as argument 3 to 'function(pointer to char, pointer to const char, pointer to volatile char, pointer to const volatile char) returning void' discards 'const' [383] */
	doubly_indirect.member_char(cvcp, cvcp, cvcp, cvcp);
}


typedef int array[8];
typedef const int *pointer_to_const_array;

// The 'const' applies to the pointer target, making it 'const int *'.
int const_array_callee(const array);

static inline int
const_array_caller(pointer_to_const_array ptr)
{
	return const_array_callee(ptr);
}
