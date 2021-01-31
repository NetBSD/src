/*	$NetBSD: d_gcc_compound_statements3.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_gcc_compound_statements3.c"

/* GCC compound statements with void type */

void
main(void)
{
	({
		void *v;
		__asm__ volatile("noop");
	});
}
