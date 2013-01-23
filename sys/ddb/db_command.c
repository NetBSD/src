/*	$NetBSD: db_command.c,v 1.136.4.3 2013/01/23 00:06:03 yamt Exp $	*/

/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2002, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik, and by Andrew Doran.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
__KERNEL_RCSID(0, "$NetBSD: db_command.c,v 1.136.4.3 2013/01/23 00:06:03 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_aio.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_mqueue.h"
#include "opt_inet.h"
#include "opt_kernhist.h"
#include "opt_ddbparam.h"
#include "opt_multiprocessor.h"
#include "arp.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/mbuf.h>
#include <sys/namei.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/vmem.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>
#include <sys/buf.h>
#include <sys/module.h>
#include <sys/kernhist.h>

/*include queue macros*/
#include <sys/queue.h>

#include <ddb/ddb.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_ddb.h>

/*
 * Results of command search.
 */
#define	CMD_EXACT		0
#define	CMD_PREFIX		1
#define	CMD_NONE		2
#define	CMD_AMBIGUOUS	3

/*
 * Exported global variables
 */
bool		db_cmd_loop_done;
label_t		*db_recover;
db_addr_t	db_dot;
db_addr_t	db_last_addr;
db_addr_t	db_prev;
db_addr_t	db_next;


/*
 * New DDB api for adding and removing commands uses three lists, because
 * we use two types of commands
 * a) standard commands without subcommands -> reboot
 * b) show commands which are subcommands of show command -> show aio_jobs
 * c) if defined machine specific commands
 *
 * ddb_add_cmd, ddb_rem_cmd use type (DDB_SHOW_CMD||DDB_BASE_CMD)argument to
 * add them to representativ lists.
 */

static const struct db_command db_command_table[];
static const struct db_command db_show_cmds[];

#ifdef DB_MACHINE_COMMANDS
/* arch/<arch>/<arch>/db_interface.c */
extern const struct db_command db_machine_command_table[];
#endif

/* the global queue of all command tables */
TAILQ_HEAD(db_cmd_tbl_en_head, db_cmd_tbl_en);

/* TAILQ entry used to register command tables */
struct db_cmd_tbl_en {
	const struct db_command *db_cmd;	/* cmd table */
	TAILQ_ENTRY(db_cmd_tbl_en) db_cmd_next;
};

/* head of base commands list */
static struct db_cmd_tbl_en_head db_base_cmd_list =
	TAILQ_HEAD_INITIALIZER(db_base_cmd_list);
static struct db_cmd_tbl_en db_base_cmd_builtins =
     { .db_cmd = db_command_table };

/* head of show commands list */
static struct db_cmd_tbl_en_head db_show_cmd_list =
	TAILQ_HEAD_INITIALIZER(db_show_cmd_list);
static struct db_cmd_tbl_en db_show_cmd_builtins =
     { .db_cmd = db_show_cmds };

/* head of machine commands list */
static struct db_cmd_tbl_en_head db_mach_cmd_list =
	TAILQ_HEAD_INITIALIZER(db_mach_cmd_list);
#ifdef DB_MACHINE_COMMANDS
static struct db_cmd_tbl_en db_mach_cmd_builtins =
     { .db_cmd = db_machine_command_table };
#endif

/*
 * if 'ed' style: 'dot' is set at start of last item printed,
 * and '+' points to next line.
 * Otherwise: 'dot' points to next item, '..' points to last.
 */
static bool	 db_ed_style = true;

static void	db_init_commands(void);
static int	db_register_tbl_entry(uint8_t type,
    struct db_cmd_tbl_en *list_ent);
static void	db_cmd_list(const struct db_cmd_tbl_en_head *);
static int	db_cmd_search(const char *, struct db_cmd_tbl_en_head *,
			      const struct db_command **);
static int	db_cmd_search_table(const char *, const struct db_command *,
				    const struct db_command **);
static void	db_cmd_search_failed(char *, int);
static const struct db_command *db_read_command(void);
static void	db_command(const struct db_command **);
static void	db_buf_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_event_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_fncall(db_expr_t, bool, db_expr_t, const char *);
static void     db_help_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_lock_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_mount_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_mbuf_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_map_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_namecache_print_cmd(db_expr_t, bool, db_expr_t,
		    const char *);
static void	db_object_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_page_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_show_all_pages(db_expr_t, bool, db_expr_t, const char *);
static void	db_pool_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_reboot_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_sifting_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_stack_trace_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_sync_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_whatis_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_uvmexp_print_cmd(db_expr_t, bool, db_expr_t, const char *);
#ifdef KERNHIST
static void	db_kernhist_print_cmd(db_expr_t, bool, db_expr_t, const char *);
#endif
static void	db_vnode_print_cmd(db_expr_t, bool, db_expr_t, const char *);
static void	db_vmem_print_cmd(db_expr_t, bool, db_expr_t, const char *);

static const struct db_command db_show_cmds[] = {
	/*added from all sub cmds*/
#ifdef _KERNEL	/* XXX CRASH(8) */
	{ DDB_ADD_CMD("callout",  db_show_callout,
	    0 ,"List all used callout functions.",NULL,NULL) },
#endif
	{ DDB_ADD_CMD("pages",	db_show_all_pages,
	    0 ,"List all used memory pages.",NULL,NULL) },
	{ DDB_ADD_CMD("proc",	db_show_proc,
	    0 ,"Print process information.",NULL,NULL) },
	{ DDB_ADD_CMD("procs",	db_show_all_procs,
	    0 ,"List all processes.",NULL,NULL) },
	{ DDB_ADD_CMD("pools",	db_show_all_pools,
	    0 ,"Show all pools",NULL,NULL) },
#ifdef AIO
	/*added from all sub cmds*/
	{ DDB_ADD_CMD("aio_jobs",	db_show_aio_jobs,	0,
	    "Show aio jobs",NULL,NULL) },
#endif
	{ DDB_ADD_CMD("all",	NULL,
	    CS_COMPAT, NULL,NULL,NULL) },
#if defined(INET) && (NARP > 0)
	{ DDB_ADD_CMD("arptab",	db_show_arptab,		0,NULL,NULL,NULL) },
#endif
#ifdef _KERNEL
	{ DDB_ADD_CMD("breaks",	db_listbreak_cmd, 	0,
	    "Display all breaks.",NULL,NULL) },
#endif
	{ DDB_ADD_CMD("buf",	db_buf_print_cmd,	0,
	    "Print the struct buf at address.", "[/f] address",NULL) },
	{ DDB_ADD_CMD("event",	db_event_print_cmd,	0,
	    "Print all the non-zero evcnt(9) event counters.", "[/fitm]",NULL) },
	{ DDB_ADD_CMD("files", db_show_files_cmd,	0,
	    "Print the files open by process at address",
	    "[/f] address", NULL) },
	{ DDB_ADD_CMD("lock",	db_lock_print_cmd,	0,NULL,NULL,NULL) },
	{ DDB_ADD_CMD("map",	db_map_print_cmd,	0,
	    "Print the vm_map at address.", "[/f] address",NULL) },
	{ DDB_ADD_CMD("module", db_show_module_cmd,	0,
	    "Print kernel modules", NULL, NULL) },
	{ DDB_ADD_CMD("mount",	db_mount_print_cmd,	0,
	    "Print the mount structure at address.", "[/f] address",NULL) },
#ifdef MQUEUE
	{ DDB_ADD_CMD("mqueue", db_show_mqueue_cmd,	0,
	    "Print the message queues", NULL, NULL) },
#endif
	{ DDB_ADD_CMD("mbuf",	db_mbuf_print_cmd,	0,NULL,NULL,
	    "-c prints all mbuf chains") },
	{ DDB_ADD_CMD("ncache",	db_namecache_print_cmd,	0,
	    "Dump the namecache list.", "address",NULL) },
	{ DDB_ADD_CMD("object",	db_object_print_cmd,	0,
	    "Print the vm_object at address.", "[/f] address",NULL) },
	{ DDB_ADD_CMD("page",	db_page_print_cmd,	0,
	    "Print the vm_page at address.", "[/f] address",NULL) },
	{ DDB_ADD_CMD("panic",	db_show_panic,	0,
	    "Print the current panic string",NULL,NULL) },
	{ DDB_ADD_CMD("pool",	db_pool_print_cmd,	0,
	    "Print the pool at address.", "[/clp] address",NULL) },
	{ DDB_ADD_CMD("registers",	db_show_regs,		0,
	    "Display the register set.", "[/u]",NULL) },
	{ DDB_ADD_CMD("sched_qs",	db_show_sched_qs,	0,
	    "Print the state of the scheduler's run queues.",
	    NULL,NULL) },
	{ DDB_ADD_CMD("uvmexp",	db_uvmexp_print_cmd, 0,
	    "Print a selection of UVM counters and statistics.",
	    NULL,NULL) },
#ifdef KERNHIST
	{ DDB_ADD_CMD("kernhist", db_kernhist_print_cmd, 0,
	    "Print the UVM history logs.",
	    NULL,NULL) },
#endif
	{ DDB_ADD_CMD("vnode",	db_vnode_print_cmd,	0,
	    "Print the vnode at address.", "[/f] address",NULL) },
	{ DDB_ADD_CMD("vmem", db_vmem_print_cmd,	0,
	    "Print the vmem usage.", "[/a] address", NULL) },
	{ DDB_ADD_CMD("vmems", db_show_all_vmems,	0,
	    "Show all vmems.", NULL, NULL) },
#ifdef _KERNEL
	{ DDB_ADD_CMD("watches",	db_listwatch_cmd, 	0,
	    "Display all watchpoints.", NULL,NULL) },
#endif
	{ DDB_ADD_CMD(NULL,		NULL,			0,NULL,NULL,NULL) }
};

static const struct db_command db_command_table[] = {
	{ DDB_ADD_CMD("b",		db_breakpoint_cmd,	0,
	    "Set a breakpoint at address", "[/u] address[,count].",NULL) },
	{ DDB_ADD_CMD("break",	db_breakpoint_cmd,	0,
	    "Set a breakpoint at address", "[/u] address[,count].",NULL) },
	{ DDB_ADD_CMD("bt",		db_stack_trace_cmd,	0,
	    "Show backtrace.", "See help trace.",NULL) },
	{ DDB_ADD_CMD("c",		db_continue_cmd,	0,
	    "Continue execution.", "[/c]",NULL) },
	{ DDB_ADD_CMD("call",	db_fncall,		CS_OWN,
	    "Call the function", "address[(expression[,...])]",NULL) },
#ifdef _KERNEL	/* XXX CRASH(8) */
	{ DDB_ADD_CMD("callout",	db_show_callout,	0, NULL,
	    NULL,NULL ) },
#endif
	{ DDB_ADD_CMD("continue",	db_continue_cmd,	0,
	    "Continue execution.", "[/c]",NULL) },
	{ DDB_ADD_CMD("d",		db_delete_cmd,		0,
	    "Delete a breakpoint.", "address | #number",NULL) },
	{ DDB_ADD_CMD("delete",	db_delete_cmd,		0,
	    "Delete a breakpoint.", "address | #number",NULL) },
	{ DDB_ADD_CMD("dmesg",	db_dmesg,		0,
	    "Show kernel message buffer.", "[count]",NULL) },
	{ DDB_ADD_CMD("dwatch",	db_deletewatch_cmd,	0,
	    "Delete the watchpoint.", "address",NULL) },
	{ DDB_ADD_CMD("examine",	db_examine_cmd,		CS_SET_DOT,
	    "Display the address locations.",
	    "[/modifier] address[,count]",NULL) },
	{ DDB_ADD_CMD("exit",		db_continue_cmd,	0,
	    "Continue execution.", "[/c]",NULL) },
	{ DDB_ADD_CMD("help",   db_help_print_cmd, CS_OWN|CS_NOREPEAT,
	    "Display help about commands",
	    "Use other commands as arguments.",NULL) },
	{ DDB_ADD_CMD("kill",	db_kill_proc,		CS_OWN,
	    "Send a signal to the process","pid[,signal_number]",
	    "   pid:\t\t\tthe process id (may need 0t prefix for decimal)\n"
	    "   signal_number:\tthe signal to send") },
#ifdef KGDB
	{ DDB_ADD_CMD("kgdb",	db_kgdb_cmd,	0,	NULL,NULL,NULL) },
#endif
	{ DDB_ADD_CMD("machine",NULL,CS_MACH,
	    "Architecture specific functions.",NULL,NULL) },
	{ DDB_ADD_CMD("match",	db_trace_until_matching_cmd,0,
	    "Stop at the matching return instruction.","See help next",NULL) },
	{ DDB_ADD_CMD("next",	db_trace_until_matching_cmd,0,
	    "Stop at the matching return instruction.","[/p]",NULL) },
	{ DDB_ADD_CMD("p",		db_print_cmd,		0,
	    "Print address according to the format.",
	    "[/axzodurc] address [address ...]",NULL) },
	{ DDB_ADD_CMD("print",	db_print_cmd,		0,
	    "Print address according to the format.",
	    "[/axzodurc] address [address ...]",NULL) },
	{ DDB_ADD_CMD("ps",		db_show_all_procs,	0,
	    "Print all processes.","See show all procs",NULL) },
	{ DDB_ADD_CMD("quit",		db_continue_cmd,	0,
	    "Continue execution.", "[/c]",NULL) },
	{ DDB_ADD_CMD("reboot",	db_reboot_cmd,		CS_OWN,
	    "Reboot","0x1  RB_ASKNAME, 0x2 RB_SINGLE, 0x4 RB_NOSYNC, 0x8 RB_HALT,"
	    "0x40 RB_KDB, 0x100 RB_DUMP, 0x808 RB_POWERDOWN",NULL) },
	{ DDB_ADD_CMD("s",		db_single_step_cmd,	0,
	    "Single-step count times.","[/p] [,count]",NULL) },
	{ DDB_ADD_CMD("search",	db_search_cmd,		CS_OWN|CS_SET_DOT,
	    "Search memory from address for value.",
	    "[/bhl] address value [mask] [,count]",NULL) },
	{ DDB_ADD_CMD("set",	db_set_cmd,		CS_OWN,
	    "Set the named variable","$variable [=] expression",NULL) },
	{ DDB_ADD_CMD("show",	NULL, CS_SHOW,
	    "Show kernel stats.", NULL,NULL) },
	{ DDB_ADD_CMD("sifting",	db_sifting_cmd,		CS_OWN,
	    "Search the symbol tables ","[/F] string",NULL) },
	{ DDB_ADD_CMD("step",	db_single_step_cmd,	0,
	    "Single-step count times.","[/p] [,count]",NULL) },
	{ DDB_ADD_CMD("sync",	db_sync_cmd,		CS_OWN,
	    "Force a crash dump, and then reboot.",NULL,NULL) },
	{ DDB_ADD_CMD("trace",	db_stack_trace_cmd,	0,
	    "Stack trace from frame-address.",
	    "[/u[l]] [frame-address][,count]",NULL) },
	{ DDB_ADD_CMD("until",	db_trace_until_call_cmd,0,
	    "Stop at the next call or return instruction.","[/p]",NULL) },
	{ DDB_ADD_CMD("w",		db_write_cmd,		CS_MORE|CS_SET_DOT,
	    "Write the expressions at succeeding locations.",
	    "[/bhl] address expression [expression ...]",NULL) },
	{ DDB_ADD_CMD("watch",	db_watchpoint_cmd,	CS_MORE,
	    "Set a watchpoint for a region. ","address[,size]",NULL) },
	{ DDB_ADD_CMD("whatis",	db_whatis_cmd, 0,
	    "Describe what an address is", "address", NULL) },
	{ DDB_ADD_CMD("write",	db_write_cmd,		CS_MORE|CS_SET_DOT,
	    "Write the expressions at succeeding locations.",
	    "[/bhl] address expression [expression ...]",NULL) },
	{ DDB_ADD_CMD("x",		db_examine_cmd,		CS_SET_DOT,
	    "Display the address locations.",
	    "[/modifier] address[,count]",NULL) },
	{ DDB_ADD_CMD(NULL, 	NULL,		   0, NULL, NULL, NULL) }
};

static const struct db_command	*db_last_command = NULL;
#if defined(DDB_COMMANDONENTER)
char db_cmd_on_enter[DB_LINE_MAXLEN + 1] = ___STRING(DDB_COMMANDONENTER);
#else /* defined(DDB_COMMANDONENTER) */
char db_cmd_on_enter[DB_LINE_MAXLEN + 1] = "";
#endif /* defined(DDB_COMMANDONENTER) */
#define	DB_LINE_SEP	';'

/*
 * Execute commandlist after ddb start
 * This function goes through the command list created from commands and ';'
 */
static void
db_execute_commandlist(const char *cmdlist)
{
	const char *cmd = cmdlist;
	const struct db_command	*dummy = NULL;

	while (*cmd != '\0') {
		const char *ep = cmd;

		while (*ep != '\0' && *ep != DB_LINE_SEP) {
			ep++;
		}
		db_set_line(cmd, ep);
		db_command(&dummy);
		cmd = ep;
		if (*cmd == DB_LINE_SEP) {
			cmd++;
		}
	}
}

/* Initialize ddb command tables */
void
db_init_commands(void)
{
	static bool done = false;

	if (done) return;
	done = true;

	/* register command tables */
	(void)db_register_tbl_entry(DDB_BASE_CMD, &db_base_cmd_builtins);
#ifdef DB_MACHINE_COMMANDS
	(void)db_register_tbl_entry(DDB_MACH_CMD, &db_mach_cmd_builtins);
#endif
	(void)db_register_tbl_entry(DDB_SHOW_CMD, &db_show_cmd_builtins);
}


/*
 * Add command table to the specified list
 * Arg:
 * int type specifies type of command table DDB_SHOW_CMD|DDB_BASE_CMD|DDB_MAC_CMD
 * *cmd_tbl poiter to static allocated db_command table
 *
 * Command table must be NULL terminated array of struct db_command
 */
int
db_register_tbl(uint8_t type, const struct db_command *cmd_tbl)
{
	struct db_cmd_tbl_en *list_ent;
	
	/* empty list - ignore */
	if (cmd_tbl->name == 0)
		return 0;

	/* force builtin commands to be registered first */
	db_init_commands();

	/* now create a list entry for this table */
	list_ent = db_zalloc(sizeof(*list_ent));
	if (list_ent == NULL)
		return ENOMEM;
	list_ent->db_cmd=cmd_tbl;

	/* and register it */
	return db_register_tbl_entry(type, list_ent);
}

static int
db_register_tbl_entry(uint8_t type, struct db_cmd_tbl_en *list_ent)
{
	struct db_cmd_tbl_en_head *list;

	switch(type) {
	case DDB_BASE_CMD:
		list = &db_base_cmd_list;
		break;
	case DDB_SHOW_CMD:
		list = &db_show_cmd_list;
		break;
	case DDB_MACH_CMD:
		list = &db_mach_cmd_list;
		break;
	default:
		return ENOENT;
	}

	TAILQ_INSERT_TAIL(list, list_ent, db_cmd_next);

	return 0;
}

/*
 * Remove command table specified with db_cmd address == cmd_tbl
 */
int
db_unregister_tbl(uint8_t type,const struct db_command *cmd_tbl)
{
	struct db_cmd_tbl_en *list_ent;
	struct db_cmd_tbl_en_head *list;

	/* find list on which the entry should live */
	switch (type) {
	case DDB_BASE_CMD:
		list=&db_base_cmd_list;
		break;
	case DDB_SHOW_CMD:
		list=&db_show_cmd_list;
		break;
	case DDB_MACH_CMD:
		list=&db_mach_cmd_list;
		break;
	default:
		return EINVAL;
	}

	TAILQ_FOREACH (list_ent, list, db_cmd_next) {
		if (list_ent->db_cmd == cmd_tbl){
			TAILQ_REMOVE(list,
			    list_ent, db_cmd_next);
			db_free(list_ent, sizeof(*list_ent));
			return 0;
		}
	}
	return ENOENT;
}

/* This function is called from machine trap code. */
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

	db_cmd_loop_done = false;

	/* Init default command tables add machine, base,
	   show command tables to the list */
	db_init_commands();

	/* save context for return from ddb */
	savejmp = db_recover;
	db_recover = &db_jmpbuf;
	(void) setjmp(&db_jmpbuf);

	/* Execute default ddb start commands */
	db_execute_commandlist(db_cmd_on_enter);

	(void) setjmp(&db_jmpbuf);
	while (!db_cmd_loop_done) {
		if (db_print_position() != 0)
			db_printf("\n");
		db_output_line = 0;
		(void) db_read_line();
		db_command(&db_last_command);
	}

	db_recover = savejmp;
}

