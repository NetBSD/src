/*	$Id: savar.h,v 1.1.2.2 2001/07/09 22:37:26 nathanw Exp $	*/

/* XXX Copyright 
 *
 */

/*
 * Internal data usd by the scheduler activation implementation
 */

#ifndef _SYS_SAVAR_H
#define _SYS_SAVAR_H

struct sadata_upcall {
	LIST_ENTRY(sadata_upcall) sau_next;
	int sau_type;
	int sau_sig;
	u_long sau_code;
	stack_t sau_stack;
	struct lwp *sau_event;
	struct lwp *sau_interrupted;
};

struct sadata {

	struct simplelock sa_lock; /* Lock on fields of this structure. */

	sa_upcall_t sa_upcall;	/* The upcall entry point for this process */
	
	LIST_HEAD(, lwp) sa_lwpcache;
	int sa_ncached;

	stack_t *sa_stacks;	/* Pointer to array of upcall stacks */
	int sa_nstackentries;	/* Size of the array */
	int sa_nstacks;		/* Number of entries with valid stacks */
	LIST_HEAD(, sadata_upcall) sa_upcalls; /* Pending upcalls */

	int sa_concurrency;	/* Desired concurrency */
};

extern struct pool sadata_pool;		/* memory pool for sadata structures */
extern struct pool saupcall_pool;	/* memory pool for pending upcalls */

#define SA_NUMSTACKS	16	/* Number of stacks allocated. XXX */

void	sa_switch(struct lwp *, int);
void	sa_switchcall(void *);
int	sa_upcall(struct lwp *, int, struct lwp *, struct lwp *, int, u_long);
void	cpu_upcall(struct lwp *);
ucontext_t *cpu_stashcontext(struct lwp *);


#endif /* !_SYS_SAVAR_H */
