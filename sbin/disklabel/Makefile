#	from: @(#)Makefile	5.6 (Berkeley) 4/1/91
#	$Id: Makefile,v 1.8 1994/02/12 17:49:32 chopps Exp $

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN8=	disklabel.0
BINGRP=operator
BINMODE=2550
CLEANFILES=disklabel.5.0
.if (${MACHINE} == "amiga")
CFLAGS+=-D${MACHINE}
.endif

.if !defined(NOMAN)
all: disklabel.5.0
.endif

beforeinstall:
	@if [ ! -d ${DESTDIR}${MANDIR}5 ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}5 ; \
                mkdir -p ${DESTDIR}${MANDIR}5 ; \
                chown root.wheel ${DESTDIR}${MANDIR}5 ; \
                chmod 755 ${DESTDIR}${MANDIR}5 ; \
        else \
                true ; \
        fi
.if !defined(NOMAN)
	install ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} disklabel.5.0 \
	    ${DESTDIR}${MANDIR}5/disklabel.0
.endif

.include <bsd.prog.mk>
