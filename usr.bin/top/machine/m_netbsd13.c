/*	$NetBSD: m_netbsd13.c,v 1.9 1999/11/05 07:25:14 lukem Exp $	*/

/*
 * top - a top users display for Unix
 *
 * SYNOPSIS:  For a NetBSD-1.3 (or later) system
 *
 * DESCRIPTION:
 * Originally written for BSD4.4 system by Christos Zoulas.
 * Based on the FreeBSD 2.0 version by Steven Wallace and Wolfram Schneider.
 * NetBSD-1.0 port by Arne Helme. Process ordering by Luke Mewburn.
 * NetBSD-1.3 port by Luke Mewburn, based on code by Matthew Green.
 * NetBSD-1.4/UVM port by matthew green.
 * -
 * This is the machine-dependent module for NetBSD-1.3 and later
 * works for:
 *	NetBSD-1.3
 *	NetBSD-1.3.1
 *	NetBSD-1.3.2
 *	NetBSD-1.3.3
 *	NetBSD-1.3.4 (beta)
 * and should work for:
 *	NetBSD-1.4   (when released)
 *
 * LIBS: -lkvm
 *
 * CFLAGS: -DHAVE_GETOPT -DORDER -DHAVE_STRERROR `printf ".include <bsd.own.mk>\nxxx:\n.if defined(UVM)\n\techo -DUVM\n.endif\n" | make -s -f-`
 *
 * AUTHORS:	Christos Zoulas <christos@ee.cornell.edu>
 *		Steven Wallace <swallace@freebsd.org>
 *		Wolfram Schneider <wosch@cs.tu-berlin.de>
 *		Arne Helme <arne@acm.org>
 *		Luke Mewburn <lukem@netbsd.org>
 *		matthew green <mrg@eterna.com.au>
 *
 *
 * $Id: m_netbsd13.c,v 1.9 1999/11/05 07:25:14 lukem Exp $
 */
#define UVM

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/sysctl.h>
#include <sys/dir.h>
#include <sys/dkstat.h>
#include <sys/file.h>
#include <sys/time.h>

#include <vm/vm_swap.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include "os.h"
#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <math.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int check_nlist __P((struct nlist *));
static int getkval __P((unsigned long, int *, int, char *));
extern char* printable __P((char *));

#include "top.h"
#include "machine.h"
#include "utils.h"


/* get_process_info passes back a handle.  This is what it looks like: */

struct handle
{
    struct kinfo_proc **next_proc;	/* points to next valid proc pointer */
    int remaining;		/* number of pointers remaining */
};

/* declarations for load_avg */
#include "loadavg.h"

#define PP(pp, field) ((pp)->kp_proc . field)
#define EP(pp, field) ((pp)->kp_eproc . field)
#define VP(pp, field) ((pp)->kp_eproc.e_vm . field)

/* define what weighted cpu is.  */
#define weighted_cpu(pct, pp) (PP((pp), p_swtime) == 0 ? 0.0 : \
			 ((pct) / (1.0 - exp(PP((pp), p_swtime) * logcpu))))

/* what we consider to be process size: */
#define PROCSIZE(pp) \
	(VP((pp), vm_tsize) + VP((pp), vm_dsize) + VP((pp), vm_ssize))

/* definitions for indices in the nlist array */


static struct nlist nlst[] = {
#define X_CCPU		0
    { "_ccpu" },		/* 0 */
#define X_CP_TIME	1
    { "_cp_time" },		/* 1 */
#define X_HZ		2
    { "_hz" },		        /* 2 */
#define X_STATHZ	3
    { "_stathz" },		/* 3 */
#define X_AVENRUN	4
    { "_averunnable" },		/* 4 */
#if !defined(UVM)
#define X_CNT		5
    { "_cnt" },
#endif

    { 0 }
};

/*
 *  These definitions control the format of the per-process area
 */

static char header[] =
  "  PID X        PRI NICE   SIZE   RES STATE   TIME   WCPU    CPU COMMAND";
/* 0123456   -- field to fill in starts at header+6 */
#define UNAME_START 6

#define Proc_format \
	"%5d %-8.8s %3d %4d%7s %5s %-5s%7s %5.2f%% %5.2f%% %.14s"


/* process state names for the "STATE" column of the display */
/* the extra nulls in the string "run" are for adding a slash and
   the processor number when needed */

