#	$NetBSD: Makefile,v 1.8 1997/03/05 03:45:05 jonathan Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/1/93

CFLAGS+=	-DLIBC_SCCS -fPIC -DPIC -DDYNAMIC -DELFSIZE=32
#CFLAGS+=	-I/usr/src/libexec/ld.elf_so

OBJS=		crt0.o gcrt0.o crtn.o # crtbegin.o crtend.o
CLEANFILES+=	core a.out

all: ${OBJS}

crt0.o: crt0.c
	@echo ${CC} ${CFLAGS} -DCRT0 -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -DCRT0 -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: crt0.c
	@echo ${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtn.o: crtend.c
	@echo ${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -DMCRT0 -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