/*
 * Search command table for command prefix
 */
static int
db_cmd_search_table(const char *name,
		    const struct db_command *table,
		    const struct db_command **cmdp)
{

	const struct db_command	*cmd;
	int result;

	result = CMD_NONE;
	*cmdp = NULL;

	for (cmd = table; cmd->name != 0; cmd++) {
		const char *lp;
		const char *rp;

		lp = name;
		rp = cmd->name;
		while (*lp != '\0' && *lp == *rp) {
			rp++;
			lp++;
		}

		if (*lp != '\0') /* mismatch or extra chars in name */
			continue;

		if (*rp == '\0') { /* exact match */
			*cmdp = cmd;
			return (CMD_EXACT);
		}

		/* prefix match: end of name, not end of command */
		if (result == CMD_NONE) {
			result = CMD_PREFIX;
			*cmdp = cmd;
		}
		else if (result == CMD_PREFIX) {
			result = CMD_AMBIGUOUS;
			*cmdp = NULL;
		}
	}

	return (result);
}


/*
 * Search list of command tables for command
 */
static int
db_cmd_search(const char *name,
	      struct db_cmd_tbl_en_head *list_head,
	      const struct db_command **cmdp)
{
	struct db_cmd_tbl_en *list_ent;
	const struct db_command *found_command;
	bool accept_prefix_match;
	int result;

	result = CMD_NONE;
	found_command = NULL;
	accept_prefix_match = true;

	TAILQ_FOREACH(list_ent, list_head, db_cmd_next) {
		const struct db_command *cmd;
		int found;

		found = db_cmd_search_table(name, list_ent->db_cmd, &cmd);
		if (found == CMD_EXACT) {
			result = CMD_EXACT;
			found_command = cmd;
			break;
		}

		if (found == CMD_PREFIX) {
			if (accept_prefix_match) {
				/*
				 * Continue search, but note current result
				 * in case we won't find anything else.
				 */
				accept_prefix_match = false;
				result = CMD_PREFIX;
				found_command = cmd;
			} else {
				/*
				 * Watch out for globally ambiguous
				 * prefix match that is not locally
				 * ambiguous - with one match in one
				 * table and another match(es) in
				 * another table.
				 */
				result = CMD_AMBIGUOUS;
				found_command = NULL;
			}
		}
		else if (found == CMD_AMBIGUOUS) {
			accept_prefix_match = false;
			result = CMD_AMBIGUOUS;
			found_command = NULL;
		}
	}

	*cmdp = found_command;
	return result;
}