char *state_abbrev[] =
{
    "", "start", "run\0\0\0", "sleep", "stop", "zomb"
#ifdef SDEAD
    , "dead"
#endif
};

static kvm_t *kd;

/* values that we stash away in _init and use in later routines */

static double logcpu;

/* these are retrieved from the kernel in _init */

static int hz;
static int ccpu;

/* these are offsets obtained via nlist and used in the get_ functions */

static unsigned long cp_time_offset;
static unsigned long avenrun_offset;
#if !defined(UVM)
static unsigned long cnt_offset;
#endif
/* these are for calculating cpu state percentages */

static long cp_time[CPUSTATES];
static long cp_old[CPUSTATES];
static long cp_diff[CPUSTATES];

/* these are for detailing the process states */

int process_states[7];
char *procstatenames[] = {
    "", " starting, ", " running, ", " sleeping, ", " stopped, ",
    " zombie, ", " ABANDONED, ",
    NULL
};

/* these are for detailing the cpu states */

int cpu_states[CPUSTATES];
char *cpustatenames[] = {
    "user", "nice", "system", "interrupt", "idle", NULL
};

/* these are for detailing the memory statistics */

int memory_stats[7];
char *memorynames[] = {
    "K Act, ", "K Inact, ", "K Wired, ", "K Free, ",
    "K Swap, ", "K Swap free, ",
    NULL
};


/* these are names given to allowed sorting orders -- first is default */
char *ordernames[] = {
    "cpu",
    "pri",
    "res",
    "size",
    "state",
    "time",
    NULL
};

/* forward definitions for comparison functions */
int (*proc_compares[]) __P((struct proc **, struct proc **)) = {
    compare_cpu,
    compare_prio,
    compare_res,
    compare_size,
    compare_state,
    compare_time,
    NULL
};


/* these are for keeping track of the proc array */

static int nproc;
static int onproc = -1;
static int pref_len;
static struct kinfo_proc *pbase;
static struct kinfo_proc **pref;

/* these are for getting the memory statistics */

static int pageshift;		/* log base 2 of the pagesize */

/* define pagetok in terms of pageshift */

#define pagetok(size) ((size) << pageshift)

int
machine_init(statics)
    struct statics *statics;
{
    int i = 0;
    int pagesize;

    if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
	return -1;


    /* get the list of symbols we want to access in the kernel */
    (void) kvm_nlist(kd, nlst);
    if (nlst[0].n_type == 0)
    {
	fprintf(stderr, "top: nlist failed\n");
	return(-1);
    }

    /* make sure they were all found */
    if (i > 0 && check_nlist(nlst) > 0)
    {
	return(-1);
    }

    /* get the symbol values out of kmem */
    (void) getkval(nlst[X_STATHZ].n_value, (int *)(&hz), sizeof(hz), "!");
    if (!hz) {
	(void) getkval(nlst[X_HZ].n_value, (int *)(&hz), sizeof(hz),
		       nlst[X_HZ].n_name);
    }


    (void) getkval(nlst[X_CCPU].n_value,   (int *)(&ccpu),	sizeof(ccpu),
	    nlst[X_CCPU].n_name);

    /* stash away certain offsets for later use */
    cp_time_offset = nlst[X_CP_TIME].n_value;
    avenrun_offset = nlst[X_AVENRUN].n_value;
#if !defined(UVM)
    cnt_offset = nlst[X_CNT].n_value;
#endif

    /* this is used in calculating WCPU -- calculate it ahead of time */
    logcpu = log(loaddouble(ccpu));

    pbase = NULL;
    pref = NULL;
    nproc = 0;
    onproc = -1;
    /* get the page size with "getpagesize" and calculate pageshift from it */
    pagesize = getpagesize();
    pageshift = 0;
    while (pagesize > 1)
    {
	pageshift++;
	pagesize >>= 1;
    }

    /* we only need the amount of log(2)1024 for our conversion */
    pageshift -= LOG1024;

    /* fill in the statics information */
    statics->procstate_names = procstatenames;
    statics->cpustate_names = cpustatenames;
    statics->memory_names = memorynames;
    statics->order_names = ordernames;

    /* all done! */
    return(0);
}

char *
format_header(uname_field)
    char *uname_field;
{
    char *ptr;

    ptr = header + UNAME_START;
    while (*uname_field != '\0')
    {
	*ptr++ = *uname_field++;
    }

    return(header);
}

