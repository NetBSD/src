/*	$NetBSD: spkrio.h,v 1.1 2016/12/09 04:32:39 christos Exp $	*/

/*
 * spkr.h -- interface definitions for speaker ioctl()
 */

#ifndef _DEV_ISA_SPKR_H_
#define _DEV_ISA_SPKR_H_

#include <sys/ioccom.h>

#define SPKRTONE        _IOW('S', 1, tone_t)    /* emit tone */
#define SPKRTUNE        _IO('S', 2)             /* emit tone sequence */

typedef struct {
	int	frequency;	/* in hertz */
	int	duration;	/* in 1/100ths of a second */
} tone_t;

void spkr_tone(u_int, u_int);
void spkr_rest(int);
int spkr__modcmd(modcmd_t, void *);
int spkr_probe(device_t, cfdata_t, void *);
extern int spkr_attached;

#endif
