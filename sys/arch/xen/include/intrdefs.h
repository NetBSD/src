/* $NetBSD: intrdefs.h,v 1.12.28.2 2020/04/08 14:07:59 martin Exp $ */

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
#define XEN_IPI_AST		0x00000040

/* Note: IPI_KICK does not have a handler. */
#define XEN_NIPIS		7

/* The number of 'irqs' that XEN understands */
#define NUM_XEN_IRQS 		256

#define XEN_IPI_NAMES {  "halt IPI", "FPU synch IPI", \
			 "DDB IPI", "xcall IPI", \
			 "HVCB IPI", "generic IPI", \
			 "AST IPI" }

#endif /* _XEN_INTRDEFS_H_ */
