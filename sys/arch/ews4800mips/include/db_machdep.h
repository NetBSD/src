/*	$NetBSD: db_machdep.h,v 1.2 2011/07/09 16:58:04 matt Exp $	*/

#include <mips/db_machdep.h>

#ifdef DDB
void __db_print_symbol(db_expr_t);
#define	DB_PRINT_CALLER()						\
	__db_print_symbol((db_expr_t)__builtin_return_address(0))
#else
#define	DB_PRINT_CALLER()	((void)0)
#endif
