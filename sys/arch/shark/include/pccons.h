/*	$NetBSD: pccons.h,v 1.1.14.2 2002/06/23 17:41:12 jdolecek Exp $	*/

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
#define CONSOLE_X_TV_ON                 _IOW('t',155,int)
#define CONSOLE_X_TV_OFF                _IO('t',156)
#define CONSOLE_GET_LINEAR_INFO         _IOR('t',157,struct map_info)
#define CONSOLE_GET_IO_INFO             _IOR('t',158,struct map_info)
#define CONSOLE_GET_MEM_INFO            _IOR('t',159,struct map_info)

#define XMODE_RGB   0
#define XMODE_NTSC  1
#define XMODE_PAL   2
#define XMODE_SECAM 3


#endif /* _PCCONS_H_ */
