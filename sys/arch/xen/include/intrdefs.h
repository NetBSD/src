/* $NetBSD: intrdefs.h,v 1.11.24.1 2014/08/10 06:54:11 tls Exp $ */

/* This file co-exists, and is included via machine/intrdefs.h */

#ifndef _XEN_INTRDEFS_H_
#define _XEN_INTRDEFS_H_

/* Xen IPI types */
#define XEN_IPI_KICK		0x00000000
#define XEN_IPI_HALT		0x00000001
#define XEN_IPI_SYNCH_FPU	0x00000002
#define XEN_IPI_DDB		0x00000004
#define XEN_IPI_XCALL		0x00000008
#define XEN_IPI_HVCB		0x00000010
#define XEN_IPI_GENERIC		0x00000020

/* Note: IPI_KICK does not have a handler. */
#define XEN_NIPIS		6

#endif /* _XEN_INTRDEFS_H_ */
