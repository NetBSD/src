# Makefile for uuto
# $Id: Makefile,v 1.3 1993/09/30 02:19:36 cgd Exp $

BINDIR=		$(bindir)

NOPROG=
CLEANFILES=	uuto

all:	${CLEANFILES}

uuto:	uuto.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    uuto ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
