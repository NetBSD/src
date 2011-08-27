/* $NetBSD: intrdefs.h,v 1.9.18.1 2011/08/27 15:59:49 jym Exp $ */

/* This file co-exists, and is included via machine/intrdefs.h */

#ifndef _XEN_INTRDEFS_H_
#define _XEN_INTRDEFS_H_

/* Xen IPI types */
#define XEN_IPI_KICK		0x00000000
#define XEN_IPI_HALT		0x00000001
#define XEN_IPI_SYNCH_FPU	0x00000002
#define XEN_IPI_DDB		0x00000004
#define XEN_IPI_XCALL		0x00000008

#define XEN_NIPIS 4 /* IPI_KICK doesn't have a handler */

#endif /* _XEN_INTRDEFS_H_ */