void
get_system_info(si)
    struct system_info *si;
{
    long    total;
#if defined(UVM)
    size_t  usize;
    int     mib[2];
    struct  uvmexp uvmexp;
#else
    struct  vmmeter sum;
#endif
    struct  swapent *sep, *seporig;
    int     totalsize, size, totalinuse, inuse, ncounted;
    int     rnswap, nswap;

    /* get the cp_time array */
    (void) getkval(cp_time_offset, (int *)cp_time, sizeof(cp_time),
		   nlst[X_CP_TIME].n_name);

    if (getloadavg(si->load_avg, NUM_AVERAGES) < 0) {
	int i;

	warn("can't getloadavg");
	for (i = 0; i < NUM_AVERAGES; i++)
	    si->load_avg[i] = 0.0;
    }

    /* convert cp_time counts to percentages */
    total = percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

#if defined(UVM)
    mib[0] = CTL_VM;
    mib[1] = VM_UVMEXP;
    usize = sizeof(uvmexp);
    if (sysctl(mib, 2, &uvmexp, &usize, NULL, 0) < 0) {
	fprintf(stderr, "top: sysctl vm.uvmexp failed: %s\n",
	    strerror(errno));
	quit(23);
    }
    
    /* convert memory stats to Kbytes */
    memory_stats[0] = pagetok(uvmexp.active);
    memory_stats[1] = pagetok(uvmexp.inactive);
    memory_stats[2] = pagetok(uvmexp.wired);
    memory_stats[3] = pagetok(uvmexp.free);
#else
    /* sum memory statistics */
    (void) getkval(cnt_offset, (int *)(&sum), sizeof(sum), "_cnt");

    /* convert memory stats to Kbytes */
    memory_stats[0] = pagetok(sum.v_active_count);
    memory_stats[1] = pagetok(sum.v_inactive_count);
    memory_stats[2] = pagetok(sum.v_wire_count);
    memory_stats[3] = pagetok(sum.v_free_count);
#endif

    memory_stats[4] = memory_stats[5] = 0;

    seporig = NULL;
    do {
	nswap = swapctl(SWAP_NSWAP, 0, 0);
	if (nswap < 1)
		break;
	/*  Use seporig to keep track of the malloc'd memory
	 *  base, as sep will be incremented in the for loop
	 *  below.  */
	seporig = sep = (struct swapent *)malloc(nswap * sizeof(*sep));
	if (sep == NULL)
		break;
	rnswap = swapctl(SWAP_STATS, (void *)sep, nswap);
	if (nswap != rnswap)
		break;

	totalsize = totalinuse = ncounted = 0;
	for (; rnswap-- > 0; sep++) {
	    ncounted++;
	    size = sep->se_nblks;
	    inuse = sep->se_inuse;
	    totalsize += size;
	    totalinuse += inuse;
	}
	memory_stats[4] = dbtob(totalinuse) / 1024;
	memory_stats[5] = dbtob(totalsize) / 1024 - memory_stats[4];
	/*  Free here, before we malloc again in the next
	 *  iteration of this loop.  */
	if (seporig) {
		free(seporig);
		seporig = NULL;
	}
    } while (0);
    /*  Catch the case where we malloc'd, but then exited the
     *  loop due to nswap != rnswap.  */
    if (seporig)
	    free(seporig);

    memory_stats[6] = -1;

    /* set arrays and strings */
    si->cpustates = cpu_states;
    si->memory = memory_stats;
    si->last_pid = -1;
}

static struct handle handle;

