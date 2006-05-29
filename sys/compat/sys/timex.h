/* $NetBSD: timex.h,v 1.1 2006/05/29 09:57:54 drochner Exp $ */

/******************************************************************************
 *                                                                            *
 * Copyright (c) David L. Mills 1993, 1994                                    *
 *                                                                            *
 * Permission to use, copy, modify, and distribute this software and its      *
 * documentation for any purpose and without fee is hereby granted, provided  *
 * that the above copyright notice appears in all copies and that both the    *
 * copyright notice and this permission notice appear in supporting           *
 * documentation, and that the name University of Delaware not be used in     *
 * advertising or publicity pertaining to distribution of the software        *
 * without specific, written prior permission.  The University of Delaware    *
 * makes no representations about the suitability this software for any       *
 * purpose.  It is provided "as is" without express or implied warranty.      *
 *                                                                            *
 ******************************************************************************/

struct ntptimeval30 {
	struct timeval time;	/* current time (ro) */
	long maxerror;		/* maximum error (us) (ro) */
	long esterror;		/* estimated error (us) (ro) */
};

#ifndef _KERNEL
int ntp_gettime(struct ntptimeval30 *);
int __ntp_gettime30(struct ntptimeval *);
#endif
