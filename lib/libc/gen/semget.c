/*
 *	$Id: semget.c,v 1.1 1993/11/14 12:40:33 cgd Exp $
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#if __STDC__
int semget(key_t key, int nsems, int semflg)
#else
int semget(key, nsems, semflg)
	key_t key;
	int nsems;
	int semflg;
#endif
{
	return (semsys(1, key, nsems, semflg));
}
