#	$NetBSD: Makefile,v 1.37 2003/01/05 19:24:06 sommerfeld Exp $

NOMAN=		# defined
NOPIC=		# defined

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
