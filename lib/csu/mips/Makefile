#	$NetBSD: Makefile,v 1.16.2.1 1997/11/09 00:59:29 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/1/93

CPPFLAGS+=	-DLIBC_SCCS -DPIC -DDYNAMIC -DELFSIZE=32
CPPFLAGS+=	-I${.CURDIR}/../../../libexec/ld.elf_so
COPTS+=		-fPIC

OBJS=		crt0.o gcrt0.o crtn.o # crtbegin.o crtend.o
CLEANFILES+=	core a.out

all: ${OBJS}

crt0.o: crt0.c
	@echo "${COMPILE.c} -DCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: crt0.c
	@echo "${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtn.o: crtend.c
	@echo "${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib
	ln -f ${DESTDIR}/usr/lib/crt0.o ${DESTDIR}/usr/lib/scrt0.o

.include <bsd.prog.mk>
