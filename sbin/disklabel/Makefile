#	@(#)Makefile	5.6 (Berkeley) 4/1/91
#
#	$Header: /cvsroot/src/sbin/disklabel/Makefile,v 1.5 1993/04/25 06:58:16 mycroft Exp $

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN8=	disklabel.0
BINGRP=operator
BINMODE=2550
CLEANFILES=disklabel.5.0

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
	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} disklabel.5.0 \
	    ${DESTDIR}${MANDIR}5/disklabel.0
.endif

.include <bsd.prog.mk>
