#	$NetBSD: Makefile,v 1.5.2.1 2015/09/22 12:06:13 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../netsmb

LIB=	rumpdev_netsmb
IOCONF=	NETSMB.ioconf
SRCS=	iconv.c smb_conn.c smb_crypt.c smb_dev.c smb_iod.c smb_rq.c	\
	smb_smb.c smb_subr.c smb_trantcp.c smb_usr.c subr_mchain.c

SRCS+=	netsmb_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs
#CPPFLAGS+=	-DSMB_SOCKET_DEBUG -DSMB_IOD_DEBUG

SRCS+=	netsmb_iconv.c
RUMPCOMP_USER_SRCS=	netsmb_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
