/*
 *	$Id: semctl.c,v 1.1 1994/10/20 04:17:06 cgd Exp $
 */     

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#if __STDC__
int semctl(int semid, int semnum, int cmd, union semun semun)
#else
int semctl(semid, int semnum, cmd, semun)
	int semid, semnum;
	int cmd;
	union semun semun;
#endif
{
	return (__semctl(semid, semnum, cmd, &semun));
}
