/*	$NetBSD: machine.h,v 1.5 2001/05/22 15:38:22 christos Exp $	*/

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
#ifdef ORDER
    char **order_names;
#endif
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
    int    p_total;
    int    P_ACTIVE;     /* number of procs considered "active" */
    int    *procstates;
    int    *cpustates;
    int    *memory;
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
    char *command;	/* only this command (unless == NULL) */
};

/* routines defined by the machine dependent module */

struct proc;
extern int (*proc_compares[]) __P((struct proc **, struct proc **));

int machine_init __P((struct statics *));
char *format_header __P((char *));
void get_system_info __P((struct system_info *));
caddr_t get_process_info __P((struct system_info *, struct process_select *,
    int (*)(struct proc **, struct proc **)));
char *format_next_process __P((caddr_t, char *(*)(int)));
int proc_owner __P((int));
