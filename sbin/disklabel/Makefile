#	@(#)Makefile	5.6 (Berkeley) 4/1/91
#
#	$Header: /cvsroot/src/sbin/disklabel/Makefile,v 1.4 1993/04/10 20:17:42 cgd Exp $

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
MAN8=	disklabel.0
BINGRP=operator
BINMODE=2550
CLEANFILES=disklabel.5.0

all: disklabel.5.0

beforeinstall:
	@if [ ! -d ${DESTDIR}${MANDIR}5 ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}5 ; \
                mkdir -p ${DESTDIR}${MANDIR}5 ; \
                chown root.wheel ${DESTDIR}${MANDIR}5 ; \
                chmod 755 ${DESTDIR}${MANDIR}5 ; \
        else \
                true ; \
        fi
	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} disklabel.5.0 \
	    ${DESTDIR}${MANDIR}5/disklabel.0

.include <bsd.prog.mk>
