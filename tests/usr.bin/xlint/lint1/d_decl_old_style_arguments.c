# 2 "d_decl_old_style_arguments.c"

/*
 * A function is declared with a prototype, followed by an old-style definition
 * that is completely different.
 */

/* lint1-extra-flags: -X 351 */

void func(int a, int b, int c);

/* expect+4: warning: parameter 'num' unused in function 'func' [231] */
/* expect+3: warning: parameter 'ptr' unused in function 'func' [231] */
/* expect+2: warning: parameter 'dbl' unused in function 'func' [231] */
/* expect+1: warning: parameter 'def' unused in function 'func' [231] */
void func(num, ptr, dbl, def)
    int num;
    char *ptr;
    double dbl;
{
	/* expect-1: warning: type of argument 'def' defaults to 'int' [32] */
	/* expect-2: error: parameter mismatch: 3 declared, 4 defined [51] */
}
