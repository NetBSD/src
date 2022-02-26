/*	$NetBSD: d_gcc_compound_statements2.c,v 1.5 2022/02/26 20:36:11 rillig Exp $	*/
# 3 "d_gcc_compound_statements2.c"

/*
 * GCC statement expressions with non-expressions.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
 */

struct cpu_info {
	int bar;
};

int
statement_expr_with_decl_and_stmt(void)
{
	return ({
	    struct cpu_info *ci;
	    __asm__ volatile("movl %%fs:4,%0":"=r" (ci));
	    ci;
	})->bar;
}

int
statement_expr_with_only_stmt(void)
{
	struct cpu_info ci = { 0 };
	return ({
		if (ci.bar > 0)
			ci.bar++;
		ci;
	}).bar;
}

/*
 * Since main1.c 1.58 from 2021-12-17 and before tree.c 1.404 from
 * 2022-02-26, lint ran into an assertion failure due to a use-after-free.
 * When typeok checked the operand types of the '=', the left node and the
 * right node overlapped by 16 out of their 40 bytes on x86_64.
 */
void
statement_expr_with_loop(unsigned u)
{
	u = ({
		do {
		} while (0);
		u;
	});
}
