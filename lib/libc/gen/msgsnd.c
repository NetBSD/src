/*
 *	$Id: msgsnd.c,v 1.1 1993/11/14 12:40:29 cgd Exp $
 */     

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#if __STDC__
int msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg)
#else
int msgsnd(msqid, msgp, msgsz, msgflg)
	int msqid;
	void *msgp;
	size_t msgsz;
	int msgflg;
#endif
{
	return (msgsys(2, msqid, msgp, msgsz, msgflg));
}
