/*	$NetBSD: db_command.c,v 1.72 2003/09/20 03:02:03 thorpej Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Command dispatcher.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_command.c,v 1.72 2003/09/20 03:02:03 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/vnode.h>

#include <machine/db_machdep.h>		/* type definitions */

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#endif
#ifdef MULTIPROCESSOR
#include <machine/cpu.h>
#endif

#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_command.h>
#include <ddb/db_break.h>
#include <ddb/db_watch.h>
#include <ddb/db_run.h>
#include <ddb/db_variables.h>
#include <ddb/db_interface.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_ddb.h>

#include "arp.h"

/*
 * Results of command search.
 */
#define	CMD_UNIQUE	0
#define	CMD_FOUND	1
#define	CMD_NONE	2
#define	CMD_AMBIGUOUS	3
#define	CMD_HELP	4

/*
 * Exported global variables
 */
boolean_t	db_cmd_loop_done;
label_t		*db_recover;
db_addr_t	db_dot;
db_addr_t	db_last_addr;
db_addr_t	db_prev;
db_addr_t	db_next;

/*
 * if 'ed' style: 'dot' is set at start of last item printed,
 * and '+' points to next line.
 * Otherwise: 'dot' points to next item, '..' points to last.
 */
static boolean_t db_ed_style = TRUE;

static void	db_buf_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_cmd_list(const struct db_command *);
static int	db_cmd_search(const char *, const struct db_command *,
		    const struct db_command **);
static void	db_command(const struct db_command **,
		    const struct db_command *);
static void	db_event_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_fncall(db_expr_t, int, db_expr_t, char *);
static void	db_malloc_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_map_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_namecache_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_object_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_page_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_pool_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_reboot_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_sifting_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_stack_trace_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_sync_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_uvmexp_print_cmd(db_expr_t, int, db_expr_t, char *);
static void	db_vnode_print_cmd(db_expr_t, int, db_expr_t, char *);

/*
 * 'show' commands
 */

static const struct db_command db_show_all_cmds[] = {
	{ "callout",	db_show_callout,	0, NULL },
	{ "procs",	db_show_all_procs,	0, NULL },
	{ NULL, 	NULL, 			0, NULL }
};

static const struct db_command db_show_cmds[] = {
	{ "all",	NULL,			0,	db_show_all_cmds },
#if defined(INET) && (NARP > 0)
	{ "arptab",	db_show_arptab,		0,	NULL },
#endif
	{ "breaks",	db_listbreak_cmd, 	0,	NULL },
	{ "buf",	db_buf_print_cmd,	0,	NULL },
	{ "event",	db_event_print_cmd,	0,	NULL },
	{ "malloc",	db_malloc_print_cmd,	0,	NULL },
	{ "map",	db_map_print_cmd,	0,	NULL },
	{ "ncache",	db_namecache_print_cmd,	0,	NULL },
	{ "object",	db_object_print_cmd,	0,	NULL },
	{ "page",	db_page_print_cmd,	0,	NULL },
	{ "pool",	db_pool_print_cmd,	0,	NULL },
	{ "registers",	db_show_regs,		0,	NULL },
	{ "sched_qs",	db_show_sched_qs,	0,	NULL },
	{ "uvmexp",	db_uvmexp_print_cmd,	0,	NULL },
	{ "vnode",	db_vnode_print_cmd,	0,	NULL },
	{ "watches",	db_listwatch_cmd, 	0,	NULL },
	{ NULL,		NULL,			0,	NULL }
};

