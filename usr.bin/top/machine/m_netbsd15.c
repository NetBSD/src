/*	$NetBSD: m_netbsd15.c,v 1.19 2003/07/26 20:34:14 salo Exp $	*/

/*
 * top - a top users display for Unix
 *
 * SYNOPSIS:  For a NetBSD-1.5 (or later) system
 *
 * DESCRIPTION:
 * Originally written for BSD4.4 system by Christos Zoulas.
 * Based on the FreeBSD 2.0 version by Steven Wallace and Wolfram Schneider.
 * NetBSD-1.0 port by Arne Helme. Process ordering by Luke Mewburn.
 * NetBSD-1.3 port by Luke Mewburn, based on code by Matthew Green.
 * NetBSD-1.4/UVM port by matthew green.
 * NetBSD-1.5 port by Simon Burge.
 * NetBSD-1.6/UBC port by Tomas Svensson.
 * -
 * This is the machine-dependent module for NetBSD-1.5 and later
 * works for:
 *	NetBSD-1.5ZC
 * and should work for:
 *	NetBSD-1.6	(when released)
 * -
 * top does not need to be installed setuid or setgid with this module.
 *
 * LIBS: -lkvm
 *
 * CFLAGS: -DHAVE_GETOPT -DORDER -DHAVE_STRERROR
 *
 * AUTHORS:	Christos Zoulas <christos@ee.cornell.edu>
 *		Steven Wallace <swallace@freebsd.org>
 *		Wolfram Schneider <wosch@cs.tu-berlin.de>
 *		Arne Helme <arne@acm.org>
 *		Luke Mewburn <lukem@NetBSD.org>
 *		matthew green <mrg@eterna.com.au>
 *		Simon Burge <simonb@NetBSD.org>
 *		Tomas Svensson <ts@unix1.net>
 *
 *
 * $Id: m_netbsd15.c,v 1.19 2003/07/26 20:34:14 salo Exp $
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: m_netbsd15.c,v 1.19 2003/07/26 20:34:14 salo Exp $");
#endif

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/sched.h>
#include <sys/swap.h>

#include <uvm/uvm_extern.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <math.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "os.h"
#include "top.h"
#include "machine.h"
#include "utils.h"
#include "display.h"
#include "loadavg.h"

void percentages64 __P((int, int *, u_int64_t *, u_int64_t *, u_int64_t *));


/* get_process_info passes back a handle.  This is what it looks like: */

struct handle {
	struct kinfo_proc2 **next_proc;	/* points to next valid proc pointer */
	int remaining;		/* number of pointers remaining */
};

/* define what weighted cpu is. */
#define weighted_cpu(pct, pp) ((pp)->p_swtime == 0 ? 0.0 : \
			 ((pct) / (1.0 - exp((pp)->p_swtime * logcpu))))

/* what we consider to be process size: */
#define PROCSIZE(pp) \
	((pp)->p_vm_tsize + (pp)->p_vm_dsize + (pp)->p_vm_ssize)


/*
 * These definitions control the format of the per-process area
 */

static char header[] =
  "  PID X        PRI NICE   SIZE   RES STATE      TIME   WCPU    CPU COMMAND";
/* 0123456   -- field to fill in starts at header+6 */
#define UNAME_START 6

#define Proc_format \
	"%5d %-8.8s %3d %4d%7s %5s %-8.8s%7s %5.2f%% %5.2f%% %.12s"


/* 
 * Process state names for the "STATE" column of the display.
 */

const char *state_abbrev[] = {
	"", "START", "RUN", "SLEEP", "STOP", "ZOMB", "DEAD", "CPU"
};

static kvm_t *kd;

/* these are retrieved from the kernel in _init */

static double logcpu;
static int hz;
static int ccpu;

/* these are for calculating cpu state percentages */

static u_int64_t cp_time[CPUSTATES];
static u_int64_t cp_old[CPUSTATES];
static u_int64_t cp_diff[CPUSTATES];

/* these are for detailing the process states */

