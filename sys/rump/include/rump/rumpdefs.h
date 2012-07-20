/*	$NetBSD: rumpdefs.h,v 1.11 2012/07/20 09:03:09 pooka Exp $	*/

/*
 *	AUTOMATICALLY GENERATED.  DO NOT EDIT.
 */

#ifndef _RUMP_RUMPDEFS_H_
#define _RUMP_RUMPDEFS_H_

#include <rump/rump_namei.h>

struct rump_sockaddr_in {
	uint8_t		sin_len;
	uint8_t		sin_family;
	uint16_t	sin_port;
	struct {
			uint32_t s_addr;
	} sin_addr;
	int8_t		sin_zero[8];
};

/*	NetBSD: fcntl.h,v 1.42 2012/01/25 00:28:35 christos Exp 	*/
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
#define	RUMP_O_NOCTTY	0x00008000	/* don't assign controlling terminal */
#define	RUMP_O_DSYNC		0x00010000	/* write: I/O data completion */
#define	RUMP_O_RSYNC		0x00020000	/* read: I/O completion as for write */
#define	RUMP_O_DIRECT	0x00080000	/* direct I/O hint */
#define	RUMP_O_DIRECTORY	0x00200000	/* fail if not a directory */
#define	RUMP_O_CLOEXEC	0x00400000	/* set close on exec */
#define	RUMP_O_SEARCH	0x00800000	/* skip search permission checks */
#define	RUMP_O_NOSIGPIPE	0x01000000	/* don't deliver sigpipe */

/*	NetBSD: vnode.h,v 1.236 2011/11/24 15:51:30 ahoka Exp 	*/
#ifndef __VTYPE_DEFINED
#define __VTYPE_DEFINED
enum vtype	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };
#endif /* __VTYPE_DEFINED */
#define	RUMP_LK_SHARED	0x00000001	
#define	RUMP_LK_EXCLUSIVE	0x00000002	
#define	RUMP_LK_NOWAIT	0x00000010	
#define	RUMP_LK_RETRY	0x00020000	

/*	NetBSD: errno.h,v 1.39 2006/10/31 00:38:07 cbiere Exp 	*/
#ifndef EJUSTRETURN
#define	EJUSTRETURN	-2		/* don't modify regs, just return */
#endif /* EJUSTRETURN */

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

