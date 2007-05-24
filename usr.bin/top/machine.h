/*	$NetBSD: machine.h,v 1.10 2007/05/24 20:04:04 ad Exp $	*/

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 * Copyright (c) 1984, 1989, William LeFebvre, Rice University
 * Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS EMPLOYER BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  This file defines the interface between top and the machine-dependent
 *  module.  It is NOT machine dependent and should not need to be changed
 *  for any specific machine.
 */

/*
 * the statics struct is filled in by machine_init
 */
struct statics
{
    char **procstate_names;
    char **cpustate_names;
    char **memory_names;
    char **swap_names;
#ifdef ORDER
    char **order_names;
#endif
    int ncpu;
};

/*
 * the system_info struct is filled in by a machine dependent routine.
 */

#ifdef p_active     /* uw7 define macro p_active */
#define P_ACTIVE p_pactive
#else
#define P_ACTIVE p_active
#endif

struct system_info
{
    int    last_pid;
    double load_avg[NUM_AVERAGES];
    time_t uptime;
    int    p_total;
    int    P_ACTIVE;     /* number of procs considered "active" */
    int    *procstates;
    int    *cpustates;
    int    *memory;
    int    *swap;
};

/* cpu_states is an array of percentages * 10.  For example, 
   the (integer) value 105 is 10.5% (or .105).
 */

/*
 * the process_select struct tells get_process_info what processes we
 * are interested in seeing
 */

struct process_select
{
    int idle;		/* show idle processes */
    int system;		/* show system processes */
    int uid;		/* only this uid (unless uid == -1) */
    int threads;	/* show threads */
    int pid;		/* show only this pid */
    char *command;	/* only this command (unless == NULL) */
};

/* routines defined by the machine dependent module */

struct proc;
extern int (*proc_compares[]) __P((struct proc **, struct proc **));

int machine_init __P((struct statics *));
char *format_header __P((char *, struct process_select *));
void get_system_info __P((struct system_info *));
caddr_t get_process_info __P((struct system_info *, struct process_select *,
    int (*)(struct proc **, struct proc **)));
char *format_next_process __P((caddr_t, char *(*)(int)));
int proc_owner __P((int));
