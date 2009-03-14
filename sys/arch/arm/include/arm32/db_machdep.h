/* $NetBSD: db_machdep.h,v 1.5 2009/03/14 14:45:55 dsl Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_panic_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);

#endif
