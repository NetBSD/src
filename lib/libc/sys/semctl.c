/*
 *	$Id: semctl.c,v 1.1.2.1 1995/04/18 04:33:42 jtc Exp $
 */     

#include "namespace.h"
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
