# Makefile for uusched
# $Id: Makefile,v 1.5 1995/08/24 05:23:28 jtc Exp $

BINDIR=		$(bindir)

NOPROG=
CLEANFILES=	uusched

all:	${CLEANFILES}

uusched:	uusched.in
	sed -e "s|@BINDIR@|$(bindir)|g" -e "s|@SBINDIR@|$(sbindir)|g" \
		$(.CURDIR)/uusched.in > $(.TARGET)

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    uusched ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
