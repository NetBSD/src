/*
 *	$Id: msgctl.c,v 1.1 1993/11/14 12:40:25 cgd Exp $
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#if __STDC__
int msgctl(int msqid, int cmd, struct msqid_ds *buf)
#else
int msgctl(msqid,cmd,buf)
	int msqid;
	int cmd;
	caddr_t buf;
#endif
{
	return (msgsys(0, msqid, cmd, buf));
}
