#	$NetBSD: Makefile,v 1.25 2000/07/25 06:29:46 cgd Exp $

RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x yp.x yppasswd.x
SRCS=		${RPCSRCS:.x=.c} ${HDRS}
HDRS=           ${RPCSRCS:.x=.h}
CLEANFILES+=	${SRCS} ${HDRS}

INCS=		${HDRS} ${RPCSRCS}
INCSDIR=	/usr/include/rpcsvc

LIB=		rpcsvc
MKMAN=		no
MKPIC=		no

# We don't use explicit suffix rules here to avoid dependencies in the
# Installed files.

.for I in ${RPCSRCS}
${I:.x=.c}: $I
	${RPCGEN} -c ${.ALLSRC} -o ${.TARGET}

${I:.x=.h}: $I
	${RPCGEN} -h ${.ALLSRC} -o ${.TARGET}
.endfor

.include <bsd.lib.mk>
