#	$NetBSD: Makefile,v 1.6 1996/10/18 02:57:34 thorpej Exp $
# Makefile for uuto

BINDIR=		$(bindir)

NOPROG=
CLEANFILES=	uuto

all:	${CLEANFILES}

uuto:	uuto.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uuto.in > $(.TARGET)

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    uuto ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
