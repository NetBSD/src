/* $NetBSD: db_machdep.h,v 1.3 2001/05/09 15:17:45 matt Exp $ */

#include <arm/db_machdep.h>

void db_of_boot_cmd (db_expr_t, int, db_expr_t, char *);
void db_of_enter_cmd (db_expr_t, int, db_expr_t, char *);
void db_of_exit_cmd (db_expr_t, int, db_expr_t, char *);

#define ARM32_DB_COMMANDS \
	{ "ofboot",	db_of_boot_cmd,		0, NULL }, \
	{ "ofenter",	db_of_enter_cmd,	0, NULL }, \
	{ "ofexit",	db_of_exit_cmd,		0, NULL }

