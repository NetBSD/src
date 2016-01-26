#	$NetBSD: Makefile,v 1.9 2016/01/26 23:12:15 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../netsmb

LIB=	rumpdev_netsmb
COMMENT=SMB protocol communicator (required by SMB/CIFS)

IOCONF=	NETSMB.ioconf
SRCS=	iconv.c smb_conn.c smb_crypt.c smb_dev.c smb_iod.c smb_rq.c	\
	smb_smb.c smb_subr.c smb_trantcp.c smb_usr.c subr_mchain.c

SRCS+=	netsmb_component.c

#CPPFLAGS+=	-DSMB_SOCKET_DEBUG -DSMB_IOD_DEBUG

SRCS+=	netsmb_iconv.c
RUMPCOMP_USER_SRCS=	netsmb_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
