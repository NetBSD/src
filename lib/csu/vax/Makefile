#	$NetBSD: Makefile,v 1.6 1997/05/26 03:57:32 cjs Exp $
#	from: @(#)Makefile	5.6 (Berkeley) 5/22/91

CFLAGS+=	-DLIBC_SCCS -I${.CURDIR}/..
OBJS=		crt0.o gcrt0.o scrt0.o
CLEANFILES+=	core a.out

all: ${OBJS}

crt0.o: crt0.c
	${COMPILE.c} -DCRT0 -UDYNAMIC ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

gcrt0.o: crt0.c
	${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

scrt0.o: crt0.c
	${COMPILE.c} -DSCRT0 ${.ALLSRC} -o ${.TARGET}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

.include <bsd.prog.mk>
