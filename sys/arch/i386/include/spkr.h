/*
 * spkr.h -- interface definitions for speaker ioctl()
 *
 *	$Id: spkr.h,v 1.2 1994/01/11 13:30:40 mycroft Exp $
 */

#ifndef _I386_SPKR_H_
#define _I386_SPKR_H_

#include <sys/ioctl.h>

#define SPKRTONE        _IOW('S', 1, tone_t)    /* emit tone */
#define SPKRTUNE        _IO('S', 2)             /* emit tone sequence */

typedef struct {
	int	frequency;	/* in hertz */
	int	duration;	/* in 1/100ths of a second */
} tone_t;

#endif /* _I386_SPKR_H_ */
