/*	$NetBSD: d_gcc_compound_statements2.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_gcc_compound_statements2.c"

/* GCC compound statements with non-expressions */
struct cpu_info {
	int bar;
};

int
main(void)
{
	return ({
	    struct cpu_info *__ci;
	    __asm__ volatile("movl %%fs:4,%0":"=r" (__ci));
	    __ci;
	})->bar;
}
