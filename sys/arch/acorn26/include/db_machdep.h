/* $NetBSD: db_machdep.h,v 1.6.54.1 2009/01/19 13:15:51 skrll Exp $ */

#include <arm/db_machdep.h>

/* acorn26 uses ELF */
#undef DB_AOUT_SYMBOLS
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32

void db_show_panic_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_bus_write_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_irqstat_cmd(db_expr_t, bool, db_expr_t, const char *);

extern volatile bool db_validating, db_faulted;