static void
db_cmd_search_failed(char *name, int search_result)
{
	if (search_result == CMD_NONE)
		db_printf("No such command: %s\n", name);
	else
		db_printf("Ambiguous command: %s\n", name);
}


/*
 * List commands to the console.
 */
static void
db_cmd_list(const struct db_cmd_tbl_en_head *list)
{

	struct db_cmd_tbl_en *list_ent;
	const struct db_command *table;
	size_t		i, j, w, columns, lines, numcmds, width=0;
	const char	*p;

	TAILQ_FOREACH(list_ent,list,db_cmd_next) {
		table = list_ent->db_cmd;
		for (i = 0; table[i].name != NULL; i++) {
			w = strlen(table[i].name);
			if (w > width)
				width = w;
		}
	}

	width = DB_NEXT_TAB(width);

	columns = db_max_width / width;
	if (columns == 0)
		columns = 1;

	TAILQ_FOREACH(list_ent,list,db_cmd_next) {
		table = list_ent->db_cmd;

		for (numcmds = 0; table[numcmds].name != NULL; numcmds++)
			;
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
				if (p) {
					w = strlen(p);
					while (w < width) {
						w = DB_NEXT_TAB(w);
						db_putchar('\t');
					}
				}
			}
		}
	}
	return;
}

/*
 * Read complete command with all subcommands, starting with current
 * db_tok_string. If subcommand is missing, print the list of all
 * subcommands.  If command/subcommand is not found, print an error
 * message.  Returns pointer to "leaf" command or NULL.
 */
