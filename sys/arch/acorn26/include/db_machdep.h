/* $NetBSD: db_machdep.h,v 1.7 2009/01/14 23:14:48 bjh21 Exp $ */

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
