# 2 "d_decl_old_style_arguments.c"

/*
 * A function is declared with a prototype, followed by an old style definition
 * that is completely different.
 */

void func(int a, int b, int c);

void func(num, ptr, dbl, def)	/* expect: 231, 231, 231, 231 */
    int num;
    char *ptr;
    double dbl;
{				/* expect: 32, 51 */
}
