#	$NetBSD: Makefile,v 1.4 1996/09/12 16:59:01 cgd Exp $

ECOFF_COMPAT= yes

CFLAGS+=	-DLIBC_SCCS -DPIC -DDYNAMIC
CFLAGS+=	-I${.CURDIR}/../../libexec/rtld/include

.if defined(ECOFF_COMPAT)
CFLAGS+=	-DECOFF_COMPAT
.endif

OBJS=		crt0.o gcrt0.o crtbegin.o crtend.o

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

crtbegin.o: crtbegin.c
	@echo ${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo ${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}
	@${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

.include <bsd.prog.mk>
