/*	$NetBSD: spkr.h,v 1.1 1997/10/14 06:48:44 sakamoto Exp $	*/

/*
 * spkr.h -- interface definitions for speaker ioctl()
 */

#ifndef _BEBOX_SPKR_H_
#define _BEBOX_SPKR_H_

#include <sys/ioctl.h>

#define SPKRTONE        _IOW('S', 1, tone_t)    /* emit tone */
#define SPKRTUNE        _IO('S', 2)             /* emit tone sequence */

typedef struct {
	int	frequency;	/* in hertz */
	int	duration;	/* in 1/100ths of a second */
} tone_t;

#endif /* _BEBOX_SPKR_H_ */