static const struct db_command *
db_read_command(void)
{
	const struct db_command *command;
	struct db_cmd_tbl_en_head *list;
	int found;
	int t;

	list = &db_base_cmd_list;
	do {
		found = db_cmd_search(db_tok_string, list, &command);
		if (command == NULL) {
			db_cmd_search_failed(db_tok_string, found);
			db_flush_lex();
			return NULL;
		}

		if (command->flag == CS_SHOW)
			list = &db_show_cmd_list;
		else if (command->flag == CS_MACH)
			list = &db_mach_cmd_list;
		else if (command->flag == CS_COMPAT)
			/* same list */;
		else
			break; /* expect no more subcommands */

		t = db_read_token(); /* read subcommand */
		if (t != tIDENT) {
			/* if none given - just print all of them */
			db_cmd_list(list);
			db_flush_lex();
			return NULL;
		}
	} while (list != NULL);

	return command;
}

/*
 * Parse command line and execute apropriate function.
 */
static void
db_command(const struct db_command **last_cmdp)
{
	const struct db_command *command;
	static db_expr_t last_count = 0;
	db_expr_t	addr, count;
	char		modif[TOK_STRING_SIZE];
	
	int			t;
	bool		have_addr = false;

	command = NULL;
	
	t = db_read_token();
	if ((t == tEOL) || (t == tCOMMA)) {
		/*
		 * An empty line repeats last command, at 'next'.
		 * Only a count repeats the last command with the new count.
		 */
		command = *last_cmdp;

		if (!command)
			return;

		addr = (db_expr_t)db_next;
		if (t == tCOMMA) {
			if (!db_expression(&count)) {
				db_printf("Count missing\n");
				db_flush_lex();
				return;
			}
		} else
			count = last_count;
		have_addr = false;
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

		command = db_read_command();
		if (command == NULL)
			return;

		if ((command->flag & CS_OWN) == 0) {

			/*
			 * Standard syntax:
			 * command [/modifier] [addr] [,count]
			 */
			t = db_read_token(); /* get modifier */
			if (t == tSLASH) {
				t = db_read_token();
				if (t != tIDENT) {
					db_printf("Bad modifier\n");
					db_flush_lex();
					return;
				}
				/* save modifier */
				strlcpy(modif, db_tok_string, sizeof(modif));

			} else {
				db_unread_token(t);
				modif[0] = '\0';
			}

			if (db_expression(&addr)) { /*get address*/
				db_dot = (db_addr_t) addr;
				db_last_addr = db_dot;
				have_addr = true;
			} else {
				addr = (db_expr_t) db_dot;
				have_addr = false;
			}

			t = db_read_token();
			if (t == tCOMMA) { /*Get count*/
				if (!db_expression(&count)) {
					db_printf("Count missing\n");
					db_flush_lex();
					return;
				}
			} else {
				db_unread_token(t);
				count = -1;
			}
			if ((command->flag & CS_MORE) == 0) {
				db_skip_to_eol();
			}
		}
	}

	if (command != NULL && command->flag & CS_NOREPEAT) {
		*last_cmdp = NULL;
		last_count = 0;
	} else {
		*last_cmdp = command;
		last_count = count;
	}


	if (command != NULL) {
		/*
		 * Execute the command.
		 */
		if (command->fcn != NULL)
			(*command->fcn)(addr, have_addr, count, modif);

		if (command->flag & CS_SET_DOT) {
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

/*
 * Print help for commands
 */
static void
db_help_print_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
const char *modif)
{
	const struct db_command *command;
	int t;

	t = db_read_token();

	/* is there another command after the "help"? */
	if (t != tIDENT) {
		/* print base commands */
		db_cmd_list(&db_base_cmd_list);
		return;
	}

	command = db_read_command();
	if (command == NULL)
		return;

#ifdef DDB_VERBOSE_HELP
	db_printf("Command: %s\n", command->name);
	if (command->cmd_descr != NULL)
		db_printf(" Description: %s\n", command->cmd_descr);
	if (command->cmd_arg != NULL)
		db_printf(" Arguments: %s\n", command->cmd_arg);
	if (command->cmd_arg_help != NULL)
		db_printf(" Arguments description:\n%s\n",
			  command->cmd_arg_help);
	if ((command->cmd_arg == NULL) && (command->cmd_descr == NULL))
		db_printf(" No help message.\n");
#endif

	db_skip_to_eol();
}

/*ARGSUSED*/
static void
db_map_print_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	bool full = false;

	if (modif[0] == 'f')
		full = true;

	if (have_addr == false)
		addr = (db_expr_t)(uintptr_t)db_read_ptr("kernel_map");

#ifdef _KERNEL
	uvm_map_printit((struct vm_map *)(uintptr_t) addr, full, db_printf);
#endif	/* XXX CRASH(8) */
}

/*ARGSUSED*/
static void
db_object_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool full = false;

	if (modif[0] == 'f')
		full = true;

#ifdef _KERNEL /* XXX CRASH(8) */
	uvm_object_printit((struct uvm_object *)(uintptr_t) addr, full,
	    db_printf);
#endif
}