caddr_t
get_process_info(si, sel, compare)
    struct system_info *si;
    struct process_select *sel;
    int (*compare) __P((struct proc **, struct proc **));
{
    int i;
    int total_procs;
    int active_procs;
    struct kinfo_proc **prefp;
    struct kinfo_proc *pp;

    /* these are copied out of sel for speed */
    int show_idle;
    int show_system;
    int show_uid;
    int show_command;

    
    pbase = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nproc);
    if (nproc > onproc)
	pref = (struct kinfo_proc **) realloc(pref, sizeof(struct kinfo_proc *)
		* (onproc = nproc));
    if (pref == NULL || pbase == NULL) {
	(void) fprintf(stderr, "top: Out of memory.\n");
	quit(23);
    }
    /* get a pointer to the states summary array */
    si->procstates = process_states;

    /* set up flags which define what we are going to select */
    show_idle = sel->idle;
    show_system = sel->system;
    show_uid = sel->uid != -1;
    show_command = sel->command != NULL;

    /* count up process states and get pointers to interesting procs */
    total_procs = 0;
    active_procs = 0;
    memset((char *)process_states, 0, sizeof(process_states));
    prefp = pref;
    for (pp = pbase, i = 0; i < nproc; pp++, i++)
    {
	/*
	 *  Place pointers to each valid proc structure in pref[].
	 *  Process slots that are actually in use have a non-zero
	 *  status field.  Processes with P_SYSTEM set are system
	 *  processes---these get ignored unless show_sysprocs is set.
	 */
	if (PP(pp, p_stat) != 0 &&
	    (show_system || ((PP(pp, p_flag) & P_SYSTEM) == 0)))
	{
	    total_procs++;
	    process_states[(unsigned char) PP(pp, p_stat)]++;
	    if (PP(pp, p_stat) != SZOMB &&
#ifdef SDEAD
		PP(pp, p_stat) != SDEAD &&
#endif
		(show_idle || (PP(pp, p_pctcpu) != 0) || 
		 (PP(pp, p_stat) == SRUN)) &&
		(!show_uid || EP(pp, e_pcred.p_ruid) == (uid_t)sel->uid))
	    {
		*prefp++ = pp;
		active_procs++;
	    }
	}
    }

    /* if requested, sort the "interesting" processes */
    if (compare != NULL)
    {
	qsort((char *)pref, active_procs, sizeof(struct kinfo_proc *), 
	    (int (*) __P((const void *, const void *)))compare);
    }

    /* remember active and total counts */
    si->p_total = total_procs;
    si->p_active = pref_len = active_procs;

    /* pass back a handle */
    handle.next_proc = pref;
    handle.remaining = active_procs;
    return((caddr_t)&handle);
}

char fmt[128];		/* static area where result is built */

char *
format_next_process(handle, get_userid)
    caddr_t handle;
    char *(*get_userid) __P((int));
{
    struct kinfo_proc *pp;
    long cputime;
    double pct;
    struct handle *hp;

    /* find and remember the next proc structure */
    hp = (struct handle *)handle;
    pp = *(hp->next_proc++);
    hp->remaining--;
    

    /* get the process's user struct and set cputime */
    if ((PP(pp, p_flag) & P_INMEM) == 0) {
	/*
	 * Print swapped processes as <pname>
	 */
	char *comm = PP(pp, p_comm);
#define COMSIZ sizeof(PP(pp, p_comm))
	char buf[COMSIZ];
	(void) strncpy(buf, comm, COMSIZ);
	comm[0] = '<';
	(void) strncpy(&comm[1], buf, COMSIZ - 2);
	comm[COMSIZ - 2] = '\0';
	(void) strncat(comm, ">", COMSIZ - 1);
	comm[COMSIZ - 1] = '\0';
    }

#if 0
    /* This does not produce the correct results */
    cputime = PP(pp, p_uticks) + PP(pp, p_sticks) + PP(pp, p_iticks);
#endif
    cputime = PP(pp, p_rtime).tv_sec;	/* This does not count interrupts */

    /* calculate the base for cpu percentages */
    pct = pctdouble(PP(pp, p_pctcpu));

#define Proc_format \
	"%5d %-8.8s %3d %4d%7s %5s %-5s%7s %5.2f%% %5.2f%% %.14s"

    /* format this entry */
    sprintf(fmt,
	    Proc_format,
	    PP(pp, p_pid),
	    (*get_userid)(EP(pp, e_pcred.p_ruid)),
	    PP(pp, p_priority) - PZERO,
	    PP(pp, p_nice) - NZERO,
	    format_k(pagetok(PROCSIZE(pp))),
	    format_k(pagetok(VP(pp, vm_rssize))),
	    state_abbrev[(unsigned char) PP(pp, p_stat)],
	    format_time(cputime),
	    100.0 * weighted_cpu(pct, pp),
	    100.0 * pct,
	    printable(PP(pp, p_comm)));

    /* return the result */
    return(fmt);
}


/*
 * check_nlist(nlst) - checks the nlist to see if any symbols were not
 *		found.  For every symbol that was not found, a one-line
 *		message is printed to stderr.  The routine returns the
 *		number of symbols NOT found.
 */

