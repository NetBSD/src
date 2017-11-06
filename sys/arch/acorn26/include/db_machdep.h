/* $NetBSD: db_machdep.h,v 1.10 2017/11/06 03:47:45 christos Exp $ */

#include <arm/db_machdep.h>

/* acorn26 uses ELF */
#define DB_ELF_SYMBOLS

void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_bus_write_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_irqstat_cmd(db_expr_t, bool, db_expr_t, const char *);

extern volatile bool db_validating, db_faulted;
