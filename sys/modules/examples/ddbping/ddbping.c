/*	$NetBSD: ddbping.c,v 1.3 2023/02/01 10:22:20 uwe Exp $ */
/*
 * Copyright (c) 2020 Valery Ushakov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Example of a kernel module that registers DDB commands.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ddbping.c,v 1.3 2023/02/01 10:22:20 uwe Exp $");

#include <sys/param.h>
#include <sys/module.h>

#include <ddb/ddb.h>

/* XXX: db_command.h should provide something like this */
typedef void db_cmdfn_t(db_expr_t, bool, db_expr_t, const char *);


static db_cmdfn_t db_ping;
static db_cmdfn_t db_show_ping;


static const struct db_command db_ping_base_tbl[] = {
	{ DDB_ADD_CMD("ping", db_ping, 0,
		      "Example command",
		      NULL, NULL) },
	{ DDB_END_CMD },
};

static const struct db_command db_ping_show_tbl[] = {
	{ DDB_ADD_CMD("ping", db_show_ping, 0,
		      "Example command stats",
		      NULL, NULL) },
	{ DDB_END_CMD },
};


static unsigned int ping_count;
static unsigned int ping_count_modif;
static unsigned int ping_count_addr;
static unsigned int ping_count_count;


static void
db_ping(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	db_printf("pong");
	++ping_count;

	if (modif != NULL && *modif != '\0') {
		db_printf("/%s", modif);
		++ping_count_modif;
	}

	if (have_addr) {
		db_printf(" 0x%zx", (size_t)addr);
		++ping_count_addr;
	}

	if (count > 0) {
		db_printf(", 0t%zu", (size_t)count);
		++ping_count_count;
	}

	db_printf("\n");
}


static void
db_show_ping(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	db_printf("total\t\t%u\n", ping_count);
	db_printf("with modifiers\t%u\n", ping_count_modif);
	db_printf("with address\t%u\n", ping_count_addr);
	db_printf("with count\t%u\n", ping_count_count);
}


MODULE(MODULE_CLASS_MISC, ddbping, NULL);

static int
ddbping_modcmd(modcmd_t cmd, void *arg __unused)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		db_register_tbl(DDB_BASE_CMD, db_ping_base_tbl);
		db_register_tbl(DDB_SHOW_CMD, db_ping_show_tbl);
		break;

	case MODULE_CMD_FINI:
		db_unregister_tbl(DDB_BASE_CMD, db_ping_base_tbl);
		db_unregister_tbl(DDB_SHOW_CMD, db_ping_show_tbl);
		break;

	case MODULE_CMD_STAT:	/* FALLTHROUGH */
	default:
		return ENOTTY;
	}

	return 0;
}
