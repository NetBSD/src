/*	$NetBSD: db_machdep.h,v 1.1.18.2 2006/06/21 14:51:12 yamt Exp $	*/

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
