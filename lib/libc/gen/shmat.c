#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if __STDC__
int shmat(int shmid, caddr_t shmaddr, int shmflg)
#else
int shmat(shmid, shmaddr, shmflg)
	int shmid;
	caddr_t shmaddr;
	int shmflg;
#endif
{
	return (shmsys(0, shmid, shmaddr, shmflg));
}
