#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: shmdt.c,v 1.4 1994/05/28 23:37:56 hpeyerl Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
void shmdt(const void *shmaddr)
#else
void shmdt(shmaddr)
	const void *shmaddr;
#endif
{
	return ((void) shmsys(2, shmaddr));
}
