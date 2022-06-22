/*	$NetBSD: msg_346.c,v 1.7 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_346.c"

// Test for message: call to '%s' effectively discards 'const' from argument [346]

typedef unsigned long size_t;

void* memchr(const void *, int, size_t);		/* C99 7.21.5.1 */
char *strchr(const char *, int);			/* C99 7.21.5.2 */
char* strpbrk(const char *, const char *);		/* C99 7.21.5.4 */
char* strrchr(const char *, int);			/* C99 7.21.5.5 */
char* strstr(const char *, const char *);		/* C99 7.21.5.7 */

void take_const_char_ptr(const char *);
void take_char_ptr(char *);

void
example(void)
{
	const char *ccp = "const char *";
	char *cp = "char *";

	ccp = strchr(ccp, 'c');
	ccp = strchr(cp, 'c');
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	cp = strchr(ccp, 'c');
	cp = strchr(cp, 'c');

	take_const_char_ptr(strchr(ccp, 'c'));
	take_const_char_ptr(strchr(cp, 'c'));
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	take_char_ptr(strchr(ccp, 'c'));
	take_char_ptr(strchr(cp, 'c'));

	take_const_char_ptr(strchr("literal", 'c'));
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	take_char_ptr(strchr("literal", 'c'));
}

void
all_functions(void)
{
	/* expect+1: warning: call to 'memchr' effectively discards 'const' from argument [346] */
	take_char_ptr(memchr("string", 'c', 7));
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	take_char_ptr(strchr("string", 'c'));
	/* expect+1: warning: call to 'strpbrk' effectively discards 'const' from argument [346] */
	take_char_ptr(strpbrk("string", "c"));
	/* expect+1: warning: call to 'strrchr' effectively discards 'const' from argument [346] */
	take_char_ptr(strrchr("string", 'c'));
	/* expect+1: warning: call to 'strstr' effectively discards 'const' from argument [346] */
	take_char_ptr(strstr("string", "c"));
}

void
edge_cases(void)
{	/* TODO: '0 argument passed' is not correct English */
	/* No arguments, to cover the 'an == NULL' in is_first_arg_const. */
	/* expect+1: error: argument mismatch: 0 argument passed, 2 expected [150] */
	take_char_ptr(strchr());
}

/*
 * Bsearch is another standard function that effectively discards the 'const'
 * modifier, but from the second argument, not the first.
 */

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
	int (*compar) (const void *, const void *));

int cmp(const void *, const void *);

void take_void_ptr(void *);
void take_const_void_ptr(const void *);

void
bsearch_example(void)
{
	int arr[] = { 1 };
	const int const_arr[] = { 1 };

	take_const_void_ptr(bsearch("", const_arr, 4, 1, cmp));
	take_const_void_ptr(bsearch("", arr, 4, 1, cmp));
	take_void_ptr(bsearch("", arr, 4, 1, cmp));

	/* expect+1: warning: call to 'bsearch' effectively discards 'const' from argument [346] */
	take_void_ptr(bsearch("", const_arr, 4, 1, cmp));
}
