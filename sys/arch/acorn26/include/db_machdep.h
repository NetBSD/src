/* $NetBSD: db_machdep.h,v 1.7.6.1 2011/04/21 01:40:44 rmind Exp $ */

#include <arm/db_machdep.h>

/* acorn26 uses ELF */
#define DB_ELF_SYMBOLS
#define DB_ELFSIZE 32

void db_show_panic_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_bus_write_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_irqstat_cmd(db_expr_t, bool, db_expr_t, const char *);

extern volatile bool db_validating, db_faulted;
