#	$Id: Makefile,v 1.1 1993/10/08 05:26:31 cgd Exp $

RPCDIR=		${.CURDIR}/../src
.PATH:		${RPCDIR}

.include "${.CURDIR}/../Makefile.src"

HDRS=		${RPCSRCS:R:S/$/.h/g}
CLEANFILES+=	${HDRS}

all:	${HDRS}

NOMAN=		noman

install:
	install -d -o bin -g bin -m 755 ${DESTDIR}/usr/include/rpcsvc
	@for i in $(HDRS); do \
		echo install ${COPY} -o bin -g bin -m 444 $$i \
		    ${DESTDIR}/usr/include/rpcsvc ; \
		install ${COPY} -o bin -g bin -m 444 $$i \
		    ${DESTDIR}/usr/include/rpcsvc ; \
	done
	@for i in $(RPCSRCS) $(RPCHDRS); do \
		echo install -c -o bin -g bin -m 444 ${RPCDIR}/$$i \
		    ${DESTDIR}/usr/include/rpcsvc ; \
		install -c -o bin -g bin -m 444 ${RPCDIR}/$$i \
		    ${DESTDIR}/usr/include/rpcsvc ; \
	done

# new suffixes have to go afterwards, because bsd.lib.mk clears them
.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.dep"