/*ARGSUSED*/
static void
db_page_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool full = false;

	if (modif[0] == 'f')
		full = true;

#ifdef _KERNEL /* XXX CRASH(8) */
	uvm_page_printit((struct vm_page *)(uintptr_t) addr, full, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_show_all_pages(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL /* XXX CRASH(8) */
	uvm_page_printall(db_printf);
#endif
}

/*ARGSUSED*/
static void
db_buf_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool full = false;

	if (modif[0] == 'f')
		full = true;

#ifdef _KERNEL /* XXX CRASH(8) */
	vfs_buf_print((struct buf *)(uintptr_t) addr, full, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_event_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool showzero = false;
	bool showall = true;
	bool showintr = false;
	bool showtrap = false;
	bool showmisc = false;
	struct evcnt ev, *evp;
	char buf[80];
	int i;

	i = 0;
	while (modif[i]) {
		switch (modif[i]) {
		case 'f':
			showzero = true;
			break;
		case 'i':
			showintr = true;
			showall = false;
			break;
		case 't':
			showtrap = true;
			showall = false;
			break;
		case 'm':
			showmisc = true;
			showall = false;
			break;
		}
		i++;
	}

	if (showall)
		showmisc = showintr = showtrap = true;

	evp = (struct evcnt *)db_read_ptr("allevents");
	while (evp != NULL) {
		db_read_bytes((db_addr_t)evp, sizeof(ev), (char *)&ev);
		evp = ev.ev_list.tqe_next;
		if (ev.ev_count == 0 && !showzero)
			continue;
		if (ev.ev_type == EVCNT_TYPE_INTR && !showintr)
			continue;
		if (ev.ev_type == EVCNT_TYPE_TRAP && !showtrap)
			continue;
		if (ev.ev_type == EVCNT_TYPE_MISC && !showmisc)
			continue;
		db_read_bytes((db_addr_t)ev.ev_group, ev.ev_grouplen + 1, buf);
		db_printf("evcnt type %d: %s ", ev.ev_type, buf);
		db_read_bytes((db_addr_t)ev.ev_name, ev.ev_namelen + 1, buf);
		db_printf("%s = %lld\n", buf, (long long)ev.ev_count);
	}
}

/*ARGSUSED*/
static void
db_vnode_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool full = false;

	if (modif[0] == 'f')
		full = true;

#ifdef _KERNEL /* XXX CRASH(8) */
	vfs_vnode_print((struct vnode *)(uintptr_t) addr, full, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_vmem_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL /* XXX CRASH(8) */
	vmem_print((uintptr_t) addr, modif, db_printf);
#endif
}

static void
db_mount_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
	bool full = false;

	if (modif[0] == 'f')
		full = true;

#ifdef _KERNEL	/* XXX CRASH(8) */
	vfs_mount_print((struct mount *)(uintptr_t) addr, full, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_mbuf_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL /* XXX CRASH(8) */
	m_print((const struct mbuf *)(uintptr_t) addr, modif, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_pool_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL /* XXX CRASH(8) */
	pool_printit((struct pool *)(uintptr_t) addr, modif, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_namecache_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL /* XXX CRASH(8) */
	namecache_print((struct vnode *)(uintptr_t) addr, db_printf);
#endif
}

/*ARGSUSED*/
static void
db_uvmexp_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL	/* XXX CRASH(8) */
	uvmexp_print(db_printf);
#endif
}

#ifdef KERNHIST
/*ARGSUSED*/
static void
db_kernhist_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

	kernhist_print(db_printf);
}
#endif

/*ARGSUSED*/
static void
db_lock_print_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL	/* XXX CRASH(8) */
	lockdebug_lock_print((void *)(uintptr_t)addr, db_printf);
#endif
}

