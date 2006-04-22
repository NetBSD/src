/* $NetBSD: kern_securelevel.c,v 1.2.4.2 2006/04/22 11:39:59 simonb Exp $ */

#include "opt_insecure.h"

#include <sys/param.h>
#include <sys/sysctl.h>

#ifdef INSECURE
int securelevel = -1;
#else
int securelevel = 0;  
#endif

static int sysctl_kern_securelevel(SYSCTLFN_PROTO);

SYSCTL_SETUP(sysctl_kern_securelevel_setup, "sysctl kern.securelevel setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "securelevel",
		       SYSCTL_DESCR("System security level"),
		       sysctl_kern_securelevel, 0, &securelevel, 0,
		       CTL_KERN, KERN_SECURELVL, CTL_EOL);
}

/*
 * sysctl helper routine for kern.securelevel.  ensures that the value
 * only rises unless the caller has pid 1 (assumed to be init).
 */
static int
sysctl_kern_securelevel(SYSCTLFN_ARGS)
{
	int newsecurelevel, error;
	struct sysctlnode node;

	newsecurelevel = securelevel;
	node = *rnode;
	node.sysctl_data = &newsecurelevel;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (newsecurelevel < securelevel && l && l->l_proc->p_pid != 1)
		return (EPERM);
	securelevel = newsecurelevel;

	return (error);
}
