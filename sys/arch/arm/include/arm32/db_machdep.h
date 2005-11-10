/* $NetBSD: db_machdep.h,v 1.1.22.1 2005/11/10 13:55:16 skrll Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_panic_cmd	__P((db_expr_t, int, db_expr_t, const char *));
void db_show_frame_cmd	__P((db_expr_t, int, db_expr_t, const char *));

#endif
