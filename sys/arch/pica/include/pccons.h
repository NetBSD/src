/*
 * pccons.h -- pccons ioctl definitions
 *
 *	$Id: pccons.h,v 1.1.1.1 1996/03/13 04:58:08 jonathan Exp $
 */

#ifndef _PCCONS_H_
#define _PCCONS_H_

#ifndef _KERNEL
#include <sys/ioctl.h>
#else
#include "ioctl.h"
#endif

#define CONSOLE_X_MODE_ON		_IO('t',121)
#define CONSOLE_X_MODE_OFF		_IO('t',122)
#define CONSOLE_X_BELL			_IOW('t',123,int[2])
#define CONSOLE_SET_TYPEMATIC_RATE	_IOW('t',124,u_char)

#endif /* _PCCONS_H_ */
