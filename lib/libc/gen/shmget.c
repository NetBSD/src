#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: shmget.c,v 1.3 1994/05/28 23:37:57 hpeyerl Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
void shmget(key_t key, size_t size, int shmflg)
#else
void shmget(key, size, shmflg)
	key_t key;
	size_t size;
	int shmflg;
#endif
{
	return ((void) shmsys(3, key, size, shmflg));
}