static int
check_nlist(nlst)
    struct nlist *nlst;
{
    int i;

    /* check to see if we got ALL the symbols we requested */
    /* this will write one line to stderr for every symbol not found */

    i = 0;
    while (nlst->n_name != NULL)
    {
	if (nlst->n_type == 0)
	{
	    /* this one wasn't found */
	    (void) fprintf(stderr, "kernel: no symbol named `%s'\n",
			   nlst->n_name);
	    i = 1;
	}
	nlst++;
    }

    return(i);
}


/*
 *  getkval(offset, ptr, size, refstr) - get a value out of the kernel.
 *	"offset" is the byte offset into the kernel for the desired value,
 *  	"ptr" points to a buffer into which the value is retrieved,
 *  	"size" is the size of the buffer (and the object to retrieve),
 *  	"refstr" is a reference string used when printing error meessages,
 *	    if "refstr" starts with a '!', then a failure on read will not
 *  	    be fatal (this may seem like a silly way to do things, but I
 *  	    really didn't want the overhead of another argument).
 *  	
 */

static int
getkval(offset, ptr, size, refstr)
    unsigned long offset;
    int *ptr;
    int size;
    char *refstr;
{
    if (kvm_read(kd, offset, (char *) ptr, size) != size)
    {
	if (*refstr == '!')
	{
	    return(0);
	}
	else
	{
	    fprintf(stderr, "top: kvm_read for %s: %s\n",
		refstr, strerror(errno));
	    quit(23);
	}
    }
    return(1);
}
    
/* comparison routines for qsort */

/*
 * There are currently four possible comparison routines.  main selects
 * one of these by indexing in to the array proc_compares.
 *
 * Possible keys are defined as macros below.  Currently these keys are
 * defined:  percent cpu, cpu ticks, process state, resident set size,
 * total virtual memory usage.  The process states are ordered as follows
 * (from least to most important):  WAIT, zombie, sleep, stop, start, run.
 * The array declaration below maps a process state index into a number
 * that reflects this ordering.
 */

/*
 * First, the possible comparison keys.  These are defined in such a way
 * that they can be merely listed in the source code to define the actual
 * desired ordering.
 */

#define ORDERKEY_PCTCPU \
    if (lresult = (pctcpu)PP(p2, p_pctcpu) - (pctcpu)PP(p1, p_pctcpu),\
	(result = lresult > 0 ? 1 : lresult < 0 ? -1 : 0) == 0)

#define ORDERKEY_CPTICKS \
    if (lresult = (pctcpu)PP(p2, p_rtime).tv_sec \
		- (pctcpu)PP(p1, p_rtime).tv_sec,\
	(result = lresult > 0 ? 1 : lresult < 0 ? -1 : 0) == 0)

#define ORDERKEY_STATE \
    if ((result = sorted_state[(int)PP(p2, p_stat)] - \
		  sorted_state[(int)PP(p1, p_stat)] ) == 0)

#define ORDERKEY_PRIO \
    if ((result = PP(p2, p_priority) - PP(p1, p_priority)) == 0)

#define ORDERKEY_RSSIZE \
    if ((result = VP(p2, vm_rssize) - VP(p1, vm_rssize)) == 0)

#define ORDERKEY_MEM	\
    if ((result = (PROCSIZE(p2) - PROCSIZE(p1))) == 0)

/*
 * Now the array that maps process state to a weight.
 * The order of the elements should match those in state_abbrev[]
 */

static int sorted_state[] = {
    0,	/*  (not used)	  ?	*/
    5,	/* "start"	SIDL	*/
    4,	/* "run"	SRUN	*/
    3,	/* "sleep"	SSLEEP	*/
    3,	/* "stop"	SSTOP	*/
#ifdef SDEAD
    2,	/* "dead"	SDEAD	*/
#endif
    1,	/* "zomb"	SZOMB	*/

};

/* compare_cpu - the comparison function for sorting by cpu percentage */

int
compare_cpu(pp1, pp2)
    struct proc **pp1, **pp2;
{
    struct kinfo_proc *p1;
    struct kinfo_proc *p2;
    int result;
    pctcpu lresult;

    /* remove one level of indirection */
    p1 = *(struct kinfo_proc **) pp1;
    p2 = *(struct kinfo_proc **) pp2;

