#	$NetBSD: Makefile,v 1.18 1996/10/18 05:45:05 thorpej Exp $


RPCSRCS=	bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
		nlm_prot.x rex.x rnusers.x rusers.x rquota.x rstat.x rwall.x\
		sm_inter.x spray.x yp.x yppasswd.x
SRCS=		${RPCSRCS:R:S/$/.c/g}
HDRS=           ${RPCSRCS:R:S/$/.h/g}
CLEANFILES+=	${SRCS} ${HDRS}

LIB=		rpcsvc
NOMAN=
NOPIC=

all:		${HDRS}
beforedepend:	${HDRS}

includes:	${HDRS}
	${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
	    ${DESTDIR}/usr/include/rpcsvc
	@for i in $(HDRS); do \
	    j="cmp -s $$i ${DESTDIR}/usr/include/rpcsvc/$$i || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}/usr/include/rpcsvc"; \
	    echo $$j; \
	    eval "$$j"; \
	done
	@cd ${.CURDIR}; for i in $(RPCSRCS); do \
	    j="cmp -s $(.CURDIR)/$$i ${DESTDIR}/usr/include/rpcsvc/$$i || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$i \
		${DESTDIR}/usr/include/rpcsvc"; \
	    echo $$j; \
	    eval "$$j"; \
	done

# new suffixes have to go afterwards, because bsd.lib.mk clears them
.include <bsd.lib.mk>
.SUFFIXES: .x .c .h

.x.c:
	rpcgen -c ${.IMPSRC} -o ${.TARGET}

.x.h:
	rpcgen -h ${.IMPSRC} -o ${.TARGET}
