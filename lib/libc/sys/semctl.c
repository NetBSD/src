/*	$NetBSD: semctl.c,v 1.2 1995/02/27 10:41:36 cgd Exp $	*/

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
