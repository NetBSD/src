/* $NetBSD: db_machdep.h,v 1.5.2.1 2002/01/10 19:38:36 thorpej Exp $ */

#include <arm/db_machdep.h>

/* arm26 uses ELF */
#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32

void db_show_panic_cmd	__P((db_expr_t, int, db_expr_t, char *));
void db_show_frame_cmd	__P((db_expr_t, int, db_expr_t, char *));
void db_bus_write_cmd	__P((db_expr_t, int, db_expr_t, char *));
void db_irqstat_cmd	__P((db_expr_t, int, db_expr_t, char *));