/*
 * Call random function:
 * !expr(arg,arg,arg)
 */
/*ARGSUSED*/
static void
db_fncall(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
#ifdef _KERNEL
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
	func = (db_expr_t (*)(db_expr_t, ...))(uintptr_t) fn_addr;

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
#else	/* _KERNEL */
	db_printf("This command can only be used in-kernel.\n");
#endif	/* _KERNEL */
}

static void
db_reboot_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
#ifdef _KERNEL
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
	panicstr = "reboot forced via kernel debugger";
	cpu_reboot((int)bootflags, NULL);
#else	/* _KERNEL */
	db_printf("This command can only be used in-kernel.\n");
#endif	/* _KERNEL */
}

static void
db_sifting_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
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
db_stack_trace_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	register const char *cp = modif;
	register char c;
	void (*pr)(const char *, ...);

	pr = db_printf;
	while ((c = *cp++) != 0)
		if (c == 'l')
			pr = (void (*)(const char *, ...))printf;

	if (count == -1)
		count = 65535;

	db_stack_trace_print(addr, have_addr, count, modif, pr);
}

static void
db_sync_cmd(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{
#ifdef _KERNEL
	/*
	 * We are leaving DDB, never to return upward.
	 * Clear db_recover so that we can debug faults in functions
	 * called from cpu_reboot.
	 */
	db_recover = 0;
	panicstr = "dump forced via kernel debugger";
	cpu_reboot(RB_DUMP, NULL);
#else	/* _KERNEL */
	db_printf("This command can only be used in-kernel.\n");
#endif	/* _KERNEL */
}

/*
 * Describe what an address is
 */
void
db_whatis_cmd(db_expr_t address, bool have_addr,
    db_expr_t count, const char *modif)
{
	const uintptr_t addr = (uintptr_t)address;

	db_lwp_whatis(addr, db_printf);
#ifdef _KERNEL	/* XXX CRASH(8) */
	pool_whatis(addr, db_printf);
	vmem_whatis(addr, db_printf);
	uvm_whatis(addr, db_printf);
	module_whatis(addr, db_printf);
#endif
}
