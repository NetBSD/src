#	$NetBSD: Makefile,v 1.9 1997/05/09 07:50:11 mycroft Exp $
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

.include <bsd.prog.mk>
