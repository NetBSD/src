/*	$NetBSD: nbcompat.c,v 1.10.38.1 2011/06/06 09:07:31 jruoho Exp $	*/

/* nbcompat.c
 * Implementations of some FreeBSD functions on NetBSD to make things
 * a bit smoother.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nbcompat.c,v 1.10.38.1 2011/06/06 09:07:31 jruoho Exp $");

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
#include <sys/bus.h>

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

int
bus_release_resource(device_t dev, int type, int rid,
    struct ndis_resource *r)
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

int
device_is_attached(device_t dev)
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
  struct proc **newpp, void *stack, size_t stacksize, const char *name)
{
  struct lwp *l;
  int error;
  
  error = kthread_create(PRI_NONE, 0, NULL, func, arg, &l, "%s", name);
  if (__predict_false(error != 0))
    return (error);
  
  /* All done! */
  if (newpp != NULL)
    *newpp = l->l_proc;
  return (0);
}
