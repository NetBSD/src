/*	$NetBSD: pccons.h,v 1.4.12.1 1997/08/23 07:09:05 thorpej Exp $	*/

/*
 * pccons.h -- pccons ioctl definitions
 */

#ifndef _PCCONS_H_
#define _PCCONS_H_

#include <sys/ioctl.h>

#define CONSOLE_X_MODE_ON		_IO('t',121)
#define CONSOLE_X_MODE_OFF		_IO('t',122)
#define CONSOLE_X_BELL			_IOW('t',123,int[2])
#define CONSOLE_SET_TYPEMATIC_RATE	_IOW('t',124,u_char)

#ifdef _KERNEL
int pccngetc __P((dev_t));
void pccnputc __P((dev_t, int));
void pccnpollc __P((dev_t, int));
void pccninit __P((struct consdev *));
#endif

#endif /* _PCCONS_H_ */
