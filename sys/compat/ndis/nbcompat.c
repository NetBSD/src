/* nbcompat.c
 * Implementations of some FreeBSD functions on NetBSD to make things
 * a bit smoother.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/lock.h>
#include <machine/bus.h>

#include <machine/stdarg.h>

#include "nbcompat.h"

/* note: this is also defined in ntoskrnl_var.h, but I didn't want to include
 * the whole file here
 */
#define NDIS_KSTACK_PAGES	8

struct ndis_resource{
   bus_space_handle_t res_handle;
   bus_space_tag_t    res_tag;
   bus_addr_t         res_base;
   bus_size_t         res_size;
};

int bus_release_resource(dev, type, rid, r)
	device_t 		 dev;
	int			 type;
	int			 rid;
	struct ndis_resource 	*r;
{
	switch(type) {
	case SYS_RES_IOPORT:
		bus_space_unmap(r->res_tag, r->res_handle, r->res_size);
		break;
	case SYS_RES_MEMORY:
		bus_space_unmap(r->res_tag, r->res_handle, r->res_size);
		break;
	default:
		printf("error: bus_release_resource()");
	}
	
	return 0;
}

void mtx_lock(struct mtx *mutex)
{
	/* I'm not sure if this is needed or not.  NetBSD kernel
	 * threads aren't preempted, but there still may be a need
	 * for lockmgr locks.
	 */
	//lockmgr(mutex, LK_EXCLUSIVE, NULL);
}

void mtx_unlock(struct mtx *mutex)
{
	//lockmgr(mutex, LK_RELEASE, NULL);
}

int device_is_attached(dev)
	device_t dev;
{
	/* Sure, it's attached? */
	return TRUE;
}

/* I took this from sys/kern/kern_kthread.c (in the NetBSD source tree).
 * The only difference is the kernel stack size
 */

/*
 * Fork a kernel thread.  Any process can request this to be done.
 * The VM space and limits, etc. will be shared with proc0.
 */
int
ndis_kthread_create(void (*func)(void *), void *arg,
    struct proc **newpp, void *stack, size_t stacksize, const char *fmt, ...)
{
        struct proc *p2;
        int error;
        va_list ap;

        /* First, create the new process. */
        error = fork1(&lwp0, FORK_SHAREVM | FORK_SHARECWD | FORK_SHAREFILES |
            FORK_SHARESIGS, SIGCHLD, stack, stacksize, func, arg, NULL, &p2);
        if (__predict_false(error != 0))
                return (error);

        /*
         * Mark it as a system process and not a candidate for
         * swapping.  Set P_NOCLDWAIT so that children are reparented
         * to init(8) when they exit.  init(8) can easily wait them
         * out for us.
         */
        p2->p_flag |= P_SYSTEM | P_NOCLDWAIT;
        LIST_FIRST(&p2->p_lwps)->l_flag |= L_INMEM;

        /* Name it as specified. */
        va_start(ap, fmt);
        vsnprintf(p2->p_comm, MAXCOMLEN, fmt, ap);
        va_end(ap);

        /* All done! */
        if (newpp != NULL)
                *newpp = p2;
        return (0);
}
