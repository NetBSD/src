#	$NetBSD: Makefile,v 1.6 1996/10/18 02:56:47 thorpej Exp $
# Makefile for uusched

BINDIR=		$(bindir)

NOPROG=
CLEANFILES=	uusched

all:	${CLEANFILES}

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    uusched ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
