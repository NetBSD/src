#	from: @(#)Makefile	5.17 (Berkeley) 5/24/90
#	$Id: Makefile,v 1.4 1993/08/01 05:39:43 mycroft Exp $

PROG=	bugfiler
CFLAGS+=-I${.CURDIR} -D"BUGS_HOME=\"owner-386bsd_bugs@sun-lamp.cs.berkeley.edu\""
SRCS=	bugfiler.c error.c gethead.c process.c redist.c reply.c
BINOWN=	root
BINMODE=4555
MAN1=	sendbug.0
MAN8=	bugfiler.0

beforeinstall:
	install -c -o bin -g ${BINGRP} -m 555 \
	    ${.CURDIR}/sendbug.sh ${DESTDIR}/usr/bin/sendbug
	install -c -o bin -g ${BINGRP} -m 444 ${.CURDIR}/bugformat \
	    ${DESTDIR}/usr/share/misc

.include <bsd.prog.mk>