    ORDERKEY_PCTCPU
    ORDERKEY_CPTICKS
    ORDERKEY_STATE
    ORDERKEY_PRIO
    ORDERKEY_RSSIZE
    ORDERKEY_MEM
    ;

    return (result);
}

/* compare_prio - the comparison function for sorting by process priority */

int
compare_prio(pp1, pp2)
    struct proc **pp1, **pp2;
{
    struct kinfo_proc *p1;
    struct kinfo_proc *p2;
    int result;
    pctcpu lresult;

    /* remove one level of indirection */
    p1 = *(struct kinfo_proc **) pp1;
    p2 = *(struct kinfo_proc **) pp2;

    ORDERKEY_PRIO
    ORDERKEY_PCTCPU
    ORDERKEY_CPTICKS
    ORDERKEY_STATE
    ORDERKEY_RSSIZE
    ORDERKEY_MEM
    ;

    return (result);
}

/* compare_res - the comparison function for sorting by resident set size */

int
compare_res(pp1, pp2)
    struct proc **pp1, **pp2;
{
    struct kinfo_proc *p1;
    struct kinfo_proc *p2;
    int result;
    pctcpu lresult;

    /* remove one level of indirection */
    p1 = *(struct kinfo_proc **) pp1;
    p2 = *(struct kinfo_proc **) pp2;

    ORDERKEY_RSSIZE
    ORDERKEY_MEM
    ORDERKEY_PCTCPU
    ORDERKEY_CPTICKS
    ORDERKEY_STATE
    ORDERKEY_PRIO
    ;

    return (result);
}

/* compare_size - the comparison function for sorting by total memory usage */

int
compare_size(pp1, pp2)
    struct proc **pp1, **pp2;
{
    struct kinfo_proc *p1;
    struct kinfo_proc *p2;
    int result;
    pctcpu lresult;

    /* remove one level of indirection */
    p1 = *(struct kinfo_proc **) pp1;
    p2 = *(struct kinfo_proc **) pp2;

    ORDERKEY_MEM
    ORDERKEY_RSSIZE
    ORDERKEY_PCTCPU
    ORDERKEY_CPTICKS
    ORDERKEY_STATE
    ORDERKEY_PRIO
    ;

    return (result);
}

/* compare_state - the comparison function for sorting by process state */

int
compare_state(pp1, pp2)
    struct proc **pp1, **pp2;
{
    struct kinfo_proc *p1;
    struct kinfo_proc *p2;
    int result;
    pctcpu lresult;

    /* remove one level of indirection */
    p1 = *(struct kinfo_proc **) pp1;
    p2 = *(struct kinfo_proc **) pp2;

    ORDERKEY_STATE
    ORDERKEY_PCTCPU
    ORDERKEY_CPTICKS
    ORDERKEY_PRIO
    ORDERKEY_RSSIZE
    ORDERKEY_MEM
    ;

    return (result);
}

/* compare_time - the comparison function for sorting by total cpu time */

int
compare_time(pp1, pp2)
    struct proc **pp1, **pp2;
{
    struct kinfo_proc *p1;
    struct kinfo_proc *p2;
    int result;
    pctcpu lresult;

    /* remove one level of indirection */
    p1 = *(struct kinfo_proc **) pp1;
    p2 = *(struct kinfo_proc **) pp2;

    ORDERKEY_CPTICKS
    ORDERKEY_PCTCPU
    ORDERKEY_STATE
    ORDERKEY_PRIO
    ORDERKEY_MEM
    ORDERKEY_RSSIZE
    ;

    return (result);
}


/*
 * proc_owner(pid) - returns the uid that owns process "pid", or -1 if
 *		the process does not exist.
 *		It is EXTREMLY IMPORTANT that this function work correctly.
 *		If top runs setuid root (as in SVR4), then this function
 *		is the only thing that stands in the way of a serious
 *		security problem.  It validates requests for the "kill"
 *		and "renice" commands.
 */

int
proc_owner(pid)
    int pid;
{
    int cnt;
    struct kinfo_proc **prefp;
    struct kinfo_proc *pp;

    prefp = pref;
    cnt = pref_len;
    while (--cnt >= 0)
    {
	pp = *prefp++;	
	if (PP(pp, p_pid) == (pid_t)pid)
	{
	    return((int)EP(pp, e_pcred.p_ruid));
	}
    }
    return(-1);
}
