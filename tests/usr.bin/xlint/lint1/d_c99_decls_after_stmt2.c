/*	$NetBSD: d_c99_decls_after_stmt2.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_decls_after_stmt2.c"

typedef int int_t;
int main(void) {
int i = 0; i += 1;
int_t j = 0; j += 1;
return 0;
}
