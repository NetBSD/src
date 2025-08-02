#	$NetBSD: Makefile,v 1.41.12.1 2025/08/02 05:54:57 perseant Exp $

WARNS=	4

USE_FORT?= yes	# network protocol library

NOMAN=		# defined

RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x

.include <bsd.own.mk>

EXPSYM_SRCS=	rpcsvc.common.expsym

.if (${MKYP} != "no")
RPCSRCS+=	yp.x yppasswd.x
EXPSYM_SRCS+=	rpcsvc.yp.expsym
.endif

LIB_EXPSYM=	rpcsvc.expsym
rpcsvc.expsym: ${EXPSYM_SRCS}
	${_MKTARGET_CREATE}
	LC_ALL=C sort -m ${.ALLSRC} >${.TARGET}.tmp && \
	${MV} ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	rpcsvc.expsym rpcsvc.expsym.tmp

RPC_INCS=	${RPCSRCS:.x=.h}
RPC_XDRFILES=	${RPCSRCS:.x=_xdr.c}	

HDRS=           ${RPC_INCS}
SRCS=		${RPC_XDRFILES} ${HDRS}

INCS=		${HDRS} ${RPCSRCS}
INCSDIR=	/usr/include/rpcsvc

LIB=		rpcsvc

.include <bsd.rpc.mk>
.include <bsd.lib.mk>
