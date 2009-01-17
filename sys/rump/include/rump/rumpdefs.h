/*	$NetBSD: rumpdefs.h,v 1.3.4.2 2009/01/17 13:29:35 mjf Exp $	*/

/*
 *	AUTOMATICALLY GENERATED.  DO NOT EDIT.
 */

#ifndef _RUMP_RUMPDEFS_H_
#define _RUMP_RUMPDEFS_H_

#include <rump/rump_namei.h>

/*	NetBSD: fcntl.h,v 1.34 2006/10/05 14:48:33 chs Exp 	*/
#define	RUMP_O_RDONLY	0x00000000	/* open for reading only */
#define	RUMP_O_WRONLY	0x00000001	/* open for writing only */
#define	RUMP_O_RDWR		0x00000002	/* open for reading and writing */
#define	RUMP_O_ACCMODE	0x00000003	/* mask for above modes */
#define	RUMP_O_NONBLOCK	0x00000004	/* no delay */
#define	RUMP_O_APPEND	0x00000008	/* set append mode */
#define	RUMP_O_SHLOCK	0x00000010	/* open with shared file lock */
#define	RUMP_O_EXLOCK	0x00000020	/* open with exclusive file lock */
#define	RUMP_O_ASYNC		0x00000040	/* signal pgrp when data ready */
#define	RUMP_O_SYNC		0x00000080	/* synchronous writes */
#define	RUMP_O_NOFOLLOW	0x00000100	/* don't follow symlinks on the last */
#define	RUMP_O_CREAT		0x00000200	/* create if nonexistent */
#define	RUMP_O_TRUNC		0x00000400	/* truncate to zero length */
#define	RUMP_O_EXCL		0x00000800	/* error if already exists */
#define	RUMP_O_DSYNC		0x00010000	/* write: I/O data completion */
#define	RUMP_O_RSYNC		0x00020000	/* read: I/O completion as for write */
#define	RUMP_O_DIRECT	0x00080000	/* direct I/O hint */
#define	RUMP_O_NOCTTY	0x00008000	/* don't assign controlling terminal */

/*	NetBSD: vnode.h,v 1.197 2008/07/31 05:38:06 simonb Exp 	*/
#ifndef __VTYPE_DEFINED
#define __VTYPE_DEFINED
enum vtype	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };
#endif /* __VTYPE_DEFINED */

/*	NetBSD: errno.h,v 1.39 2006/10/31 00:38:07 cbiere Exp 	*/
#ifndef EJUSTRETURN
#define	EJUSTRETURN	-2		/* don't modify regs, just return */
#endif /* EJUSTRETURN */

/*	NetBSD: lock.h,v 1.83 2008/04/28 20:24:10 martin Exp 	*/
#define	RUMP_LK_TYPE_MASK	0x0000000f	
#define	RUMP_LK_SHARED	0x00000001	
#define	RUMP_LK_EXCLUSIVE	0x00000002	
#define	RUMP_LK_RELEASE	0x00000006	
#define	RUMP_LK_EXCLOTHER	0x00000008	
#define	RUMP_LK_NOWAIT	0x00000010	
#define	RUMP_LK_CANRECURSE	0x00000040	
#define	RUMP_LK_INTERLOCK	0x00010000	
#define	RUMP_LK_RETRY	0x00020000	

#endif /* _RUMP_RUMPDEFS_H_ */
