#	from: @(#)Makefile	5.6 (Berkeley) 5/22/91
#	$Id: Makefile,v 1.5 1993/08/01 05:41:42 mycroft Exp $

CFLAGS+=	-DLIBC_SCCS
OBJS=		crt0.o gcrt0.o
CLEANFILES+=	gmon.o moncrt0.o core a.out

all: ${OBJS}

crt0.o: crt0.c
	${CC} ${CFLAGS} -c -DCRT0 ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

moncrt0.o: crt0.c
	${CC} ${CFLAGS} -c -DMCRT0 ${.ALLSRC} -o ${.TARGET}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

gcrt0.o: moncrt0.o gmon.o
	${LD} -x -r -o ${.TARGET} moncrt0.o gmon.o

gmon.o: gmon.c gmon.h
	${CC} ${CFLAGS} -c ${.IMPSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

install:
	@if [ ! -d ${DESTDIR}/usr/lib ]; then \
                /bin/rm -f ${DESTDIR}/usr/lib ; \
                mkdir -p ${DESTDIR}/usr/lib ; \
                chown root.wheel ${DESTDIR}/usr/lib ; \
                chmod 755 ${DESTDIR}/usr/lib ; \
        else \
                true ; \
        fi
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