static const struct db_command db_command_table[] = {
	{ "break",	db_breakpoint_cmd,	0,		NULL },
	{ "bt",		db_stack_trace_cmd,	0,		NULL },
	{ "c",		db_continue_cmd,	0,		NULL },
	{ "call",	db_fncall,		CS_OWN,		NULL },
	{ "callout",	db_show_callout,	0,		NULL },
	{ "continue",	db_continue_cmd,	0,		NULL },
	{ "d",		db_delete_cmd,		0,		NULL },
	{ "delete",	db_delete_cmd,		0,		NULL },
	{ "dmesg",	db_dmesg,		0,		NULL },
	{ "dwatch",	db_deletewatch_cmd,	0,		NULL },
	{ "examine",	db_examine_cmd,		CS_SET_DOT, 	NULL },
	{ "kill",	db_kill_proc,		CS_OWN,		NULL },
#ifdef KGDB
	{ "kgdb",	db_kgdb_cmd,		0,		NULL },
#endif
#ifdef DB_MACHINE_COMMANDS
	{ "machine",	NULL,			0, db_machine_command_table },
#endif
	{ "match",	db_trace_until_matching_cmd,0,		NULL },
	{ "next",	db_trace_until_matching_cmd,0,		NULL },
	{ "p",		db_print_cmd,		0,		NULL },
	{ "print",	db_print_cmd,		0,		NULL },
	{ "ps",		db_show_all_procs,	0,		NULL },
	{ "reboot",	db_reboot_cmd,		CS_OWN,		NULL },
	{ "s",		db_single_step_cmd,	0,		NULL },
	{ "search",	db_search_cmd,		CS_OWN|CS_SET_DOT, NULL },
	{ "set",	db_set_cmd,		CS_OWN,		NULL },
	{ "show",	NULL,			0,		db_show_cmds },
	{ "sifting",	db_sifting_cmd,		CS_OWN,		NULL },
	{ "step",	db_single_step_cmd,	0,		NULL },
	{ "sync",	db_sync_cmd,		CS_OWN,		NULL },
	{ "trace",	db_stack_trace_cmd,	0,		NULL },
	{ "until",	db_trace_until_call_cmd,0,		NULL },
	{ "w",		db_write_cmd,		CS_MORE|CS_SET_DOT, NULL },
	{ "watch",	db_watchpoint_cmd,	CS_MORE,	NULL },
	{ "write",	db_write_cmd,		CS_MORE|CS_SET_DOT, NULL },
	{ "x",		db_examine_cmd,		CS_SET_DOT, 	NULL },
	{ NULL, 	NULL,			0,		NULL }
};

static const struct db_command	*db_last_command = NULL;

/*
 * Utility routine - discard tokens through end-of-line.
 */
void
db_skip_to_eol(void)
{
	int t;

	do {
		t = db_read_token();
	} while (t != tEOL);
}

void
db_error(s)
	char *s;
{

	if (s)
		db_printf("%s", s);
	db_flush_lex();
	longjmp(db_recover);
}

void
db_command_loop(void)
{
	label_t	db_jmpbuf;
	label_t	*savejmp;

	/*
	 * Initialize 'prev' and 'next' to dot.
	 */
	db_prev = db_dot;
	db_next = db_dot;

	db_cmd_loop_done = 0;

	savejmp = db_recover;
	db_recover = &db_jmpbuf;
	(void) setjmp(&db_jmpbuf);

	while (!db_cmd_loop_done) {
		if (db_print_position() != 0)
			db_printf("\n");
		db_output_line = 0;


#ifdef MULTIPROCESSOR
		db_printf("db{%ld}> ", (long)cpu_number());
#else
		db_printf("db> ");
#endif
		(void) db_read_line();

		db_command(&db_last_command, db_command_table);
	}

	db_recover = savejmp;
}

/*
 * Search for command prefix.
 */
static int
db_cmd_search(const char *name, const struct db_command *table,
    const struct db_command **cmdp)
{
	const struct db_command	*cmd;
	int			result = CMD_NONE;

	for (cmd = table; cmd->name != 0; cmd++) {
		const char *lp;
		const char *rp;
		int  c;

		lp = name;
		rp = cmd->name;
		while ((c = *lp) == *rp) {
			if (c == 0) {
				/* complete match */
				*cmdp = cmd;
				return (CMD_UNIQUE);
			}
			lp++;
			rp++;
		}
		if (c == 0) {
			/* end of name, not end of command -
			   partial match */
			if (result == CMD_FOUND) {
				result = CMD_AMBIGUOUS;
				/* but keep looking for a full match -
				   this lets us match single letters */
			} else {
				*cmdp = cmd;
				result = CMD_FOUND;
			}
		}
	}
	if (result == CMD_NONE) {
		/* check for 'help' */
		if (name[0] == 'h' && name[1] == 'e'
		    && name[2] == 'l' && name[3] == 'p')
			result = CMD_HELP;
	}
	return (result);
}

static void
db_cmd_list(const struct db_command *table)
{
	int	 i, j, w, columns, lines, width=0, numcmds;
	const char	*p;

	for (numcmds = 0; table[numcmds].name != NULL; numcmds++) {
		w = strlen(table[numcmds].name);
		if (w > width)
			width = w;
	}
	width = DB_NEXT_TAB(width);

	columns = db_max_width / width;
	if (columns == 0)
		columns = 1;
	lines = (numcmds + columns - 1) / columns;
	for (i = 0; i < lines; i++) {
		for (j = 0; j < columns; j++) {
			p = table[j * lines + i].name;
			if (p)
				db_printf("%s", p);
			if (j * lines + i + lines >= numcmds) {
				db_putchar('\n');
				break;
			}
			w = strlen(p);
			while (w < width) {
				w = DB_NEXT_TAB(w);
				db_putchar('\t');
			}
		}
	}
}

