#	$Id: Makefile,v 1.8 1994/01/20 23:04:13 jtc Exp $

RPCDIR=		${.CURDIR}/src
.PATH:		${RPCDIR}

RPCSRCS=	amq.x bootparam_prot.x klm_prot.x mount.x nfs_prot.x\
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

beforeinstall:
	install -d -o bin -g bin -m 755 ${DESTDIR}/usr/include/rpcsvc
	@for i in $(HDRS); do \
		echo install ${COPY} -o bin -g bin -m 444 $$i \
			${DESTDIR}/usr/include/rpcsvc ; \
		install ${COPY} -o bin -g bin -m 444 $$i \
			${DESTDIR}/usr/include/rpcsvc ; \
	done
	@for i in $(RPCSRCS); do \
		echo install -c -o bin -g bin -m 444 ${RPCDIR}/$$i \
			${DESTDIR}/usr/include/rpcsvc ; \
		install -c -o bin -g bin -m 444 ${RPCDIR}/$$i \
			${DESTDIR}/usr/include/rpcsvc ; \
	done

# new suffixes have to go afterwards, because bsd.lib.mk clears them
.include <bsd.lib.mk>
.SUFFIXES: .x .c .h

.x.c:
	@echo generating $@...
	@rpcgen -c ${.IMPSRC} -o ${.TARGET}

.x.h:
	@echo generating $@...
	@rpcgen -h ${.IMPSRC} -o ${.TARGET}
