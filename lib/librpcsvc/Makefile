#	$NetBSD: Makefile,v 1.31 2001/03/21 20:14:10 mycroft Exp $

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

# This little dance is necessary because RPCGEN doesn't get set until
# bsd.lib.mk is included at the end of the makefile.  Unfortunately
# that's after make has figured out dependencies using the (lack of) values
# of variables at the time it parses the dependency line.
.ifnmake getrpcgen
XRPCGEN != cd ${.CURDIR} && ${MAKE} -B ${MAKEFLAGS} getrpcgen
.endif

getrpcgen:
	@set -- X `type ${RPCGEN}` && shift `expr $$# - 1` && echo "$$1"

# We don't use explicit suffix rules here to avoid dependencies in the
# Installed files.

.for I in ${RPCSRCS}

${I:.x=.c}: $I ${XRPCGEN}
	${RPCGEN} -C -c ${.CURDIR}/$I -o ${.TARGET}

${I:.x=.h}: $I ${XRPCGEN}
	${RPCGEN} -C -h ${.CURDIR}/$I -o ${.TARGET}
.endfor

.include <bsd.lib.mk>
