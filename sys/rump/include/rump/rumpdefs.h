/*	$NetBSD: rumpdefs.h,v 1.10.4.3 2013/01/23 00:06:28 yamt Exp $	*/

/*
 *	AUTOMATICALLY GENERATED.  DO NOT EDIT.
 */

#ifndef _RUMP_RUMPDEFS_H_
#define _RUMP_RUMPDEFS_H_

#include <rump/rump_namei.h>

/*	NetBSD: fcntl.h,v 1.43 2012/11/18 17:41:54 manu Exp 	*/
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

/*	NetBSD: vnode.h,v 1.237 2012/11/18 18:39:24 pooka Exp 	*/
enum rump_vtype	{ RUMP_VNON, RUMP_VREG, RUMP_VDIR, RUMP_VBLK, RUMP_VCHR, RUMP_VLNK, RUMP_VSOCK, RUMP_VFIFO, RUMP_VBAD };
#define	RUMP_LK_SHARED	0x00000001	
#define	RUMP_LK_EXCLUSIVE	0x00000002	
#define	RUMP_LK_NOWAIT	0x00000010	
#define	RUMP_LK_RETRY	0x00020000	

/*	NetBSD: errno.h,v 1.39 2006/10/31 00:38:07 cbiere Exp 	*/
#define	RUMP_EPERM		1		/* Operation not permitted */
#define	RUMP_ENOENT		2		/* No such file or directory */
#define	RUMP_ESRCH		3		/* No such process */
#define	RUMP_EINTR		4		/* Interrupted system call */
#define	RUMP_EIO		5		/* Input/output error */
#define	RUMP_ENXIO		6		/* Device not configured */
#define	RUMP_E2BIG		7		/* Argument list too long */
#define	RUMP_ENOEXEC		8		/* Exec format error */
#define	RUMP_EBADF		9		/* Bad file descriptor */
#define	RUMP_ECHILD		10		/* No child processes */
#define	RUMP_EDEADLK		11		/* Resource deadlock avoided */
#define	RUMP_ENOMEM		12		/* Cannot allocate memory */
#define	RUMP_EACCES		13		/* Permission denied */
#define	RUMP_EFAULT		14		/* Bad address */
#define	RUMP_ENOTBLK		15		/* Block device required */
#define	RUMP_EBUSY		16		/* Device busy */
#define	RUMP_EEXIST		17		/* File exists */
#define	RUMP_EXDEV		18		/* Cross-device link */
#define	RUMP_ENODEV		19		/* Operation not supported by device */
#define	RUMP_ENOTDIR		20		/* Not a directory */
#define	RUMP_EISDIR		21		/* Is a directory */
#define	RUMP_EINVAL		22		/* Invalid argument */
#define	RUMP_ENFILE		23		/* Too many open files in system */
#define	RUMP_EMFILE		24		/* Too many open files */
#define	RUMP_ENOTTY		25		/* Inappropriate ioctl for device */
#define	RUMP_ETXTBSY		26		/* Text file busy */
#define	RUMP_EFBIG		27		/* File too large */
#define	RUMP_ENOSPC		28		/* No space left on device */
#define	RUMP_ESPIPE		29		/* Illegal seek */
#define	RUMP_EROFS		30		/* Read-only file system */
#define	RUMP_EMLINK		31		/* Too many links */
#define	RUMP_EPIPE		32		/* Broken pipe */
#define	RUMP_EDOM		33		/* Numerical argument out of domain */
#define	RUMP_ERANGE		34		/* Result too large or too small */
#define	RUMP_EAGAIN		35		/* Resource temporarily unavailable */
#define	RUMP_EWOULDBLOCK	EAGAIN		/* Operation would block */
#define	RUMP_EINPROGRESS	36		/* Operation now in progress */
#define	RUMP_EALREADY	37		/* Operation already in progress */
#define	RUMP_ENOTSOCK	38		/* Socket operation on non-socket */
#define	RUMP_EDESTADDRREQ	39		/* Destination address required */
#define	RUMP_EMSGSIZE	40		/* Message too long */
#define	RUMP_EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	RUMP_ENOPROTOOPT	42		/* Protocol option not available */
#define	RUMP_EPROTONOSUPPORT	43		/* Protocol not supported */
#define	RUMP_ESOCKTNOSUPPORT	44		/* Socket type not supported */
#define	RUMP_EOPNOTSUPP	45		/* Operation not supported */
#define	RUMP_EPFNOSUPPORT	46		/* Protocol family not supported */
#define	RUMP_EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	RUMP_EADDRINUSE	48		/* Address already in use */
#define	RUMP_EADDRNOTAVAIL	49		/* Can't assign requested address */
#define	RUMP_ENETDOWN	50		/* Network is down */
#define	RUMP_ENETUNREACH	51		/* Network is unreachable */
#define	RUMP_ENETRESET	52		/* Network dropped connection on reset */
#define	RUMP_ECONNABORTED	53		/* Software caused connection abort */
#define	RUMP_ECONNRESET	54		/* Connection reset by peer */
#define	RUMP_ENOBUFS		55		/* No buffer space available */
#define	RUMP_EISCONN		56		/* Socket is already connected */
#define	RUMP_ENOTCONN	57		/* Socket is not connected */
#define	RUMP_ESHUTDOWN	58		/* Can't send after socket shutdown */
#define	RUMP_ETOOMANYREFS	59		/* Too many references: can't splice */
#define	RUMP_ETIMEDOUT	60		/* Operation timed out */
#define	RUMP_ECONNREFUSED	61		/* Connection refused */
#define	RUMP_ELOOP		62		/* Too many levels of symbolic links */
#define	RUMP_ENAMETOOLONG	63		/* File name too long */
#define	RUMP_EHOSTDOWN	64		/* Host is down */
#define	RUMP_EHOSTUNREACH	65		/* No route to host */
#define	RUMP_ENOTEMPTY	66		/* Directory not empty */
#define	RUMP_EPROCLIM	67		/* Too many processes */
#define	RUMP_EUSERS		68		/* Too many users */
#define	RUMP_EDQUOT		69		/* Disc quota exceeded */
#define	RUMP_ESTALE		70		/* Stale NFS file handle */
#define	RUMP_EREMOTE		71		/* Too many levels of remote in path */
#define	RUMP_EBADRPC		72		/* RPC struct is bad */
#define	RUMP_ERPCMISMATCH	73		/* RPC version wrong */
#define	RUMP_EPROGUNAVAIL	74		/* RPC prog. not avail */
#define	RUMP_EPROGMISMATCH	75		/* Program version wrong */
#define	RUMP_EPROCUNAVAIL	76		/* Bad procedure for program */
#define	RUMP_ENOLCK		77		/* No locks available */
#define	RUMP_ENOSYS		78		/* Function not implemented */
#define	RUMP_EFTYPE		79		/* Inappropriate file type or format */
#define	RUMP_EAUTH		80		/* Authentication error */
#define	RUMP_ENEEDAUTH	81		/* Need authenticator */
#define	RUMP_EIDRM		82		/* Identifier removed */
#define	RUMP_ENOMSG		83		/* No message of desired type */
#define	RUMP_EOVERFLOW	84		/* Value too large to be stored in data type */
#define	RUMP_EILSEQ		85		/* Illegal byte sequence */
#define RUMP_ENOTSUP		86		/* Not supported */
#define RUMP_ECANCELED	87		/* Operation canceled */
#define RUMP_EBADMSG		88		/* Bad or Corrupt message */
#define RUMP_ENODATA		89		/* No message available */
#define RUMP_ENOSR		90		/* No STREAM resources */
#define RUMP_ENOSTR		91		/* Not a STREAM */
#define RUMP_ETIME		92		/* STREAM ioctl timeout */
#define	RUMP_ENOATTR		93		/* Attribute not found */
#define	RUMP_EMULTIHOP	94		/* Multihop attempted */ 
#define	RUMP_ENOLINK		95		/* Link has been severed */
#define	RUMP_EPROTO		96		/* Protocol error */
#define	RUMP_ELAST		96		/* Must equal largest errno */
#define	RUMP_EJUSTRETURN	-2		/* don't modify regs, just return */
#define	RUMP_ERESTART	-3		/* restart syscall */
#define	RUMP_EPASSTHROUGH	-4		/* ioctl not handled by this layer */
#define	RUMP_EDUPFD		-5		/* Dup given fd */
#define	RUMP_EMOVEFD		-6		/* Move given fd */

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

/*	NetBSD: module.h,v 1.32 2012/10/17 17:48:48 dyoung Exp 	*/
struct rump_modctl_load {
	const char *ml_filename;

	int ml_flags;

	const char *ml_props;
	size_t ml_propslen;
};

/*	NetBSD: ufsmount.h,v 1.39 2012/10/19 17:09:08 drochner Exp 	*/
struct rump_ufs_args {
	char	*fspec;			/* block special device to mount */
};

#endif /* _RUMP_RUMPDEFS_H_ */
