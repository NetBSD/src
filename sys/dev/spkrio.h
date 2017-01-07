/*	$NetBSD: spkrio.h,v 1.2.2.2 2017/01/07 08:56:31 pgoyette Exp $	*/

/*
 * spkrio.h -- interface definitions for speaker ioctl()
 */

#ifndef _DEV_SPKRIO_H_
#define _DEV_SPKRIO_H_

#include <sys/ioccom.h>

#define SPKRTONE        _IOW('S', 1, tone_t)    /* emit tone */
#define SPKRTUNE        _IO('S', 2)             /* emit tone sequence */

typedef struct {
	int	frequency;	/* in hertz */
	int	duration;	/* in 1/100ths of a second */
} tone_t;

#endif
