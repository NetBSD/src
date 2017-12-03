/* $NetBSD: db_machdep.h,v 1.8.14.2 2017/12/03 11:35:44 jdolecek Exp $ */

#include <arm/db_machdep.h>

/* acorn26 uses ELF */
#define DB_ELF_SYMBOLS

void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_bus_write_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_irqstat_cmd(db_expr_t, bool, db_expr_t, const char *);

extern volatile bool db_validating, db_faulted;
