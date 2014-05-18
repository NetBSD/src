/* $NetBSD: db_machdep.h,v 1.7.2.1 2014/05/18 17:44:58 rmind Exp $ */

#ifndef _ARM32_DB_MACHDEP_H_
#define _ARM32_DB_MACHDEP_H_

#include <arm/db_machdep.h>

void db_show_frame_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_show_fault_cmd(db_expr_t, bool, db_expr_t, const char *);
#ifdef _KERNEL
void db_show_tlb_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif
#if defined(_KERNEL) && defined(MULTIPROCESSOR)
void db_switch_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif

#endif
