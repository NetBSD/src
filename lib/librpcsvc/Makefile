#	$NetBSD: Makefile,v 1.40.32.1 2019/06/10 22:05:27 christos Exp $

WARNS=	4

USE_FORT?= yes	# network protocol library

NOMAN=		# defined

RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x

.include <bsd.own.mk>

.if (${MKYP} != "no")
RPCSRCS+=	yp.x yppasswd.x
.endif

RPC_INCS=	${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}	

HDRS=           ${RPC_INCS}
SRCS=		${RPC_XDRFILES} ${HDRS}

INCS=		${HDRS} ${RPCSRCS}
INCSDIR=	/usr/include/rpcsvc

LIB=		rpcsvc

.include <bsd.rpc.mk>
.include <bsd.lib.mk>