int process_states[8];
char *procstatenames[] = {
	"", " starting, ", " runnable, ", " sleeping, ", " stopped, ",
	" zombie, ", " dead, ", " on processor, ",
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
	"K Act, ", "K Inact, ", "K Wired, ", "K Exec, ", "K File, ",
	"K Free, ",
	NULL
};

int swap_stats[4];
char *swapnames[] = {
	"K Total, ", "K Used, ", "K Free, ",
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
static int compare_cpu __P((struct proc **, struct proc **));
static int compare_prio __P((struct proc **, struct proc **));
static int compare_res __P((struct proc **, struct proc **));
static int compare_size __P((struct proc **, struct proc **));
static int compare_state __P((struct proc **, struct proc **));
static int compare_time __P((struct proc **, struct proc **));

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
static struct kinfo_proc2 *pbase;
static struct kinfo_proc2 **pref;

/* these are for getting the memory statistics */

static int pageshift;		/* log base 2 of the pagesize */

/* define pagetok in terms of pageshift */

#define pagetok(size) ((size) << pageshift)

int
machine_init(statics)
	struct statics *statics;
{
	int pagesize;
	int mib[2];
	size_t size;
	struct clockinfo clockinfo;

	if ((kd = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open")) == NULL)
		return -1;

	mib[0] = CTL_KERN;
	mib[1] = KERN_CCPU;
	size = sizeof(ccpu);
	if (sysctl(mib, 2, &ccpu, &size, NULL, 0) == -1) {
		fprintf(stderr, "top: sysctl kern.ccpu failed: %s\n",
		    strerror(errno));
		return(-1);
	}

	mib[0] = CTL_KERN;
	mib[1] = KERN_CLOCKRATE;
	size = sizeof(clockinfo);
	if (sysctl(mib, 2, &clockinfo, &size, NULL, 0) == -1) {
		fprintf(stderr, "top: sysctl kern.clockrate failed: %s\n",
		    strerror(errno));
		return(-1);
	}
	hz = clockinfo.stathz;

	/* this is used in calculating WCPU -- calculate it ahead of time */
	logcpu = log(loaddouble(ccpu));

	pbase = NULL;
	pref = NULL;
	nproc = 0;
	onproc = -1;
	/* get the page size with "getpagesize" and calculate pageshift from it */
	pagesize = getpagesize();
	pageshift = 0;
	while (pagesize > 1) {
		pageshift++;
		pagesize >>= 1;
	}

	/* we only need the amount of log(2)1024 for our conversion */
	pageshift -= LOG1024;

	/* fill in the statics information */
	statics->procstate_names = procstatenames;
	statics->cpustate_names = cpustatenames;
	statics->memory_names = memorynames;
	statics->swap_names = swapnames;
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
	while (*uname_field != '\0') {
		*ptr++ = *uname_field++;
	}

	return(header);
}

void
get_system_info(si)
	struct system_info *si;
{
	size_t ssize;
	int mib[2];
	struct uvmexp_sysctl uvmexp;
	struct swapent *sep, *seporig;
	u_int64_t totalsize, totalinuse;
	int size, inuse, ncounted;
	int rnswap, nswap;

	mib[0] = CTL_KERN;
	mib[1] = KERN_CP_TIME;
	ssize = sizeof(cp_time);
	if (sysctl(mib, 2, cp_time, &ssize, NULL, 0) < 0) {
		fprintf(stderr, "top: sysctl kern.cp_time failed: %s\n",
		    strerror(errno));
		quit(23);
	}

	if (getloadavg(si->load_avg, NUM_AVERAGES) < 0) {
		int i;

		warn("can't getloadavg");
		for (i = 0; i < NUM_AVERAGES; i++)
			si->load_avg[i] = 0.0;
	}

	/* convert cp_time counts to percentages */
	percentages64(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP2;
	ssize = sizeof(uvmexp);
	if (sysctl(mib, 2, &uvmexp, &ssize, NULL, 0) < 0) {
		fprintf(stderr, "top: sysctl vm.uvmexp2 failed: %s\n",
		    strerror(errno));
		quit(23);
	}

	/* convert memory stats to Kbytes */
	memory_stats[0] = pagetok(uvmexp.active);
	memory_stats[1] = pagetok(uvmexp.inactive);
	memory_stats[2] = pagetok(uvmexp.wired);
	memory_stats[3] = pagetok(uvmexp.execpages);
	memory_stats[4] = pagetok(uvmexp.filepages);
	memory_stats[5] = pagetok(uvmexp.free);

	swap_stats[0] = swap_stats[1] = swap_stats[2] = 0;

	seporig = NULL;
	do {
		nswap = swapctl(SWAP_NSWAP, 0, 0);
		if (nswap < 1)
			break;
		/* Use seporig to keep track of the malloc'd memory
		 * base, as sep will be incremented in the for loop
		 * below.
		 */
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
		swap_stats[0] = dbtob(totalsize) / 1024;
		swap_stats[1] = dbtob(totalinuse) / 1024;
		swap_stats[2] = dbtob(totalsize) / 1024 - swap_stats[1];
		/* Free here, before we malloc again in the next
		 * iteration of this loop.
		 */
		if (seporig) {
			free(seporig);
			seporig = NULL;
		}
	} while (0);
	/* Catch the case where we malloc'd, but then exited the
	 * loop due to nswap != rnswap.
	 */
	if (seporig)
		free(seporig);

	memory_stats[6] = -1;
	swap_stats[3] = -1;

	/* set arrays and strings */
	si->cpustates = cpu_states;
	si->memory = memory_stats;
	si->swap = swap_stats;
	si->last_pid = -1;
}


caddr_t
get_process_info(si, sel, compare)
	struct system_info *si;
	struct process_select *sel;
	int (*compare) __P((struct proc **, struct proc **));
{
	int i;
	int total_procs;
	int active_procs;
	struct kinfo_proc2 **prefp;
	struct kinfo_proc2 *pp;

	/* these are copied out of sel for speed */
	int show_idle;
	int show_system;
	int show_uid;
	int show_command;

	static struct handle handle;


	pbase = kvm_getproc2(kd, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2), &nproc);
	if (nproc > onproc)
		pref = (struct kinfo_proc2 **) realloc(pref,
		    sizeof(struct kinfo_proc2 *) * (onproc = nproc));
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
	for (pp = pbase, i = 0; i < nproc; pp++, i++) {

		/*
		 * Place pointers to each valid proc structure in pref[].
		 * Process slots that are actually in use have a non-zero
		 * status field.  Processes with P_SYSTEM set are system
		 * processes---these get ignored unless show_sysprocs is set.
		 */
		if (pp->p_stat != 0 && (show_system || ((pp->p_flag & P_SYSTEM) == 0))) {
			total_procs++;
			process_states[(unsigned char) pp->p_stat]++;
			if (pp->p_stat != LSZOMB && pp->p_stat != LSDEAD &&
			    (show_idle || (pp->p_pctcpu != 0) || 
			    (pp->p_stat == LSRUN || pp->p_stat == LSONPROC)) &&
			    (!show_uid || pp->p_ruid == (uid_t)sel->uid)) {
				*prefp++ = pp;
				active_procs++;
			}
		}
	}

	/* if requested, sort the "interesting" processes */
	if (compare != NULL) {
		qsort((char *)pref, active_procs, sizeof(struct kinfo_proc2 *), 
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


char *
format_next_process(handle, get_userid)
	caddr_t handle;
	char *(*get_userid) __P((int));
{
	struct kinfo_proc2 *pp;
	long cputime;
	double pct;
	struct handle *hp;
	const char *statep;
#ifdef KI_NOCPU
	char state[10];
#endif
	char wmesg[KI_WMESGLEN + 1];
	static char fmt[128];		/* static area where result is built */
	char *pretty = "";

	/* find and remember the next proc structure */
	hp = (struct handle *)handle;
	pp = *(hp->next_proc++);
	hp->remaining--;

	/* get the process's user struct and set cputime */
	if ((pp->p_flag & L_INMEM) == 0)
		pretty = "<>";
	else if ((pp->p_flag & P_SYSTEM) != 0)
		pretty = "[]";

	if (pretty[0] != '\0') {
		/*
		 * Print swapped processes as <pname> and
		 * system processes as [pname]
		 */
		char *comm = pp->p_comm;
#define COMSIZ sizeof(pp->p_comm)
		char buf[COMSIZ];
		(void) strncpy(buf, comm, COMSIZ);
		comm[0] = pretty[0];
		(void) strncpy(&comm[1], buf, COMSIZ - 2);
		comm[COMSIZ - 2] = '\0';
		(void) strncat(comm, &pretty[1], COMSIZ - 1);
		comm[COMSIZ - 1] = '\0';
	}

#if 0
	/* This does not produce the correct results */
	cputime = pp->p_uticks + pp->p_sticks + pp->p_iticks;
#else
	cputime = pp->p_rtime_sec;	/* This does not count interrupts */
#endif

	/* calculate the base for cpu percentages */
	pct = pctdouble(pp->p_pctcpu);

	if (pp->p_stat == LSSLEEP) {
		strlcpy(wmesg, pp->p_wmesg, sizeof(wmesg));
		statep = wmesg;
	} else
		statep = state_abbrev[(unsigned)pp->p_stat];

#ifdef KI_NOCPU
	/* Post-1.5 change: add cpu number if appropriate */
	if (pp->p_cpuid != KI_NOCPU) {
		switch (pp->p_stat) {
		case LSONPROC:
		case LSRUN:
		case LSSLEEP:			
			snprintf(state, sizeof(state), "%.6s/%lld", 
				 statep, (long long)pp->p_cpuid);
			statep = state;
			break;
		}
	}
#endif
	/* format this entry */
	sprintf(fmt,
	    Proc_format,
	    pp->p_pid,
	    (*get_userid)(pp->p_ruid),
	    pp->p_priority - PZERO,
	    pp->p_nice - NZERO,
	    format_k(pagetok(PROCSIZE(pp))),
	    format_k(pagetok(pp->p_vm_rssize)),
	    statep,
	    format_time(cputime),
	    100.0 * weighted_cpu(pct, pp),
	    100.0 * pct,
	    printable(pp->p_comm));

	/* return the result */
	return(fmt);
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
	if (lresult = (pctcpu)(p2)->p_pctcpu - (pctcpu)(p1)->p_pctcpu,\
	    (result = lresult > 0 ? 1 : lresult < 0 ? -1 : 0) == 0)

#define ORDERKEY_CPTICKS \
	if (lresult = (pctcpu)(p2)->p_rtime_sec \
		    - (pctcpu)(p1)->p_rtime_sec,\
	    (result = lresult > 0 ? 1 : lresult < 0 ? -1 : 0) == 0)

#define ORDERKEY_STATE \
	if ((result = sorted_state[(int)(p2)->p_stat] - \
		      sorted_state[(int)(p1)->p_stat] ) == 0)

#define ORDERKEY_PRIO \
	if ((result = (p2)->p_priority - (p1)->p_priority) == 0)

#define ORDERKEY_RSSIZE \
	if ((result = p2->p_vm_rssize - p1->p_vm_rssize) == 0)

#define ORDERKEY_MEM	\
	if ((result = (PROCSIZE(p2) - PROCSIZE(p1))) == 0)

/*
 * Now the array that maps process state to a weight.
 * The order of the elements should match those in state_abbrev[]
 */

static int sorted_state[] = {
	0,	/*  (not used)	  ?	*/
	6,	/* "start"	SIDL	*/
	4,	/* "run"	SRUN	*/
	3,	/* "sleep"	SSLEEP	*/
	3,	/* "stop"	SSTOP	*/
	2,	/* "dead"	SDEAD	*/
	1,	/* "zomb"	SZOMB	*/
	5,	/* "onproc"	SONPROC	*/
};

/* compare_cpu - the comparison function for sorting by cpu percentage */

static int
compare_cpu(pp1, pp2)
	struct proc **pp1, **pp2;
{
	struct kinfo_proc2 *p1;
	struct kinfo_proc2 *p2;
	int result;
	pctcpu lresult;

	/* remove one level of indirection */
	p1 = *(struct kinfo_proc2 **) pp1;
	p2 = *(struct kinfo_proc2 **) pp2;

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

static int
compare_prio(pp1, pp2)
	struct proc **pp1, **pp2;
{
	struct kinfo_proc2 *p1;
	struct kinfo_proc2 *p2;
	int result;
	pctcpu lresult;

	/* remove one level of indirection */
	p1 = *(struct kinfo_proc2 **) pp1;
	p2 = *(struct kinfo_proc2 **) pp2;

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

static int
compare_res(pp1, pp2)
	struct proc **pp1, **pp2;
{
	struct kinfo_proc2 *p1;
	struct kinfo_proc2 *p2;
	int result;
	pctcpu lresult;

	/* remove one level of indirection */
	p1 = *(struct kinfo_proc2 **) pp1;
	p2 = *(struct kinfo_proc2 **) pp2;

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

static int
compare_size(pp1, pp2)
	struct proc **pp1, **pp2;
{
	struct kinfo_proc2 *p1;
	struct kinfo_proc2 *p2;
	int result;
	pctcpu lresult;

	/* remove one level of indirection */
	p1 = *(struct kinfo_proc2 **) pp1;
	p2 = *(struct kinfo_proc2 **) pp2;

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

static int
compare_state(pp1, pp2)
	struct proc **pp1, **pp2;
{
	struct kinfo_proc2 *p1;
	struct kinfo_proc2 *p2;
	int result;
	pctcpu lresult;

	/* remove one level of indirection */
	p1 = *(struct kinfo_proc2 **) pp1;
	p2 = *(struct kinfo_proc2 **) pp2;

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

static int
compare_time(pp1, pp2)
	struct proc **pp1, **pp2;
{
	struct kinfo_proc2 *p1;
	struct kinfo_proc2 *p2;
	int result;
	pctcpu lresult;

	/* remove one level of indirection */
	p1 = *(struct kinfo_proc2 **) pp1;
	p2 = *(struct kinfo_proc2 **) pp2;

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
	struct kinfo_proc2 **prefp;
	struct kinfo_proc2 *pp;

	prefp = pref;
	cnt = pref_len;
	while (--cnt >= 0) {
		pp = *prefp++;	
		if (pp->p_pid == (pid_t)pid)
			return(pp->p_ruid);
	}
	return(-1);
}

/*
 *  percentages(cnt, out, new, old, diffs) - calculate percentage change
 *	between array "old" and "new", putting the percentages i "out".
 *	"cnt" is size of each array and "diffs" is used for scratch space.
 *	The array "old" is updated on each call.
 *	The routine assumes modulo arithmetic.  This function is especially
 *	useful on BSD mchines for calculating cpu state percentages.
 */

void
percentages64(cnt, out, new, old, diffs)
	int cnt;
	int *out;
	u_int64_t *new;
	u_int64_t *old;
	u_int64_t *diffs;
{
	int i;
	u_int64_t change;
	u_int64_t total_change;
	u_int64_t *dp;
	u_int64_t half_total;

	/* initialization */
	total_change = 0;
	dp = diffs;

	/* calculate changes for each state and the overall change */
	for (i = 0; i < cnt; i++) {
		/*
		 * Don't worry about wrapping - even at hz=1GHz, a
		 * u_int64_t will last at least 544 years.
		 */
		change = *new - *old;
		total_change += (*dp++ = change);
		*old++ = *new++;
	}

	/* avoid divide by zero potential */
	if (total_change == 0)
		total_change = 1;

	/* calculate percentages based on overall change, rounding up */
	half_total = total_change / 2;
	for (i = 0; i < cnt; i++)
		*out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
}
