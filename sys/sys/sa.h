/*	$Id: sa.h,v 1.1.2.1 2001/03/05 22:50:03 nathanw Exp $	*/

/* XXX Copyright 
 */

#ifndef _SYS_SA_H
#define _SYS_SA_H

#include <sys/ucontext.h>

struct sa_t {
	ucontext_t *sa_context;
	int sa_id;
	int sa_cpu;
	int sa_sig;
	int sa_code;
};

typedef void (*sa_upcall_t)(int type, struct sa_t *sas[], int events, 
    int interrupted);

#define SA_UPCALL_NEWPROC		0
#define SA_UPCALL_PREEMPTED		1
#define SA_UPCALL_BLOCKED		2
#define SA_UPCALL_UNBLOCKED		3
#define SA_UPCALL_SIGNAL		4
#define SA_UPCALL_USER 			5
#define SA_UPCALL_NUPCALLS		6


#endif /* !_SYS_SA_H */
