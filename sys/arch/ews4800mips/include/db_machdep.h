/*	$NetBSD: db_machdep.h,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

#include <mips/db_machdep.h>

#define	DB_ELF_SYMBOLS
#define	DB_ELFSIZE	32
#ifdef DDB
void __db_print_symbol(db_expr_t);
#define	DB_PRINT_CALLER()						\
	__db_print_symbol((db_expr_t)__builtin_return_address(0))
#else
#define	DB_PRINT_CALLER()	((void)0)
#endif
