#	$NetBSD: Makefile,v 1.20 1999/03/19 23:34:51 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/1/93

CPPFLAGS+=	-DLIBC_SCCS -DPIC -DDYNAMIC -DELFSIZE=32
CPPFLAGS+=	-I${.CURDIR}/../../../libexec/ld.elf_so
CPPFLAGS+=	-I${.CURDIR}/../common_elf
COPTS+=		-fPIC

.PATH:		${.CURDIR}/../common_elf

OBJS=		crt0.o gcrt0.o crtbegin.o crtend.o

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

#crtn.o: crtend.c
#	@echo "${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}"
#	@${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}.o
#	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
#	@rm -f ${.TARGET}.o

crtbegin.o: crtbegin.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib
	rm -f ${DESTDIR}/usr/lib/crtbeginS.o ${DESTDIR}/usr/lib/crtendS.o
	ln ${DESTDIR}/usr/lib/crtbegin.o ${DESTDIR}/usr/lib/crtbeginS.o
	ln ${DESTDIR}/usr/lib/crtend.o ${DESTDIR}/usr/lib/crtendS.o
	ln -f ${DESTDIR}/usr/lib/crtend.o ${DESTDIR}/usr/lib/crtn.o

.include <bsd.prog.mk>