static void
db_command(const struct db_command **last_cmdp,
    const struct db_command *cmd_table)
{
	const struct db_command	*cmd;
	int		t;
	char		modif[TOK_STRING_SIZE];
	db_expr_t	addr, count;
	boolean_t	have_addr = FALSE;
	int		result;

	static db_expr_t last_count = 0;

	t = db_read_token();
	if ((t == tEOL) || (t == tCOMMA)) {
		/*
		 * An empty line repeats last command, at 'next'.
		 * Only a count repeats the last command with the new count.
		 */
		cmd = *last_cmdp;
		addr = (db_expr_t)db_next;
		if (t == tCOMMA) {
			if (!db_expression(&count)) {
				db_printf("Count missing\n");
				db_flush_lex();
				return;
			}
		} else
			count = last_count;
		have_addr = FALSE;
		modif[0] = '\0';
		db_skip_to_eol();
	} else if (t == tEXCL) {
		db_fncall(0, 0, 0, NULL);
		return;
	} else if (t != tIDENT) {
		db_printf("?\n");
		db_flush_lex();
		return;
	} else {
		/*
		 * Search for command
		 */
		while (cmd_table) {
			result = db_cmd_search(db_tok_string, cmd_table, &cmd);
			switch (result) {
			case CMD_NONE:
				db_printf("No such command\n");
				db_flush_lex();
				return;
			case CMD_AMBIGUOUS:
				db_printf("Ambiguous\n");
				db_flush_lex();
				return;
			case CMD_HELP:
				db_cmd_list(cmd_table);
				db_flush_lex();
				return;
			default:
				break;
			}
			if ((cmd_table = cmd->more) != 0) {
				t = db_read_token();
				if (t != tIDENT) {
					db_cmd_list(cmd_table);
					db_flush_lex();
					return;
				}
			}
		}

		if ((cmd->flag & CS_OWN) == 0) {
			/*
			 * Standard syntax:
			 * command [/modifier] [addr] [,count]
			 */
			t = db_read_token();
			if (t == tSLASH) {
				t = db_read_token();
				if (t != tIDENT) {
					db_printf("Bad modifier\n");
					db_flush_lex();
					return;
				}
				strlcpy(modif, db_tok_string, sizeof(modif));
			} else {
				db_unread_token(t);
				modif[0] = '\0';
			}

			if (db_expression(&addr)) {
				db_dot = (db_addr_t) addr;
				db_last_addr = db_dot;
				have_addr = TRUE;
			} else {
				addr = (db_expr_t) db_dot;
				have_addr = FALSE;
			}
			t = db_read_token();
			if (t == tCOMMA) {
				if (!db_expression(&count)) {
					db_printf("Count missing\n");
					db_flush_lex();
					return;
				}
			} else {
				db_unread_token(t);
				count = -1;
			}
			if ((cmd->flag & CS_MORE) == 0) {
				db_skip_to_eol();
			}
		}
	}
	*last_cmdp = cmd;
	last_count = count;
	if (cmd != 0) {
		/*
		 * Execute the command.
		 */
		(*cmd->fcn)(addr, have_addr, count, modif);

		if (cmd->flag & CS_SET_DOT) {
			/*
			 * If command changes dot, set dot to
			 * previous address displayed (if 'ed' style).
			 */
			if (db_ed_style)
				db_dot = db_prev;
			else
				db_dot = db_next;
		} else {
			/*
			 * If command does not change dot,
			 * set 'next' location to be the same.
			 */
			db_next = db_dot;
		}
	}
}

/*ARGSUSED*/
static void
db_map_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	boolean_t full = FALSE;

	if (modif[0] == 'f')
		full = TRUE;

	if (have_addr == FALSE)
		addr = (db_expr_t)(intptr_t) kernel_map;

	uvm_map_printit((struct vm_map *)(intptr_t) addr, full, db_printf);
}

/*ARGSUSED*/
static void
db_malloc_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

#ifdef MALLOC_DEBUG
	if (!have_addr)
		addr = 0;

	debug_malloc_printit(db_printf, (vaddr_t) addr);
#else
	db_printf("The kernel is not built with the MALLOC_DEBUG option.\n");
#endif /* MALLOC_DEBUG */
}

/*ARGSUSED*/
static void
db_object_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	boolean_t full = FALSE;

	if (modif[0] == 'f')
		full = TRUE;

	uvm_object_printit((struct uvm_object *)(intptr_t) addr, full,
	    db_printf);
}

