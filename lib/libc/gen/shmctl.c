#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: shmctl.c,v 1.4 1994/05/28 23:37:55 hpeyerl Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
void shmctl(int shmid, int cmd, struct shmid_ds *buf)
#else
void shmctl(shmid, cmd, buf)
	int shmid;
	int cmd;
	struct shmid_ds *buf;
#endif
{
	return ((void) shmsys(1, shmid, cmd, buf));
}