/*	NetBSD: socket.h,v 1.107 2012/06/22 18:26:35 christos Exp 	*/
#define	RUMP_SOCK_STREAM	1		
#define	RUMP_SOCK_DGRAM	2		
#define	RUMP_SOCK_RAW	3		
#define	RUMP_SOCK_RDM	4		
#define	RUMP_SOCK_SEQPACKET	5		
#define	RUMP_SOCK_CLOEXEC	0x10000000	
#define	RUMP_SOCK_NONBLOCK	0x20000000	
#define	RUMP_SOCK_NOSIGPIPE	0x40000000	
#define	RUMP_SOCK_FLAGS_MASK	0xf0000000	
#define	RUMP_AF_UNSPEC	0		
#define	RUMP_AF_LOCAL	1		
#define	RUMP_AF_UNIX		RUMP_AF_LOCAL	
#define	RUMP_AF_INET		2		
#define	RUMP_AF_IMPLINK	3		
#define	RUMP_AF_PUP		4		
#define	RUMP_AF_CHAOS	5		
#define	RUMP_AF_NS		6		
#define	RUMP_AF_ISO		7		
#define	RUMP_AF_OSI		RUMP_AF_ISO
#define	RUMP_AF_ECMA		8		
#define	RUMP_AF_DATAKIT	9		
#define	RUMP_AF_CCITT	10		
#define	RUMP_AF_SNA		11		
#define RUMP_AF_DECnet	12		
#define RUMP_AF_DLI		13		
#define RUMP_AF_LAT		14		
#define	RUMP_AF_HYLINK	15		
#define	RUMP_AF_APPLETALK	16		
#define	RUMP_AF_OROUTE	17		
#define	RUMP_AF_LINK		18		
#define	RUMP_AF_COIP		20		
#define	RUMP_AF_CNT		21		
#define	RUMP_AF_IPX		23		
#define	RUMP_AF_INET6	24		
#define RUMP_AF_ISDN		26		
#define RUMP_AF_E164		RUMP_AF_ISDN		
#define RUMP_AF_NATM		27		
#define RUMP_AF_ARP		28		
#define RUMP_AF_BLUETOOTH	31		
#define	RUMP_AF_IEEE80211	32		
#define	RUMP_AF_MPLS		33		
#define	RUMP_AF_ROUTE	34		
#define	RUMP_AF_MAX		35
#define	RUMP_PF_UNSPEC	RUMP_AF_UNSPEC
#define	RUMP_PF_LOCAL	RUMP_AF_LOCAL
#define	RUMP_PF_UNIX		RUMP_PF_LOCAL	
#define	RUMP_PF_INET		RUMP_AF_INET
#define	RUMP_PF_IMPLINK	RUMP_AF_IMPLINK
#define	RUMP_PF_PUP		RUMP_AF_PUP
#define	RUMP_PF_CHAOS	RUMP_AF_CHAOS
#define	RUMP_PF_NS		RUMP_AF_NS
#define	RUMP_PF_ISO		RUMP_AF_ISO
#define	RUMP_PF_OSI		RUMP_AF_ISO
#define	RUMP_PF_ECMA		RUMP_AF_ECMA
#define	RUMP_PF_DATAKIT	RUMP_AF_DATAKIT
#define	RUMP_PF_CCITT	RUMP_AF_CCITT
#define	RUMP_PF_SNA		RUMP_AF_SNA
#define RUMP_PF_DECnet	RUMP_AF_DECnet
#define RUMP_PF_DLI		RUMP_AF_DLI
#define RUMP_PF_LAT		RUMP_AF_LAT
#define	RUMP_PF_HYLINK	RUMP_AF_HYLINK
#define	RUMP_PF_APPLETALK	RUMP_AF_APPLETALK
#define	RUMP_PF_OROUTE	RUMP_AF_OROUTE
#define	RUMP_PF_LINK		RUMP_AF_LINK
#define	RUMP_PF_XTP		pseudo_RUMP_AF_XTP	
#define	RUMP_PF_COIP		RUMP_AF_COIP
#define	RUMP_PF_CNT		RUMP_AF_CNT
#define	RUMP_PF_INET6	RUMP_AF_INET6
#define	RUMP_PF_IPX		RUMP_AF_IPX		
#define RUMP_PF_RTIP		pseudo_RUMP_AF_RTIP	
#define RUMP_PF_PIP		pseudo_RUMP_AF_PIP
#define RUMP_PF_ISDN		RUMP_AF_ISDN		
#define RUMP_PF_E164		RUMP_AF_E164
#define RUMP_PF_NATM		RUMP_AF_NATM
#define RUMP_PF_ARP		RUMP_AF_ARP
#define RUMP_PF_KEY 		pseudo_RUMP_AF_KEY	
#define RUMP_PF_BLUETOOTH	RUMP_AF_BLUETOOTH
#define	RUMP_PF_MPLS		RUMP_AF_MPLS
#define	RUMP_PF_ROUTE	RUMP_AF_ROUTE
#define	RUMP_PF_MAX		RUMP_AF_MAX
#define	RUMP_SO_DEBUG	0x0001		
#define	RUMP_SO_ACCEPTCONN	0x0002		
#define	RUMP_SO_REUSEADDR	0x0004		
#define	RUMP_SO_KEEPALIVE	0x0008		
#define	RUMP_SO_DONTROUTE	0x0010		
#define	RUMP_SO_BROADCAST	0x0020		
#define	RUMP_SO_USELOOPBACK	0x0040		
#define	RUMP_SO_LINGER	0x0080		
#define	RUMP_SO_OOBINLINE	0x0100		
#define	RUMP_SO_REUSEPORT	0x0200		
#define	RUMP_SO_NOSIGPIPE	0x0800		
#define	RUMP_SO_ACCEPTFILTER	0x1000		
#define	RUMP_SO_TIMESTAMP	0x2000		
#define RUMP_SO_SNDBUF	0x1001		
#define RUMP_SO_RCVBUF	0x1002		
#define RUMP_SO_SNDLOWAT	0x1003		
#define RUMP_SO_RCVLOWAT	0x1004		
#define	RUMP_SO_ERROR	0x1007		
#define	RUMP_SO_TYPE		0x1008		
#define	RUMP_SO_OVERFLOWED	0x1009		
#define	RUMP_SO_NOHEADER	0x100a		
#define RUMP_SO_SNDTIMEO	0x100b		
#define RUMP_SO_RCVTIMEO	0x100c		
#define	RUMP_SOL_SOCKET	0xffff		

#endif /* _RUMP_RUMPDEFS_H_ */
