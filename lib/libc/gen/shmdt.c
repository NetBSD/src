#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
int shmdt(caddr_t shmaddr)
#else
int shmdt(shmaddr)
	caddr_t shmaddr;
#endif
{
	return (shmsys(2, shmaddr));
}
