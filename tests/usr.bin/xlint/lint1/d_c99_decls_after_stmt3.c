/*	$NetBSD: d_c99_decls_after_stmt3.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_decls_after_stmt3.c"

void sample(int i)
{
  i += 1;
  int j = 0; j += 1;
}
