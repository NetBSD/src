#ifndef _SIGDEBUG_H_
#define _SIGDEBUG_H_

#ifdef DEBUG
extern int sigdebug;
extern int sigpid;
#define	SDB_FOLLOW	0x01
#define	SDB_KSTACK	0x02
#endif

#endif /* _SIGDEBUG_H_ */
