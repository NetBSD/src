/*	$NetBSD: rumpdefs.h,v 1.5 2009/11/03 18:23:15 pooka Exp $	*/

/*
 *	AUTOMATICALLY GENERATED.  DO NOT EDIT.
 */

#ifndef _RUMP_RUMPDEFS_H_
#define _RUMP_RUMPDEFS_H_

#include <rump/rump_namei.h>

/*	NetBSD: fcntl.h,v 1.35 2009/03/11 06:05:29 mrg Exp 	*/
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

/*	NetBSD: vnode.h,v 1.210 2009/10/06 04:28:10 elad Exp 	*/
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

/*	NetBSD: reboot.h,v 1.25 2007/12/25 18:33:48 perry Exp 	*/
#define	RUMP_RB_AUTOBOOT	0	
#define	RUMP_RB_ASKNAME	0x00000001	
#define	RUMP_RB_SINGLE	0x00000002	
#define	RUMP_RB_NOSYNC	0x00000004	
#define	RUMP_RB_HALT		0x00000008	
#define	RUMP_RB_INITNAME	0x00000010	
#define	__RUMP_RB_UNUSED1	0x00000020	
#define	RUMP_RB_KDB		0x00000040	
#define	RUMP_RB_RDONLY	0x00000080	
#define	RUMP_RB_DUMP		0x00000100	
#define	RUMP_RB_MINIROOT	0x00000200	
#define	RUMP_RB_STRING	0x00000400	
#define	RUMP_RB_POWERDOWN	(RUMP_RB_HALT|0x800) 
#define RUMP_RB_USERCONF	0x00001000	
#define	RUMP_RB_MD1		0x10000000
#define	RUMP_RB_MD2		0x20000000
#define	RUMP_RB_MD3		0x40000000
#define	RUMP_RB_MD4		0x80000000
#define	RUMP_AB_NORMAL	0x00000000	
#define	RUMP_AB_QUIET	0x00010000 	
#define	RUMP_AB_VERBOSE	0x00020000	
#define	RUMP_AB_SILENT	0x00040000	
#define	RUMP_AB_DEBUG	0x00080000	

#endif /* _RUMP_RUMPDEFS_H_ */
