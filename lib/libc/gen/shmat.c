#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: shmat.c,v 1.3 1993/11/19 03:18:24 mycroft Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
int shmat(int shmid, void *shmaddr, int shmflg)
#else
int shmat(shmid, shmaddr, shmflg)
	int shmid;
	void *shmaddr;
	int shmflg;
#endif
{
	return (shmsys(0, shmid, shmaddr, shmflg));
}
