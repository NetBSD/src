#	$NetBSD: Makefile,v 1.20 1997/05/09 07:50:15 mycroft Exp $


RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x yp.x yppasswd.x
SRCS=		${RPCSRCS:R:S/$/.c/g} ${HDRS}
HDRS=           ${RPCSRCS:R:S/$/.h/g}
CLEANFILES+=	${SRCS} ${HDRS}

INCS=${HDRS} ${RPCSRCS}
INCSDIR=/usr/include/rpcsvc

LIB=		rpcsvc
NOMAN=
NOPIC=

# We don't use explicit suffix rules here to avoid dependencies in the
# Installed files.

.for I in ${RPCSRCS}
${I:S/.x/.c/}: $I
	rpcgen -c ${.ALLSRC} -o ${.TARGET}

${I:S/.x/.h/}: $I
	rpcgen -h ${.ALLSRC} -o ${.TARGET}
.endfor

.include <bsd.lib.mk>
