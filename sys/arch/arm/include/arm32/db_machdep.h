/* $NetBSD: db_machdep.h,v 1.4 2007/02/22 04:38:03 matt Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_panic_cmd	__P((db_expr_t, bool, db_expr_t, const char *));
void db_show_frame_cmd	__P((db_expr_t, bool, db_expr_t, const char *));

#endif
