/* $NetBSD: db_machdep.h,v 1.4 2006/09/27 21:21:09 bjh21 Exp $ */

#include <arm/db_machdep.h>

/* acorn26 uses ELF */
#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32

void db_show_panic_cmd	__P((db_expr_t, int, db_expr_t, const char *));
void db_show_frame_cmd	__P((db_expr_t, int, db_expr_t, const char *));
void db_bus_write_cmd	__P((db_expr_t, int, db_expr_t, const char *));
void db_irqstat_cmd	__P((db_expr_t, int, db_expr_t, const char *));

extern volatile boolean_t db_validating, db_faulted;
