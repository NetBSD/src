/* $NetBSD: db_machdep.h,v 1.4.66.1 2014/02/15 16:18:36 matt Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_show_fault_cmd(db_expr_t, bool, db_expr_t, const char *);

#endif
