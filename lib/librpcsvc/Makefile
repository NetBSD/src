#	$NetBSD: Makefile,v 1.34 2001/12/12 00:51:05 tv Exp $

RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x yp.x yppasswd.x
SRCS=		${RPCSRCS:.x=.c} ${HDRS}
HDRS=           ${RPCSRCS:.x=.h}
CLEANFILES+=	${SRCS} ${HDRS}

INCS=		${HDRS} ${RPCSRCS}
INCSDIR=	/usr/include/rpcsvc

LIB=		rpcsvc
NOMAN=		# defined
NOPIC=		# defined

.include <bsd.lib.mk>

# Resolve rpcgen's path, to allow it to be a dependency.
_RPCGEN:=	${RPCGEN}
.if ${_RPCGEN:M/*} == ""
_RPCGEN!=	type ${RPCGEN} | awk '{print $$NF}'
.endif

# We don't use explicit suffix rules here to avoid dependencies in the
# Installed files.

.for I in ${RPCSRCS}
${I:.x=.c}: $I ${_RPCGEN}
	${RPCGEN} -C -c ${.CURDIR}/$I -o ${.TARGET}

${I:.x=.h}: $I ${_RPCGEN}
	${RPCGEN} -C -h ${.CURDIR}/$I -o ${.TARGET}
.endfor
