/* Memory breakpoint interfaces for the remote server for GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef MEM_BREAK_H
#define MEM_BREAK_H

/* Breakpoints are opaque.  */
struct breakpoint;
struct fast_tracepoint_jump;

/* Locate a breakpoint placed at address WHERE and return a pointer
   to its structure.  */

struct breakpoint *find_gdb_breakpoint_at (CORE_ADDR where);

/* Create a new GDB breakpoint at WHERE.  Returns -1 if breakpoints
   are not supported on this target, 0 otherwise.  */

int set_gdb_breakpoint_at (CORE_ADDR where);

/* Returns TRUE if there's any breakpoint at ADDR in our tables,
   inserted, or not.  */

int breakpoint_here (CORE_ADDR addr);

/* Returns TRUE if there's any inserted breakpoint set at ADDR.  */

int breakpoint_inserted_here (CORE_ADDR addr);

/* Clear all breakpoint conditions associated with this address.  */

void clear_gdb_breakpoint_conditions (CORE_ADDR addr);

/* Set target-side condition CONDITION to the breakpoint at ADDR.  */

int add_breakpoint_condition (CORE_ADDR addr, char **condition);

int add_breakpoint_commands (CORE_ADDR addr, char **commands, int persist);

int any_persistent_commands (void);

/* Evaluation condition (if any) at breakpoint BP.  Return 1 if
   true and 0 otherwise.  */

int gdb_condition_true_at_breakpoint (CORE_ADDR where);

int gdb_no_commands_at_breakpoint (CORE_ADDR where);

void run_breakpoint_commands (CORE_ADDR where);

/* Returns TRUE if there's a GDB breakpoint set at ADDR.  */

int gdb_breakpoint_here (CORE_ADDR where);

/* Create a new breakpoint at WHERE, and call HANDLER when
   it is hit.  HANDLER should return 1 if the breakpoint
   should be deleted, 0 otherwise.  */

struct breakpoint *set_breakpoint_at (CORE_ADDR where,
				      int (*handler) (CORE_ADDR));

/* Delete a GDB breakpoint previously inserted at ADDR with
   set_gdb_breakpoint_at.  */

int delete_gdb_breakpoint_at (CORE_ADDR addr);

/* Delete a breakpoint.  */

int delete_breakpoint (struct breakpoint *bkpt);

/* Set a reinsert breakpoint at STOP_AT.  */

void set_reinsert_breakpoint (CORE_ADDR stop_at);

/* Delete all reinsert breakpoints.  */

void delete_reinsert_breakpoints (void);

/* Reinsert breakpoints at WHERE (and change their status to
   inserted).  */

void reinsert_breakpoints_at (CORE_ADDR where);

/* Uninsert breakpoints at WHERE (and change their status to
   uninserted).  This still leaves the breakpoints in the table.  */

void uninsert_breakpoints_at (CORE_ADDR where);

/* Reinsert all breakpoints of the current process (and change their
   status to inserted).  */

void reinsert_all_breakpoints (void);

/* Uninsert all breakpoints of the current process (and change their
   status to uninserted).  This still leaves the breakpoints in the
   table.  */

void uninsert_all_breakpoints (void);

/* See if any breakpoint claims ownership of STOP_PC.  Call the handler for
   the breakpoint, if found.  */

void check_breakpoints (CORE_ADDR stop_pc);

/* See if any breakpoints shadow the target memory area from MEM_ADDR
   to MEM_ADDR + MEM_LEN.  Update the data already read from the target
   (in BUF) if necessary.  */

void check_mem_read (CORE_ADDR mem_addr, unsigned char *buf, int mem_len);

/* See if any breakpoints shadow the target memory area from MEM_ADDR
   to MEM_ADDR + MEM_LEN.  Update the data to be written to the target
   (in BUF, a copy of MYADDR on entry) if necessary, as well as the
   original data for any breakpoints.  */

void check_mem_write (CORE_ADDR mem_addr,
		      unsigned char *buf, const unsigned char *myaddr, int mem_len);

/* Set the byte pattern to insert for memory breakpoints.  This function
   must be called before any breakpoints are set.  */

void set_breakpoint_data (const unsigned char *bp_data, int bp_len);

/* Delete all breakpoints.  */

void delete_all_breakpoints (void);

/* Clear the "inserted" flag in all breakpoints of PROC.  */

void mark_breakpoints_out (struct process_info *proc);

/* Delete all breakpoints, but do not try to un-insert them from the
   inferior.  */

void free_all_breakpoints (struct process_info *proc);

/* Check if breakpoints still seem to be inserted in the inferior.  */

void validate_breakpoints (void);

/* Insert a fast tracepoint jump at WHERE, using instruction INSN, of
   LENGTH bytes.  */

struct fast_tracepoint_jump *set_fast_tracepoint_jump (CORE_ADDR where,
						       unsigned char *insn,
						       ULONGEST length);

/* Increment reference counter of JP.  */
void inc_ref_fast_tracepoint_jump (struct fast_tracepoint_jump *jp);

/* Delete fast tracepoint jump TODEL from our tables, and uninsert if
   from memory.  */

int delete_fast_tracepoint_jump (struct fast_tracepoint_jump *todel);

/* Returns true if there's fast tracepoint jump set at WHERE.  */

int fast_tracepoint_jump_here (CORE_ADDR);

/* Uninsert fast tracepoint jumps at WHERE (and change their status to
   uninserted).  This still leaves the tracepoints in the table.  */

void uninsert_fast_tracepoint_jumps_at (CORE_ADDR pc);

/* Reinsert fast tracepoint jumps at WHERE (and change their status to
   inserted).  */

void reinsert_fast_tracepoint_jumps_at (CORE_ADDR where);

#endif /* MEM_BREAK_H */
