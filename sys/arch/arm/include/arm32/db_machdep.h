/* $NetBSD: db_machdep.h,v 1.1 2002/01/05 22:41:48 chris Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_panic_cmd	__P((db_expr_t, int, db_expr_t, char *));
void db_show_frame_cmd	__P((db_expr_t, int, db_expr_t, char *));

#endif
