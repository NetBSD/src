/*	$NetBSD: d_gcc_compound_statements2.c,v 1.4 2021/09/10 20:02:51 rillig Exp $	*/
# 3 "d_gcc_compound_statements2.c"

/* GCC compound statements with non-expressions */
struct cpu_info {
	int bar;
};

int
compound_expression_with_decl_and_stmt(void)
{
	return ({
	    struct cpu_info *ci;
	    __asm__ volatile("movl %%fs:4,%0":"=r" (ci));
	    ci;
	})->bar;
}

int
compound_expression_with_only_stmt(void)
{
	struct cpu_info ci = { 0 };
	return ({
		if (ci.bar > 0)
			ci.bar++;
		ci;
	}).bar;
}