/*ARGSUSED*/
static void
db_page_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	boolean_t full = FALSE;

	if (modif[0] == 'f')
		full = TRUE;

	uvm_page_printit((struct vm_page *)(intptr_t) addr, full, db_printf);
}

/*ARGSUSED*/
static void
db_buf_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	boolean_t full = FALSE;

	if (modif[0] == 'f')
		full = TRUE;

	vfs_buf_print((struct buf *)(intptr_t) addr, full, db_printf);
}

/*ARGSUSED*/
static void
db_event_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	boolean_t full = FALSE;

	if (modif[0] == 'f')
		full = TRUE;

	event_print(full, db_printf);
}

/*ARGSUSED*/
static void
db_vnode_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	boolean_t full = FALSE;

	if (modif[0] == 'f')
		full = TRUE;

	vfs_vnode_print((struct vnode *)(intptr_t) addr, full, db_printf);
}

/*ARGSUSED*/
static void
db_pool_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

	pool_printit((struct pool *)(intptr_t) addr, modif, db_printf);
}

/*ARGSUSED*/
static void
db_namecache_print_cmd(db_expr_t addr, int have_addr, db_expr_t count,
    char *modif)
{

	namecache_print((struct vnode *)(intptr_t) addr, db_printf);
}

/*ARGSUSED*/
static void
db_uvmexp_print_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

	uvmexp_print(db_printf);
}

/*
 * Call random function:
 * !expr(arg,arg,arg)
 */
/*ARGSUSED*/
static void
db_fncall(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	db_expr_t	fn_addr;
#define	MAXARGS		11
	db_expr_t	args[MAXARGS];
	int		nargs = 0;
	db_expr_t	retval;
	db_expr_t	(*func)(db_expr_t, ...);
	int		t;

	if (!db_expression(&fn_addr)) {
		db_printf("Bad function\n");
		db_flush_lex();
		return;
	}
	func = (db_expr_t (*)(db_expr_t, ...))(intptr_t) fn_addr;

	t = db_read_token();
	if (t == tLPAREN) {
		if (db_expression(&args[0])) {
			nargs++;
			while ((t = db_read_token()) == tCOMMA) {
				if (nargs == MAXARGS) {
					db_printf("Too many arguments\n");
					db_flush_lex();
					return;
				}
				if (!db_expression(&args[nargs])) {
					db_printf("Argument missing\n");
					db_flush_lex();
					return;
				}
				nargs++;
			}
			db_unread_token(t);
		}
		if (db_read_token() != tRPAREN) {
			db_printf("?\n");
			db_flush_lex();
			return;
		}
	}
	db_skip_to_eol();

	while (nargs < MAXARGS) {
		args[nargs++] = 0;
	}

	retval = (*func)(args[0], args[1], args[2], args[3], args[4],
			 args[5], args[6], args[7], args[8], args[9]);
	db_printf("%s\n", db_num_to_str(retval));
}

static void
db_reboot_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	db_expr_t bootflags;

	/* Flags, default to RB_AUTOBOOT */
	if (!db_expression(&bootflags))
		bootflags = (db_expr_t)RB_AUTOBOOT;
	if (db_read_token() != tEOL) {
		db_error("?\n");
		/*NOTREACHED*/
	}
	/*
	 * We are leaving DDB, never to return upward.
	 * Clear db_recover so that we can debug faults in functions
	 * called from cpu_reboot.
	 */
	db_recover = 0;
	cpu_reboot((int)bootflags, NULL);
}

static void
db_sifting_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	int	mode, t;

	t = db_read_token();
	if (t == tSLASH) {
		t = db_read_token();
		if (t != tIDENT) {
			bad_modifier:
			db_printf("Bad modifier\n");
			db_flush_lex();
			return;
		}
		if (!strcmp(db_tok_string, "F"))
			mode = 'F';
		else
			goto bad_modifier;
		t = db_read_token();
	} else
		mode = 0;

	if (t == tIDENT)
		db_sifting(db_tok_string, mode);
	else {
		db_printf("Bad argument (non-string)\n");
		db_flush_lex();
	}
}

static void
db_stack_trace_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	register char *cp = modif;
	register char c;
	void (*pr)(const char *, ...);

	pr = db_printf;
	while ((c = *cp++) != 0)
		if (c == 'l')
			pr = printf;

	if (count == -1)
		count = 65535;

	db_stack_trace_print(addr, have_addr, count, modif, pr);
}

static void
db_sync_cmd(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{

	/*
	 * We are leaving DDB, never to return upward.
	 * Clear db_recover so that we can debug faults in functions
	 * called from cpu_reboot.
	 */
	db_recover = 0;
	cpu_reboot(RB_DUMP, NULL);
}
