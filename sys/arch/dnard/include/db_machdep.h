/* $NetBSD: db_machdep.h,v 1.2.4.2 2001/03/12 13:28:02 bouyer Exp $ */

void db_of_boot_cmd	(db_expr_t addr, int have_addr, db_expr_t count, char *modif);
void db_of_enter_cmd	(db_expr_t addr, int have_addr, db_expr_t count, char *modif);
void db_of_exit_cmd	(db_expr_t addr, int have_addr, db_expr_t count, char *modif);

#define ARM32_DB_COMMANDS \
	{ "ofboot",	db_of_boot_cmd,		0, NULL }, \
	{ "ofenter",	db_of_enter_cmd,	0, NULL }, \
	{ "ofexit",	db_of_exit_cmd,		0, NULL }

#include <arm/db_machdep.h>
