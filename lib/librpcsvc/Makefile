#	$NetBSD: Makefile,v 1.39.34.1 2012/04/17 00:05:32 yamt Exp $

WARNS=	4

USE_FORT?= yes	# network protocol library

NOMAN=		# defined

RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x yp.x yppasswd.x
RPC_INCS=	${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}	

HDRS=           ${RPC_INCS}
SRCS=		${RPC_XDRFILES} ${HDRS}

INCS=		${HDRS} ${RPCSRCS}
INCSDIR=	/usr/include/rpcsvc

LIB=		rpcsvc

.include <bsd.rpc.mk>
.include <bsd.lib.mk>
