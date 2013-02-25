/* $NetBSD: db_machdep.h,v 1.5.22.2 2013/02/25 00:28:30 tls Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_show_fault_cmd(db_expr_t, bool, db_expr_t, const char *);

#endif
